#!/bin/bash
# Builds the small ffmpeg.exe the installer carries for its "Translation" button: only what it uses (reading ADX,
# PlayStation XA / STR, WAV and MPEG program streams; writing WAV, MP2 and MPEG-1), no outside libraries, LGPL.
# Run it in the MSYS2 MINGW64 shell, with: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm make diffutils
# It leaves in build\: ffmpeg.exe, ffmpeg-LICENSE.txt (LGPL 2.1) and ffmpeg-README.txt (version, settings, source).
set -e
VERSION=9.0.2
SHA256=8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$HERE/build/ffmpeg"
mkdir -p "$WORK"
cd "$WORK"
TARBALL="ffmpeg-$VERSION.tar.xz"
if [ ! -f "$TARBALL" ]; then
	curl -sSfLO "https://ffmpeg.org/releases/$TARBALL"
fi
echo "$SHA256 *$TARBALL" | sha256sum -c -
rm -rf "ffmpeg-$VERSION"
tar -xf "$TARBALL"
cd "ffmpeg-$VERSION"

CONFIG=(
	--disable-everything --disable-autodetect --disable-network --disable-doc --disable-debug
	--disable-ffplay --disable-ffprobe --disable-avdevice
	--enable-static --disable-shared --extra-ldflags=-static
	--enable-protocol=file,pipe
	--enable-demuxer=adx,str,wav,mpegps,mpegvideo
	--enable-decoder=adpcm_adx,adpcm_xa,adpcm_ms,adpcm_ima_wav,pcm_s16le,pcm_u8,mdec,mpeg1video,mpeg2video,mp2
	--enable-parser=adx,mpegvideo,mpegaudio
	--enable-encoder=pcm_s16le,mp2,mpeg1video
	--enable-muxer=wav,mpeg1system,null
	--enable-filter=scale,fps,aresample,aformat,format,null,anull
	--enable-swscale --enable-swresample
)
./configure "${CONFIG[@]}"
make -j"$(nproc)" ffmpeg.exe
strip ffmpeg.exe

OUT="$HERE/build"
cp ffmpeg.exe "$OUT/ffmpeg.exe"
cp COPYING.LGPLv2.1 "$OUT/ffmpeg-LICENSE.txt"
{
	echo "ffmpeg $VERSION, built for the installer of the ...& Beyond patch."
	echo
	echo "License: GNU LGPL 2.1 or later (ffmpeg-LICENSE.txt). No outside libraries."
	echo "Source: https://ffmpeg.org/releases/$TARBALL (unchanged; SHA-256 $SHA256)"
	echo "Also attached to the patch's release on GitHub."
	echo
	echo "Built with MSYS2 MINGW64 (gcc) and this configuration:"
	echo "./configure ${CONFIG[*]}"
} > "$OUT/ffmpeg-README.txt"
echo "ffmpeg.exe: $(stat -c %s "$OUT/ffmpeg.exe") bytes"
