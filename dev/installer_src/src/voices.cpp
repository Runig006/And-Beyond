// Voices and videos from a console disc (see voices.h): cd\audio tracks, the bosses' phrases in data\sfx and the
// videos in cd\rtlibs.
#include "voices.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cstring>

static const char *const kBosses[] = {"thug", "torq", "grav", "warp", "fixer", "xl", "nos", "wire", "plas", "fin",
	"skrell", "marl", "zurg"};

// cd\audio tracks, in the order of the PC table (004f3328) from zeb1. Also the order of the PlayStation's
// SPEAK1-6.XA: track n is SPEAK(n / 8 + 1), channel n % 8.
static std::vector<std::string> Tracks()
{
	std::vector<std::string> t;
	for (int i = 1; i <= 14; i++) {
		t.push_back("zeb" + std::to_string(i));
	}
	t.insert(t.end(), {"lgms", "badguy", "weapon"});
	for (const char *b : kBosses) {
		t.push_back(std::string("i") + b);
	}
	t.push_back("izurg2");
	for (const char *b : kBosses) {
		t.push_back(std::string("e") + b);
	}
	t.insert(t.end(), {"ixr", "fmira", "mmira", "itime"});
	return t;
}

// The bosses' dubbed phrases. Dreamcast: bank PACKED\SFXnn.KAT and slot (the PC level's sound list order).
// PlayStation: VAB bank LEVELnn\*.VH/.VB and slot; the two of Zurg 2 match none (another take?): left as they are.
struct Phrase {
	const char *name;
	int dcBank, dcSlot;
	const char *psxBank;
	int psxSlot;
};
static const Phrase kPhrases[] = {
	{"usboos04", 1, 63, "LEVEL10\\LEVEL10", 63},
	{"usthug04", 1, 81, "LEVEL01\\LEVEL01", 81},
	{"usgrav04", 3, 79, "LEVEL03\\LEVEL03", 79},
	{"usgrav07", 3, 81, "LEVEL03\\LEVEL03", 81},
	{"usxl10", 6, 81, "LEVEL06\\LEVEL06", 81},
	{"usnos03", 7, 81, "LEVEL07\\LEVEL07", 81},
	{"usblac04", 10, 81, "LEVEL10\\LEVEL10", 81},
	{"usskre02", 11, 79, "LEVEL01\\LEVEL11", 79},
	{"usskre04", 11, 80, "LEVEL01\\LEVEL11", 81},
	{"uszurg04", 13, 81, "LEVEL03\\LEVEL13", 81},
	{"uszurg11b", 14, 80, nullptr, 0},
	{"uszurg18", 14, 81, nullptr, 0},
	{"usxr05", 15, 79, "LEVEL01\\XRPARTS", 79},
	{"usxr06", 15, 80, "LEVEL01\\XRPARTS", 80},
	{"usxr09", 15, 81, "LEVEL01\\XRPARTS", 81},
};

// PC video (cd\rtlibs\<name>.dll, MPEG-1) -> the consoles' one (Dreamcast MOVIES\<x>.SFD, PlayStation BUZZFMV\<x>.STR
// or BUZZFMV2). The logos (acti, dlogo, tt) stay.
struct Video {
	const char *pc;    // cd\rtlibs\<pc>.dll
	const char *disc;  // MOVIES\<disc>.SFD, BUZZFMV(2)\<disc>.STR
};
static const Video kVideos[] = {
	{"buzz01a", "BUZZ01"},
	{"buzz02", "BUZZ02"},
	{"continue", "CONTINUE"},
	{"fin_in", "FININ"},
	{"fin_out", "FINOUT"},
	{"fixer_in", "FIXERIN"},
	{"fixer_out", "FIXEROUT"},
	{"game_intro", "INTRO"},
	{"gameover_1", "OVER1"},
	{"gameover_2", "OVER2"},
	{"grav_in", "GRAVIN"},
	{"grav_out", "GRAVOUT"},
	{"marl_in", "MARLIN"},
	{"marl_out", "MARLOUT"},
	{"mech_in", "MECHIN"},
	{"mech_lose", "MECHLOSE"},
	{"mech_win", "MECHWIN"},
	{"nos_in", "NOSIN"},
	{"nos_out", "NOSOUT"},
	{"plasma_in", "PLASIN"},
	{"plasma_out", "PLASOUT"},
	{"skrell_in", "SKRIN"},
	{"skrell-zurg", "SKROUT"},
	{"thug_in", "THUGIN"},
	{"thug_out", "THUGOUT"},
	{"torque_in", "TORQIN"},
	{"torque_out", "TORQOUT"},
	{"warp_in", "WARPIN"},
	{"warp_out", "WARPOUT"},
	{"wolf_in", "WOLFIN"},
	{"wolf_out", "WOLFOUT"},
	{"xl_in", "XLIN"},
	{"xl_out", "XLOUT"},
	{"xr_complete", "XRCOMP"},
	{"zurg_in_1", "ZURG1IN"},
	{"zurg_in_2", "ZURG2IN"},
};

static std::wstring Widen(const std::string &s)
{
	return std::wstring(s.begin(), s.end());
}

static std::string Upper(std::string s)
{
	for (char &c : s) {
		c = (char)toupper((unsigned char)c);
	}
	return s;
}

static bool Exists(const std::wstring &path)
{
	return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool WriteFileAll(const std::wstring &path, const void *data, size_t size)
{
	HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD put = 0;
	bool ok = size == 0 || (WriteFile(h, data, (DWORD)size, &put, nullptr) && put == size);
	CloseHandle(h);
	return ok;
}

DiscKind IdentifyDisc(IsoImage &disc)
{
	if (disc.Has("MUSIC\\ZEB1.ADX") && disc.Has("PACKED\\SFX01.KAT") && disc.Has("MOVIES\\INTRO.SFD")) {
		return DISC_DREAMCAST;
	}
	if (disc.Has("BUZZXA\\SPEAK1.XA") && disc.Has("BUZZFMV\\INTRO.STR")) {
		return DISC_PLAYSTATION;
	}
	if (disc.Has("SETUP\\BUZZLY\\HD\\BUZZ.EXE") && disc.Has("SETUP\\BUZZLY\\CD\\AUDIO\\ZEB1.WAV")) {
		return DISC_PC;
	}
	return DISC_NONE;
}

// The game's setup\\ffmpeg.exe; written there from this program if it is not (an install from an older version)
std::wstring FindFfmpeg(const std::wstring &gameDir)
{
	if (gameDir.empty()) {
		return L"";
	}
	std::wstring setupDir = gameDir + L"\\setup", exe = setupDir + L"\\ffmpeg.exe";
	if (Exists(exe) || WriteFfmpeg(setupDir)) {
		return exe;
	}
	return L"";
}

// Runs ffmpeg without a window; its messages (stderr) go to *log when given
static bool Ffmpeg(const std::wstring &exe, const std::wstring &args, std::string *log = nullptr)
{
	std::wstring cmd = L"\"" + exe + L"\" -hide_banner -y " + args;
	SECURITY_ATTRIBUTES sa = {sizeof sa, nullptr, TRUE};
	HANDLE readPipe = nullptr, writePipe = nullptr;
	if (log && !CreatePipe(&readPipe, &writePipe, &sa, 0)) {
		return false;
	}
	if (readPipe) {
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
	}
	STARTUPINFOW si = {sizeof si};
	if (log) {
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = writePipe;
		si.hStdOutput = writePipe;
	}
	PROCESS_INFORMATION pi;
	BOOL started = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, log != nullptr, CREATE_NO_WINDOW, nullptr,
		nullptr, &si, &pi);
	if (writePipe) {
		CloseHandle(writePipe);
	}
	if (!started) {
		if (readPipe) {
			CloseHandle(readPipe);
		}
		return false;
	}
	if (log) {
		char buf[4096];
		DWORD got;
		while (ReadFile(readPipe, buf, sizeof buf, &got, nullptr) && got > 0) {
			log->append(buf, got);
		}
		CloseHandle(readPipe);
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD code = 1;
	GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return code == 0;
}

static std::wstring Q(const std::wstring &path)
{
	return L"\"" + path + L"\"";
}

// Length in seconds: the one the file states ("Duration:" of ffmpeg, as ffprobe's), or if it states none (the
// PlayStation's STR files) the length of its sound, decoding it; -1 if unknown
static double Duration(const std::wstring &ffmpeg, const std::wstring &path)
{
	int h = 0, m = 0;
	double s = 0;
	std::string log;
	Ffmpeg(ffmpeg, L"-i " + Q(path), &log);  // no output file: it only describes the input
	size_t at = log.find("Duration: ");
	if (at != std::string::npos && sscanf(log.c_str() + at + 10, "%d:%d:%lf", &h, &m, &s) == 3) {
		return h * 3600 + m * 60 + s;
	}
	log.clear();
	Ffmpeg(ffmpeg, L"-i " + Q(path) + L" -map 0:a:0 -f null -", &log);
	at = log.rfind("time=");
	if (at == std::string::npos || sscanf(log.c_str() + at + 5, "%d:%d:%lf", &h, &m, &s) != 3) {
		return -1;
	}
	return h * 3600 + m * 60 + s;
}

// Sample rate and channels of a .wav (the PC file a track replaces keeps its format)
static bool WavFormat(const std::wstring &path, int *rate, int *channels)
{
	HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	uint8_t head[512];
	DWORD got = 0;
	ReadFile(h, head, sizeof head, &got, nullptr);
	CloseHandle(h);
	for (DWORD pos = 12; pos + 16 <= got;) {
		uint32_t len;
		memcpy(&len, head + pos + 4, 4);
		if (memcmp(head + pos, "fmt ", 4) == 0 && pos + 16 <= got) {
			uint16_t ch;
			uint32_t r;
			memcpy(&ch, head + pos + 10, 2);
			memcpy(&r, head + pos + 12, 4);
			*rate = (int)r;
			*channels = ch;
			return true;
		}
		pos += 8 + len + (len & 1);
	}
	return false;
}

static bool WriteWav16(const std::wstring &path, const std::vector<short> &samples, int rate)
{
	uint32_t bytes = (uint32_t)samples.size() * 2;
	std::vector<uint8_t> out(44 + bytes);
	uint32_t riff = 36 + bytes, fmtLen = 16, byteRate = (uint32_t)rate * 2;
	uint16_t pcm = 1, mono = 1, align = 2, bits = 16;
	memcpy(&out[0], "RIFF", 4);
	memcpy(&out[4], &riff, 4);
	memcpy(&out[8], "WAVEfmt ", 8);
	memcpy(&out[16], &fmtLen, 4);
	memcpy(&out[20], &pcm, 2);
	memcpy(&out[22], &mono, 2);
	memcpy(&out[24], &rate, 4);
	memcpy(&out[28], &byteRate, 4);
	memcpy(&out[32], &align, 2);
	memcpy(&out[34], &bits, 2);
	memcpy(&out[36], "data", 4);
	memcpy(&out[40], &bytes, 4);
	if (bytes) {
		memcpy(&out[44], samples.data(), bytes);
	}
	return WriteFileAll(path, out.data(), out.size());
}

// ADPCM Yamaha AICA (Dreamcast), 4 bits per sample, low nibble first
static std::vector<short> Aica(const uint8_t *data, size_t size)
{
	static const int kScale[16] = {230, 230, 230, 230, 307, 409, 512, 614, 230, 230, 230, 230, 307, 409, 512, 614};
	static const int kDiff[8] = {1, 3, 5, 7, 9, 11, 13, 15};
	std::vector<short> out;
	out.reserve(size * 2);
	int pred = 0, step = 127;
	for (size_t i = 0; i < size; i++) {
		for (int nib : {data[i] & 15, data[i] >> 4}) {
			// The difference is worked out positive and then signed, as in C
			int diff = step * kDiff[nib & 7] >> 3;
			pred = (nib & 8) ? pred - diff : pred + diff;
			pred = pred < -32768 ? -32768 : pred > 32767 ? 32767 : pred;
			step = step * kScale[nib] >> 8;
			step = step < 127 ? 127 : step > 24576 ? 24576 : step;
			out.push_back((short)pred);
		}
	}
	return out;
}

// ADPCM of the PlayStation SPU: 16-byte blocks (shift and filter, flags, 28 samples)
static std::vector<short> Spu(const uint8_t *data, size_t size)
{
	static const struct {
		int f0, f1;  // weights of the last sample and the one before, in 1/64
	} kFilters[5] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};
	std::vector<short> out;
	int s1 = 0, s2 = 0;
	for (size_t o = 0; o + 16 <= size; o += 16) {
		int shift = data[o] & 15, filter = data[o] >> 4;
		int f0 = filter < 5 ? kFilters[filter].f0 : 0, f1 = filter < 5 ? kFilters[filter].f1 : 0;
		for (int i = 2; i < 16; i++) {
			for (int nib : {data[o + i] & 15, data[o + i] >> 4}) {
				int v = ((nib >= 8 ? nib - 16 : nib) << 12) >> shift;
				v += (s1 * f0 + s2 * f1 + 32) >> 6;
				v = v < -32768 ? -32768 : v > 32767 ? 32767 : v;
				out.push_back((short)v);
				s2 = s1;
				s1 = v;
			}
		}
		if ((data[o + 1] & 1) && o > 0) {  // end of the sample
			break;
		}
	}
	return out;
}

static std::wstring OriginalsDir(const std::wstring &gameDir)
{
	return gameDir + L"\\setup\\original";
}

// Puts a converted file in the game: the first time, the original goes to setup\original
static bool Replace(const std::wstring &gameDir, std::wstring rel, const std::wstring &converted)
{
	// The file's own name on disk (the PC ones mix capitals: "USBOOS04.wav", "LGMs.wav")
	WIN32_FIND_DATAW fd;
	HANDLE found = FindFirstFileW((gameDir + L"\\" + rel).c_str(), &fd);
	if (found != INVALID_HANDLE_VALUE) {
		FindClose(found);
		rel = rel.substr(0, rel.find_last_of(L'\\') + 1) + fd.cFileName;
	}
	std::wstring dest = gameDir + L"\\" + rel;
	std::wstring keep = OriginalsDir(gameDir) + L"\\" + rel;
	if (Exists(dest) && !Exists(keep)) {
		SHCreateDirectoryExW(nullptr, keep.substr(0, keep.find_last_of(L'\\')).c_str(), nullptr);
		if (!CopyFileW(dest.c_str(), keep.c_str(), TRUE)) {
			return false;
		}
	}
	return MoveFileExW(converted.c_str(), dest.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

// Deletes the files of a folder (this program's temporary one), and the folder itself if asked
static void ClearFolder(const std::wstring &dir, bool remove)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				DeleteFileW((dir + L"\\" + fd.cFileName).c_str());
			}
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}
	if (remove) {
		RemoveDirectoryW(dir.c_str());
	}
}

// The disc's language, from its executable's texts: the PC buzz.exe, the Dreamcast 1ST_READ.BIN, the PlayStation one
// that SYSTEM.CNF boots (SLES_033.15 on the Spanish disc)
static std::string DiscLanguage(IsoImage &disc, DiscKind kind)
{
	std::vector<uint8_t> data;
	if (kind == DISC_PC) {
		bool known = false;
		const GameVersion *v = IdentifyVersion(disc, &known);
		return v ? v->name : "?";
	}
	if (kind == DISC_DREAMCAST) {
		return disc.Read("1ST_READ.BIN", data) ? TextLanguage(data) : "?";
	}
	// "BOOT = cdrom:\\SLES_033.15;1"
	if (!disc.Read("SYSTEM.CNF", data)) {
		return "?";
	}
	std::string cnf(data.begin(), data.end());
	size_t at = cnf.find(":\\");
	size_t end = at == std::string::npos ? at : cnf.find_first_of(";\r\n", at);
	if (end == std::string::npos) {
		return "?";
	}
	return disc.Read(Upper(cnf.substr(at + 2, end - at - 2)), data) ? TextLanguage(data) : "?";
}

static std::wstring VoicesFile(const std::wstring &gameDir)
{
	return gameDir + L"\\setup\\voices.txt";
}

bool VoicesFrom(const std::wstring &gameDir, std::string *language, DiscKind *kind)
{
	char text[32] = {};
	HANDLE h = CreateFileW(VoicesFile(gameDir).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0,
		nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD got = 0;
	ReadFile(h, text, sizeof text - 1, &got, nullptr);
	CloseHandle(h);
	char lang[8] = {};
	int k = 0;
	if (sscanf(text, "%7s %d", lang, &k) != 2) {
		return false;
	}
	*language = lang;
	*kind = (DiscKind)k;
	return true;
}

bool HasOriginals(const std::wstring &gameDir)
{
	return Exists(OriginalsDir(gameDir));
}

// %TEMP%\BuzzSetup while converting: emptied when it starts (what a closed or crashed run left) and deleted when it
// ends, however it ends
struct TempFolder {
	std::wstring path;
	TempFolder()
	{
		wchar_t base[MAX_PATH];
		GetTempPathW(MAX_PATH, base);
		path = std::wstring(base) + L"BuzzSetup";
		CreateDirectoryW(path.c_str(), nullptr);
		ClearFolder(path, false);
	}
	~TempFolder() { ClearFolder(path, true); }
};

bool ApplyVoices(IsoImage &disc, DiscKind kind, const std::wstring &gameDir, const std::wstring &ffmpeg,
	ProgressFn progress, void *ctx, std::wstring &failed)
{
	TempFolder temp;
	const std::wstring &tmp = temp.path;
	std::vector<std::string> tracks = Tracks();
	std::vector<std::pair<std::string, std::wstring>> pcPhrases;  // the other PC CD's us*.wav: path, file name
	if (kind == DISC_PC) {
		const std::string dir = "SETUP\\BUZZLY\\HD\\DATA\\SFX\\US";
		for (const auto &f : disc.Files()) {
			std::wstring name = f.second.name.substr(f.second.name.find_last_of(L'\\') + 1);
			if (f.first.compare(0, dir.size(), dir) == 0 && Exists(gameDir + L"\\data\\sfx\\" + name)) {
				pcPhrases.push_back({f.first, name});
			}
		}
	}
	int phrases = kind == DISC_PC ? (int)pcPhrases.size() : (int)(sizeof kPhrases / sizeof kPhrases[0]);
	int done = 0, total = (int)tracks.size() + phrases + (int)(sizeof kVideos / sizeof kVideos[0]);
	std::vector<uint8_t> data;

	// Tracks: decoded by ffmpeg into the format of the PC file they replace
	std::wstring xa;
	for (size_t n = 0; n < tracks.size(); n++) {
		std::wstring rel = L"cd\\audio\\" + Widen(tracks[n]) + L".wav";
		std::wstring out = tmp + L"\\track.wav";
		int rate = 22050, channels = 1;
		WavFormat(gameDir + L"\\" + rel, &rate, &channels);
		std::wstring format = L" -ar " + std::to_wstring(rate) + L" -ac " + std::to_wstring(channels) +
			L" -c:a pcm_s16le " + Q(out);
		bool ok;
		if (kind == DISC_PC) {
			std::wstring in = tmp + L"\\track_in.wav";
			ok = disc.Read("SETUP\\BUZZLY\\CD\\AUDIO\\" + Upper(tracks[n]) + ".WAV", data) &&
				WriteFileAll(in, data.data(), data.size()) && Ffmpeg(ffmpeg, L"-loglevel error -i " + Q(in) + format);
		} else if (kind == DISC_DREAMCAST) {
			std::string name = Upper(tracks[n]);
			if (name == "ISKRELL" || name == "ESKRELL") {
				name.replace(name.size() - 4, 4, "REEL");  // spelt "skreel" on the Dreamcast
			}
			std::wstring adx = tmp + L"\\track.adx";
			ok = disc.Read("MUSIC\\" + name + ".ADX", data) && WriteFileAll(adx, data.data(), data.size()) &&
				Ffmpeg(ffmpeg, L"-loglevel error -i " + Q(adx) + format);
		} else {
			std::wstring file = tmp + L"\\speak" + std::to_wstring(n / 8 + 1) + L".xa";
			if (file != xa) {
				ok = disc.ReadRaw("BUZZXA\\SPEAK" + std::to_string(n / 8 + 1) + ".XA", data) &&
					WriteFileAll(file, data.data(), data.size());
				xa = ok ? file : L"";
			}
			ok = !xa.empty() &&
				Ffmpeg(ffmpeg, L"-loglevel error -i " + Q(xa) + L" -map 0:a:" + std::to_wstring(n % 8) + format);
		}
		if (!ok || !Replace(gameDir, rel, out)) {
			failed = rel;
			return false;
		}
		progress(++done, total, ctx);
	}

	// Another PC CD: its phrases, into the format of the installed ones
	for (const auto &p : pcPhrases) {
		std::wstring rel = L"data\\sfx\\" + p.second;
		std::wstring in = tmp + L"\\phrase_in.wav", out = tmp + L"\\phrase.wav";
		int rate = 22050, channels = 1;
		WavFormat(gameDir + L"\\" + rel, &rate, &channels);
		if (!disc.Read(p.first, data) || !WriteFileAll(in, data.data(), data.size()) ||
			!Ffmpeg(ffmpeg, L"-loglevel error -i " + Q(in) + L" -ar " + std::to_wstring(rate) + L" -ac " +
				std::to_wstring(channels) + L" -c:a pcm_s16le " + Q(out)) ||
			!Replace(gameDir, rel, out)) {
			failed = rel;
			return false;
		}
		progress(++done, total, ctx);
	}

	// The consoles' phrases: decoded here, resampled by ffmpeg to the PC file's rate (mono)
	for (const Phrase &p : kPhrases) {
		if (kind == DISC_PC) {
			break;
		}
		std::vector<short> samples;
		int rate = 0;
		if (kind == DISC_DREAMCAST) {
			char bank[32];
			sprintf(bank, "PACKED\\SFX%02d.KAT", p.dcBank);
			uint32_t e[6];
			size_t at = 4 + (size_t)(p.dcSlot - 1) * 44;
			if (disc.Read(bank, data) && at + 24 <= data.size()) {
				memcpy(e, &data[at], 24);  // type, offset, size, rate, loop, bits (4 = AICA, 16 = PCM)
				if ((size_t)e[1] + e[2] <= data.size()) {
					if (e[5] == 4) {
						samples = Aica(&data[e[1]], e[2]);
					} else {
						samples.assign((const short *)&data[e[1]], (const short *)&data[e[1]] + e[2] / 2);
					}
					// The bank says 22050 Hz but the dubbed phrases were recorded at 44100
					rate = (int)e[3] * 2;
				}
			}
		} else if (p.psxBank) {
			std::vector<uint8_t> vh, vb;
			if (disc.Read(std::string(p.psxBank) + ".VH", vh) && disc.Read(std::string(p.psxBank) + ".VB", vb) &&
				vh.size() > 0x18) {
				uint16_t programs;
				memcpy(&programs, &vh[0x12], 2);
				size_t table = 32 + 128 * 16 + (size_t)programs * 16 * 32;
				if (table + 512 <= vh.size()) {
					size_t start = 0;
					for (int k = 1; k < p.psxSlot; k++) {
						start += (size_t)(vh[table + k * 2] | vh[table + k * 2 + 1] << 8) * 8;
					}
					size_t len = (size_t)(vh[table + p.psxSlot * 2] | vh[table + p.psxSlot * 2 + 1] << 8) * 8;
					if (start + len <= vb.size()) {
						samples = Spu(&vb[start], len);
						rate = 16000;
					}
				}
			}
		} else {
			progress(++done, total, ctx);  // not on this disc
			continue;
		}
		std::wstring rel = L"data\\sfx\\" + Widen(p.name) + L".wav";
		std::wstring raw = tmp + L"\\phrase_in.wav", out = tmp + L"\\phrase.wav";
		int pcRate = 22050, channels = 1;
		WavFormat(gameDir + L"\\" + rel, &pcRate, &channels);
		if (samples.empty() || !WriteWav16(raw, samples, rate) ||
			!Ffmpeg(ffmpeg, L"-loglevel error -i " + Q(raw) + L" -ar " + std::to_wstring(pcRate) +
				L" -ac 1 -c:a pcm_s16le " + Q(out)) ||
			!Replace(gameDir, rel, out)) {
			failed = rel;
			return false;
		}
		progress(++done, total, ctx);
	}

	// Videos: the PC picture with the disc's sound if they last the same, else the disc's picture too
	for (const auto &v : kVideos) {
		std::wstring rel = L"cd\\rtlibs\\" + Widen(v.pc) + L".dll";
		std::wstring pc = gameDir + L"\\" + rel;
		std::wstring src = tmp + L"\\video.src", out = tmp + L"\\video.mpg";
		bool read;
		double margin;
		if (kind == DISC_PC) {
			// Already the PC's format: as it is
			if (!disc.Read("SETUP\\BUZZLY\\CD\\RTLIBS\\" + Upper(v.pc) + ".DLL", data) ||
				!WriteFileAll(out, data.data(), data.size()) || !Replace(gameDir, rel, out)) {
				failed = rel;
				return false;
			}
			progress(++done, total, ctx);
			continue;
		}
		if (kind == DISC_DREAMCAST) {
			read = disc.Read(std::string("MOVIES\\") + v.disc + ".SFD", data);
			margin = 0.1;
		} else {
			read = disc.ReadRaw(std::string("BUZZFMV\\") + v.disc + ".STR", data) ||
				disc.ReadRaw(std::string("BUZZFMV2\\") + v.disc + ".STR", data);
			margin = 0.25;  // the XA sound goes in whole sectors: it ends 0.1-0.23 s after the PC video
		}
		if (!read || !WriteFileAll(src, data.data(), data.size())) {
			failed = rel;
			return false;
		}
		// When converting again, compare with the original PC video, not with the one already converted
		std::wstring original = OriginalsDir(gameDir) + L"\\" + rel;
		std::wstring picture = Exists(original) ? original : pc;
		double a = Duration(ffmpeg, picture), b = Duration(ffmpeg, src);
		bool same = a > 0 && b > 0 && (a > b ? a - b : b - a) < margin;
		std::wstring args = same ?
			L"-loglevel error -i " + Q(picture) + L" -i " + Q(src) + L" -map 0:v -map 1:a -c:v copy -c:a mp2 -ar 44100 "
				L"-b:a 192k -f mpeg " + Q(out) :
			L"-loglevel error -i " + Q(src) + L" -vf scale=320:240 -r 30000/1001 -c:v mpeg1video -b:v 2500k -maxrate "
				L"2500k -bufsize 1835k -c:a mp2 -ar 44100 -b:a 192k -f mpeg " + Q(out);
		if (!Ffmpeg(ffmpeg, args) || !Replace(gameDir, rel, out)) {
			failed = rel;
			return false;
		}
		progress(++done, total, ctx);
	}
	std::string note = DiscLanguage(disc, kind) + " " + std::to_string((int)kind);
	WriteFileAll(VoicesFile(gameDir), note.data(), note.size());
	return true;
}

// Each file of setup\original back to its place; then the folder goes
static bool RestoreFolder(const std::wstring &from, const std::wstring &to, std::wstring &failed)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((from + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) {
		return true;
	}
	bool ok = true;
	do {
		std::wstring name = fd.cFileName;
		if (name == L"." || name == L"..") {
			continue;
		}
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ok = RestoreFolder(from + L"\\" + name, to + L"\\" + name, failed) && ok;
		} else if (!MoveFileExW((from + L"\\" + name).c_str(), (to + L"\\" + name).c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
			failed = to + L"\\" + name;
			ok = false;
		}
	} while (FindNextFileW(h, &fd));
	FindClose(h);
	RemoveDirectoryW(from.c_str());
	return ok;
}

static void DeleteTree(const std::wstring &dir)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			std::wstring name = fd.cFileName, path = dir + L"\\" + name;
			if (name == L"." || name == L"..") {
				continue;
			}
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteTree(path);
			} else {
				DeleteFileW(path.c_str());
			}
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}
	RemoveDirectoryW(dir.c_str());
}

void ForgetVoices(const std::wstring &gameDir)
{
	DeleteFileW(VoicesFile(gameDir).c_str());
	DeleteTree(OriginalsDir(gameDir));
}

bool RestoreOriginals(const std::wstring &gameDir, std::wstring &failed)
{
	DeleteFileW(VoicesFile(gameDir).c_str());
	return RestoreFolder(OriginalsDir(gameDir), gameDir, failed);
}
