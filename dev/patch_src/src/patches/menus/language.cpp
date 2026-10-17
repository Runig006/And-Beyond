// The game's texts in another language. Tables: lang\<code>.json next to buzz.exe (dev\lang): key = an English text
// of the PC buzz.exe, value = its translation (nothing else). Each place of buzz.exe that uses one of those texts
// (lang_sites.h) is pointed at the translation, and back at the English string for
// English. A place something else has patched since is left alone. Accents and the
// inverted question and exclamation marks are dropped: the PC fonts have none.
#include "common.h"
#include "config.h"
#include "lang_sites.h"
#include "language.h"
#include "texts.h"
#include <stdio.h>
#include <map>
#include <string>
#include <vector>

// The languages of the tables; 'name' as the game's font writes it (lower case, no accents)
static const struct {
	const char *code;
	const char *name;
} kLanguages[] = {
	{"en", "english"},
	{"es", "espanol"},
	{"fr", "francais"},
	{"de", "deutsch"},
	{"it", "italiano"},
	{"pt", "portugues"},
};
static const int kCodeCount = sizeof kLanguages / sizeof kLanguages[0];

// The places of the running buzz.exe (lang_sites.h); none for a version without a list
static const LangSite *g_sites;
static int g_siteCount;
static std::vector<unsigned> g_original; // what buzz.exe had at each place
static std::vector<unsigned> g_written;  // what we left there last
static int g_current = 0;                // index into kLanguages
static std::vector<int> g_available;    // languages with a table (English always)

// --- The tables: just what they use of JSON (objects and strings)

struct JsonValue {
	std::string text;
	std::vector<std::pair<std::string, JsonValue>> members;
};

static void SkipSpace(const char *&p)
{
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
		p++;
	}
}

static void PutUtf8(std::string &out, unsigned c)
{
	if (c < 0x80) {
		out += (char)c;
	} else if (c < 0x800) {
		out += (char)(0xc0 | c >> 6);
		out += (char)(0x80 | (c & 0x3f));
	} else {
		out += (char)(0xe0 | c >> 12);
		out += (char)(0x80 | (c >> 6 & 0x3f));
		out += (char)(0x80 | (c & 0x3f));
	}
}

static bool ParseString(const char *&p, std::string &out)
{
	if (*p != '"') {
		return false;
	}
	for (p++; *p && *p != '"'; p++) {
		if (*p != '\\') {
			out += *p;
			continue;
		}
		p++;
		switch (*p) {
		case 'n':
			out += '\n';
			break;
		case 't':
			out += '\t';
			break;
		case 'u': {
			unsigned c = 0;
			if (sscanf(p + 1, "%4x", &c) == 1) {
				PutUtf8(out, c);
				p += 4;
			}
			break;
		}
		default:
			out += *p;
			break;
		}
	}
	if (*p != '"') {
		return false;
	}
	p++;
	return true;
}

static bool ParseValue(const char *&p, JsonValue &out, int depth)
{
	SkipSpace(p);
	if (*p == '"') {
		return ParseString(p, out.text);
	}
	if (*p != '{' || depth > 8) {
		// numbers, true, false, null: not in the tables; skipped up to the next separator
		while (*p && *p != ',' && *p != '}') {
			p++;
		}
		return *p != 0;
	}
	for (p++;;) {
		SkipSpace(p);
		if (*p == '}') {
			p++;
			return true;
		}
		std::pair<std::string, JsonValue> member;
		if (!ParseString(p, member.first)) {
			return false;
		}
		SkipSpace(p);
		if (*p++ != ':' || !ParseValue(p, member.second, depth + 1)) {
			return false;
		}
		out.members.push_back(member);
		SkipSpace(p);
		if (*p == ',') {
			p++;
		}
	}
}


// UTF-8 to the game's characters: letters without accents, no inverted question or exclamation mark; in German
// the umlauts as ae, oe, ue
static std::string GameText(const std::string &utf8, bool german)
{
	static const char kLatin1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPsaaaaaaaceeeeiiiidnooooo/ouuuuypy";  // 0xc0..0xff
	std::string out;
	for (size_t i = 0; i < utf8.size();) {
		unsigned char c = (unsigned char)utf8[i];
		unsigned code = c;
		int len = 1;
		if (c >= 0xe0 && i + 2 < utf8.size()) {
			code = (c & 0x0f) << 12 | (utf8[i + 1] & 0x3f) << 6 | (utf8[i + 2] & 0x3f);
			len = 3;
		} else if (c >= 0xc0 && i + 1 < utf8.size()) {
			code = (c & 0x1f) << 6 | (utf8[i + 1] & 0x3f);
			len = 2;
		}
		i += len;
		if (code < 0x80) {
			out += (char)code;
		} else if (german && (code == 0xe4 || code == 0xf6 || code == 0xfc || code == 0xc4 || code == 0xd6 ||
		                      code == 0xdc)) {
			out += kLatin1[code - 0xc0];
			out += 'e';
		} else if (code >= 0xc0 && code <= 0xff) {
			out += kLatin1[code - 0xc0];
			if (code == 0xdf) {
				out += 's';  // sharp s: ss
			}
		} else if (code != 0xbf && code != 0xa1) {  // inverted ? and ! go
			out += '?';
		}
	}
	return out;
}

// English text -> translation, kept for as long as the game runs (it may hold the pointers)
static std::map<std::string, const char *> LoadTable(const char *code)
{
	std::map<std::string, const char *> table;
	char path[MAX_PATH];
	wsprintfA(path, "%slang\\%s.json", g_exeDir, code);
	FILE *f = fopen(path, "rb");
	if (!f) {
		return table;
	}
	std::string data;
	char buf[4096];
	size_t got;
	while ((got = fread(buf, 1, sizeof buf, f)) > 0) {
		data.append(buf, got);
	}
	fclose(f);
	JsonValue root;
	const char *p = data.c_str();
	if (data.compare(0, 3, "\xef\xbb\xbf") == 0) {
		p += 3;
	}
	if (!ParseValue(p, root, 0)) {
		LogLine("ERROR language: %s is not a table", path);
		return table;
	}
	for (const auto &m : root.members) {
		if (m.second.text.empty()) {
			continue;
		}
		std::string text = GameText(m.second.text, !strcmp(code, "de"));
		// Room to spare: the level select writes the number into "mission 1"
		char *copy = new char[text.size() + 8]();
		memcpy(copy, text.c_str(), text.size());
		table[m.first] = copy;
	}
	return table;
}

// English in another version: a copy of the list's text, which the game may write into like its own (the level
// select puts the number into "mission 1"); the list's is read-only
static const char *EnglishText(const char *text)
{
	static std::map<std::string, const char *> copies;
	auto it = copies.find(text);
	if (it != copies.end()) {
		return it->second;
	}
	size_t len = strlen(text);
	char *copy = new char[len + 8]();
	memcpy(copy, text, len);
	copies[text] = copy;
	return copy;
}

static bool HasTable(int lang)
{
	char path[MAX_PATH];
	wsprintfA(path, "%slang\\%s.json", g_exeDir, kLanguages[lang].code);
	return lang == 0 || GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

static void WriteSite(unsigned site, unsigned value)
{
	DWORD old;
	if (VirtualProtect((void *)site, 4, PAGE_EXECUTE_READWRITE, &old)) {
		*(unsigned *)site = value;
		VirtualProtect((void *)site, 4, old, &old);
	}
}

static void Apply(int lang)
{
	std::map<std::string, const char *> table;
	// Its own language: the buzz.exe's texts as they are (the official ones). English in another version: the
	// English text of the list (that buzz.exe has none).
	bool own = !strcmp(kLanguages[lang].code, RunningGameName());
	if (!own && lang != 0) {
		table = LoadTable(kLanguages[lang].code);
	}
	int changed = 0;
	for (int i = 0; i < g_siteCount; i++) {
		const LangSite &s = g_sites[i];
		if (*(unsigned *)s.site != g_written[i]) {
			continue;  // patched by something else since
		}
		unsigned value = g_original[i];
		const char *text = nullptr;
		if (!own && !*s.text) {
			// The second line of a text this buzz.exe writes in two: the whole translation is on the first
			text = EnglishText("");
		} else if (!own) {
			auto it = table.find(s.text);
			text = it != table.end() ? it->second : lang == 0 ? EnglishText(s.text) : nullptr;
		}
		if (text) {
			// Some places point inside the string (" rotational" read from its second letter); a buzz.exe already
			// translated points elsewhere: from the start
			unsigned offset = g_original[i] - s.string;
			unsigned len = (unsigned)strlen(text);
			if (offset > strlen((const char *)s.string)) {
				offset = 0;
			}
			value = (unsigned)text + (offset < len ? offset : len);
			changed++;
		}
		WriteSite(s.site, value);
		g_written[i] = value;
	}
	g_current = lang;
	LogLine("language: %s (%d places translated)", kLanguages[lang].code, changed);
}

// The first word of a file of setup\ that names a language, if there is a table for it; -1 if not
static int LanguageInFile(const char *name)
{
	char path[MAX_PATH], code[8] = {};
	wsprintfA(path, "%ssetup\\%s", g_exeDir, name);
	FILE *f = fopen(path, "rb");
	if (!f) {
		return -1;
	}
	fscanf(f, "%7s", code);
	fclose(f);
	for (int l = 0; l < kCodeCount; l++) {
		if (!strcmp(code, kLanguages[l].code) && HasTable(l)) {
			return l;
		}
	}
	return -1;
}

// Automatic: the language chosen in the installer (setup\language.txt), else that of the voices and videos it put
// (setup\voices.txt: "es 1"), else the buzz.exe's own
static int AutoLanguage()
{
	int l = LanguageInFile("language.txt");
	if (l < 0) {
		l = LanguageInFile("voices.txt");
	}
	for (int k = 0; l < 0 && k < kCodeCount; k++) {
		if (!strcmp(kLanguages[k].code, RunningGameName())) {
			l = k;
		}
	}
	return l < 0 ? 0 : l;
}

// The player's choice in PATCH OPTIONS; until there is one, the installer's language or the voices'
static int Chosen()
{
	int l = g_config.language - 1;  // 0 = none chosen yet
	return l >= 0 && l < kCodeCount && HasTable(l) ? l : AutoLanguage();
}

extern "C" void LanguageInit(void)
{
	g_sites = nullptr;
	g_siteCount = 0;
	for (const LangVersion &v : kLangVersions) {
		if (v.game == RunningGame()) {
			g_sites = v.sites;
			g_siteCount = v.count;
		}
	}
	g_original.resize(g_siteCount);
	g_written.resize(g_siteCount);
	for (int i = 0; i < g_siteCount; i++) {
		g_original[i] = g_written[i] = *(unsigned *)g_sites[i].site;
	}
	g_available.clear();
	for (int l = 0; l < kCodeCount; l++) {
		if (HasTable(l)) {
			g_available.push_back(l);
		}
	}
	Apply(Chosen());
}

extern "C" const char *LanguageCode(void)
{
	return kLanguages[g_current].code;
}

int LanguageChoiceCount()
{
	return (int)g_available.size();
}

int LanguageChoice()
{
	for (size_t i = 0; i < g_available.size(); i++) {
		if (g_available[i] == g_current) {
			return (int)i;
		}
	}
	return 0;
}

void RefreshPatchTexts(); // menu.cpp: our texts the game keeps a pointer to

void SetLanguageChoice(int choice)
{
	if (choice < 0 || choice >= (int)g_available.size()) {
		return;
	}
	g_config.language = g_available[choice] + 1;
	Apply(Chosen());
	RefreshPatchTexts();
}

const char *LanguageChoiceName(int choice)
{
	return choice >= 0 && choice < (int)g_available.size() ? kLanguages[g_available[choice]].name : "";
}
