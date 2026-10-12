// The game's Windows help (BUZZAG.HLP, WinHelp 4) as one HTML page with the pictures inside (BMP in base64): Windows
// 10/11 no longer open .hlp files. The result is the whole file (UTF-8, CRLF line endings), written as is.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// False if the data is not a WinHelp 4 file this can read
bool HlpToHtml(const std::vector<uint8_t> &hlp, std::string &html);
