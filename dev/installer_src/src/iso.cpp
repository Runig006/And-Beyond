// ISO9660 reader for the game's CD images (see iso.h).
#include "iso.h"

#include <algorithm>
#include <cstring>

static std::wstring CueBin(const std::wstring &cue)
{
	FILE *f = _wfopen(cue.c_str(), L"rb");
	if (!f) {
		return L"";
	}
	char line[1024];
	std::string name;
	while (fgets(line, sizeof line, f)) {
		const char *p = strstr(line, "FILE");
		const char *q = p ? strchr(p, '"') : nullptr;
		const char *r = q ? strchr(q + 1, '"') : nullptr;
		if (r) {
			name.assign(q + 1, r);
			break;
		}
	}
	fclose(f);
	if (name.empty()) {
		return L"";
	}
	std::wstring dir = cue.substr(0, cue.find_last_of(L"\\/") + 1);
	return dir + std::wstring(name.begin(), name.end());
}

IsoImage::~IsoImage()
{
	if (f_) {
		fclose(f_);
	}
}

bool IsoImage::Open(const std::wstring &path)
{
	std::wstring file = path;
	std::wstring ext = path.size() > 4 ? path.substr(path.size() - 4) : L"";
	std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
	if (ext == L".cue") {
		file = CueBin(path);
	}
	f_ = _wfopen(file.c_str(), L"rb");
	if (!f_) {
		return false;
	}
	if (ext == L".cdi") {
		if (!OpenCdi()) {
			return false;
		}
	} else {
		// A raw sector starts with the sync pattern 00 FF x10 00; byte 15 is the mode
		uint8_t head[16];
		static const uint8_t sync[12] = {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0};
		if (fread(head, 1, 16, f_) == 16 && memcmp(head, sync, 12) == 0) {
			sectorSize_ = 2352;
			dataOffset_ = head[15] == 2 ? 24 : 16;
		}
	}
	// Volume descriptors from sector 16: 1 = primary, 2 with escape "%/" = Joliet, 255 = end
	uint8_t vd[2048];
	uint8_t root[34] = {};
	bool joliet = false;
	for (uint32_t lba = firstLba_ + 16; lba < firstLba_ + 32 && ReadSectors(lba, 1, vd) && memcmp(vd + 1, "CD001", 5) == 0;
		lba++) {
		if (vd[0] == 255) {
			break;
		}
		if (vd[0] == 1 && !joliet) {
			memcpy(root, vd + 156, 34);
		}
		if (vd[0] == 2 && vd[88] == '%' && vd[89] == '/') {
			memcpy(root, vd + 156, 34);
			joliet = true;
		}
	}
	uint32_t rootLba, rootSize;
	memcpy(&rootLba, root + 2, 4);
	memcpy(&rootSize, root + 10, 4);
	if (rootSize == 0) {
		return false;
	}
	Walk(rootLba, rootSize, "", L"", joliet, 0);
	return !files_.empty();
}

// Dreamcast: the game's track is the second session's, 2336-byte sectors (8 of subheader, then the data) numbered
// from 45000. Its volume descriptor is the last "\1CD001\1" in the file (the first one is the small first session's).
bool IsoImage::OpenCdi()
{
	static const uint8_t mark[7] = {1, 'C', 'D', '0', '0', '1', 1};
	const size_t chunk = 1 << 24;
	std::vector<uint8_t> buf(chunk + sizeof mark);
	int64_t pos = 0, last = -1;
	for (;;) {
		_fseeki64(f_, pos, SEEK_SET);
		size_t got = fread(buf.data(), 1, buf.size(), f_);
		for (size_t i = 0; i + sizeof mark <= got; i++) {
			if (buf[i] == 1 && memcmp(&buf[i], mark, sizeof mark) == 0) {
				last = pos + (int64_t)i;
			}
		}
		if (got < buf.size()) {
			break;
		}
		pos += chunk;
	}
	if (last < 0) {
		return false;
	}
	sectorSize_ = 2336;
	dataOffset_ = 0;
	firstLba_ = 45000;
	base_ = last - 16 * 2336;  // the descriptor is the data of sector 45016
	return true;
}

bool IsoImage::ReadSectors(uint32_t lba, uint32_t count, uint8_t *out)
{
	for (uint32_t i = 0; i < count; i++) {
		int64_t at = base_ + (int64_t)(lba + i - firstLba_) * sectorSize_ + dataOffset_;
		if (lba + i < firstLba_ || _fseeki64(f_, at, SEEK_SET) != 0) {
			return false;
		}
		if (fread(out + i * 2048, 1, 2048, f_) != 2048) {
			return false;
		}
	}
	return true;
}

void IsoImage::Walk(uint32_t lba, uint32_t size, const std::string &key, const std::wstring &name, bool joliet,
	int depth)
{
	if (depth > 16) {
		return;
	}
	std::vector<uint8_t> dir((size + 2047) / 2048 * 2048);
	if (dir.empty() || !ReadSectors(lba, (uint32_t)dir.size() / 2048, dir.data())) {
		return;
	}
	size_t pos = 0;
	while (pos < size) {
		uint8_t len = dir[pos];
		if (len == 0) {
			pos = (pos / 2048 + 1) * 2048;  // rest of the sector is empty
			continue;
		}
		if (len < 34 || pos + len > dir.size() || 33u + dir[pos + 32] > len) {
			return;  // damaged record
		}
		uint32_t elba, esize;
		memcpy(&elba, &dir[pos + 2], 4);
		memcpy(&esize, &dir[pos + 10], 4);
		uint8_t flags = dir[pos + 25];
		const uint8_t *raw = &dir[pos + 33];
		uint8_t rawLen = dir[pos + 32];
		pos += len;
		if (rawLen == 1 && (raw[0] == 0 || raw[0] == 1)) {
			continue;
		}
		std::wstring wide;
		if (joliet) {
			for (int i = 0; i + 1 < rawLen; i += 2) {
				wide += (wchar_t)(raw[i] << 8 | raw[i + 1]);  // UCS-2 big endian
			}
		} else {
			wide.assign(raw, raw + rawLen);
		}
		wide = wide.substr(0, wide.find(L';'));
		// A damaged or crafted disc must not name a file outside the game folder
		if (wide.empty() || wide == L"." || wide == L".." || wide.find_first_of(L"\\/:") != std::wstring::npos) {
			continue;
		}
		std::string upper;
		for (wchar_t c : wide) {
			upper += c < 128 ? (char)toupper(c) : '?';
		}
		if (flags & 2) {
			Walk(elba, esize, key + upper + "\\", name + wide + L"\\", joliet, depth + 1);
		} else {
			files_[key + upper] = {elba, esize, name + wide};
		}
	}
}

bool IsoImage::ReadRaw(const std::string &path, std::vector<uint8_t> &out)
{
	auto it = files_.find(path);
	if (it == files_.end() || sectorSize_ != 2352) {
		return false;
	}
	size_t sectors = (it->second.size + 2047) / 2048;
	out.resize(sectors * 2352);
	return _fseeki64(f_, (int64_t)it->second.lba * 2352, SEEK_SET) == 0 &&
		fread(out.data(), 1, out.size(), f_) == out.size();
}

bool IsoImage::Has(const std::string &path) const
{
	return files_.count(path) != 0;
}

bool IsoImage::Read(const std::string &path, std::vector<uint8_t> &out)
{
	auto it = files_.find(path);
	if (it == files_.end()) {
		return false;
	}
	out.resize((it->second.size + 2047) / 2048 * 2048);
	if (!out.empty() && !ReadSectors(it->second.lba, (uint32_t)out.size() / 2048, out.data())) {
		return false;
	}
	out.resize(it->second.size);
	return true;
}
