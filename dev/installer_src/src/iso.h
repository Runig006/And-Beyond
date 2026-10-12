// CD images: the PC game's .iso (2048-byte sectors), .bin (2352, MODE1 or MODE2: also the PlayStation one) or the
// .cue that names the .bin, and the Dreamcast .cdi (DiscJuggler). Files by path in capitals with backslashes
// ("SETUP\\MEDIA\\BURSTBG.BMP"). Uses the Joliet names when the disc has them: the plain ISO9660 ones are 8.3
// ("FIXER_~1.DLL") and the game opens the long ones ("fixer_out.dll").
#pragma once
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

struct IsoEntry {
	uint32_t lba;
	uint32_t size;
	std::wstring name;  // path as written on the disc
};

class IsoImage {
public:
	~IsoImage();
	bool Open(const std::wstring &path);
	bool Has(const std::string &path) const;
	bool Read(const std::string &path, std::vector<uint8_t> &out);
	// Whole 2352-byte sectors of a file (.bin only): PlayStation XA audio and STR videos are read this way
	bool ReadRaw(const std::string &path, std::vector<uint8_t> &out);
	const std::map<std::string, IsoEntry> &Files() const { return files_; }

private:
	bool OpenCdi();
	bool ReadSectors(uint32_t lba, uint32_t count, uint8_t *out);
	void Walk(uint32_t lba, uint32_t size, const std::string &key, const std::wstring &name, bool joliet, int depth);

	FILE *f_ = nullptr;
	int sectorSize_ = 2048;
	int dataOffset_ = 0;
	int64_t base_ = 0;      // file position of sector firstLba_
	uint32_t firstLba_ = 0; // the Dreamcast data track starts at sector 45000
	std::map<std::string, IsoEntry> files_;
};
