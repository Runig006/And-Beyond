// Installer texts: the original installer's (SETUP\BURST\STRINGS\<language>.APS, from the CD) and our own.
#pragma once
#include <string>
#include <vector>

enum TextId {
	T_TITLE,
	T_PICK_ISO,
	T_PICK_ISO_OR_GAME,
	T_ISO_OR_GAME_FILTER,
	T_NOT_OURS,
	T_ISO_FILTER,
	T_BAD_ISO,
	T_OTHER_VERSION,
	T_NEEDS_DISC,
	T_PICK_FOLDER,
	T_FOLDER_NOTE,
	T_INSTALL_HERE,
	T_CONFIRM_DIR,
	T_INSTALLING,
	T_INSTALLED,
	T_COPY_FAILED,
	T_TRANSLATION,
	T_ASK_LINKS,
	T_ASK_VOICES,
	T_VOICES_NOTE,
	T_FROM_DISC,
	T_RESTORE,
	T_PICK_DISC,
	T_DISC_FILTER,
	T_BAD_DISC,
	T_NO_FFMPEG,
	T_CONVERTING,
	T_VOICES_DONE,
	T_RESTORED,
	T_CONVERT_FAILED,
	T_NO_PATCH,
	T_OLD_PATCH,
	T_INFO_SETUP,
	T_INFO_INSTALLED,
	T_INFO_CD,
	T_UPDATE,
	T_UPDATED,
	T_INFO_CONTENT,
	T_INFO_EXE,
	T_LANG_EN,
	T_LANG_ES,
	T_LANG_IT,
	T_LANG_PT,
	T_LANG_FR,
	T_LANG_DE,
	T_LANG_OTHER,
	T_HELP_LINK,
	T_GAME_HELP_LINK,
	T_DISCLAIMER,
	T_COUNT
};

// The installer's language: chosen at the start, Windows' until then. 0 en, 1 es, 2 fr, 3 de, 4 it, 5 pt.
enum { INSTALLER_LANGUAGES = 6 };
int WindowsLanguage();
void SetLanguage(int lang);
int CurrentLanguage();
const char *LanguageCodeOf(int lang);  // "en", "es"...
int LanguageFromCode(const char *code);  // -1 if unknown
const wchar_t *LanguageOwnName(int lang);  // "Español", written in itself
const wchar_t *LanguageName(int lang);     // its name in the installer's language ("other" if unknown)

// Our own text in the installer's language
const wchar_t *Tr(TextId id);

// The original installer's .APS file for the installer's language ("SPANISH.APS"; "USA.APS" for English)
const char *ApsName();

// Key=value strings of an .APS file (Windows-1252)
class ApsStrings {
public:
	void Load(const std::vector<unsigned char> &data);
	std::wstring Get(const char *key, const wchar_t *fallback) const;

private:
	std::vector<std::pair<std::string, std::wstring>> items_;
};
