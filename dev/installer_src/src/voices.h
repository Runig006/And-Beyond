// "Translation" button: voices and videos of the installed PC game from another disc: a dubbed console one
// (Dreamcast .cdi or PlayStation .bin) or the PC CD in another language. The originals
// are kept in setup\original to go back to them.
#pragma once
#include <string>

#include "iso.h"
#include "setup.h"

enum DiscKind { DISC_NONE, DISC_DREAMCAST, DISC_PLAYSTATION, DISC_PC };

DiscKind IdentifyDisc(IsoImage &disc);
std::wstring FindFfmpeg(const std::wstring &gameDir);  // "" if there is none
bool HasOriginals(const std::wstring &gameDir);
// Forgets a translation (setup\\voices.txt and setup\\original): installing again brings the PC files back
void ForgetVoices(const std::wstring &gameDir);
bool ApplyVoices(IsoImage &disc, DiscKind kind, const std::wstring &gameDir, const std::wstring &ffmpeg,
	ProgressFn progress, void *ctx, std::wstring &failed);
bool RestoreOriginals(const std::wstring &gameDir, std::wstring &failed);
// Voices and videos put by ApplyVoices: their language ("es", "it"...; "?") and disc; false = the game's own
bool VoicesFrom(const std::wstring &gameDir, std::string *language, DiscKind *kind);
