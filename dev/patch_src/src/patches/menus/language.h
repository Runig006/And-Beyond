// The game's texts in another language (language.cpp): lang\<code>.json next to buzz.exe, chosen in PATCH OPTIONS
// or, by default, the one chosen in the installer (setup\language.txt) or that of the voices and videos it put.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
void LanguageInit(void);
const char *LanguageCode(void);  // the language in use: "en", "es", "fr", "de", "it", "pt"
#ifdef __cplusplus
}
#endif

// PATCH OPTIONS > language: each language there is a table for (English always)
int LanguageChoiceCount();
int LanguageChoice();
void SetLanguageChoice(int choice);
const char *LanguageChoiceName(int choice);
