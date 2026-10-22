<!-- SPDX-FileContributor: Ion Reguera <ion@stagelab.coop> -->

# cuems-mediadecoder

Part of the **CUEMS** ecosystem — see the [`cuems-RELATIONS`](https://github.com/stagesoft/cuems-RELATIONS) repo for the system index, architecture diagram, and protocol/port map.

## Role

Shared FFmpeg demuxing/decoding library for the CUEMS C++ media players. C++17, LGPL-3.0. Added as a **git submodule** (notably inside `cuems-audioplayer`, which uses it to extract/decode audio from video files). Provides `MediaFileReader`, `VideoDecoder`, `AudioDecoder`, and `FFmpegUtils`; supports hardware video decode and timecode-synced seeking. System deps: `libavformat`, `libavcodec`, `libavutil`, `libswresample`.

The repo's own `README.md` carries the full API reference (namespace, per-class docs, CMake integration, compile-time options, return codes). This file only records CUEMS-ecosystem context. Build it via the parent player's `git submodule update --init --recursive`.

## Gotchas

- **Never pass an uninitialised `AVChannelLayout` as the destination of `av_channel_layout_copy()`.** The copy
  `av_channel_layout_uninit()`s its destination first, so stack garbage that reads as `AV_CHANNEL_ORDER_CUSTOM`
  gets its bogus `u.map` `free()`d. Until `8f5c3c6` (2026-10-21), `AudioDecoder::createSwrContextExplicit()` did
  exactly that with `inChLayout`: every cuems-audioplayer release from at least 0.0.3-9 to 0.0.3-12 aborted at arm on
  any AudioCue whose decoder reports a known channel layout (MP3, Vorbis, Opus, FLAC, ALAC, AAC, PCM in MOV, audio in
  MP4/MKV); PCM in WAV/AVI/MKV reports `UNSPEC` and was immune. Fixed by `= {}`; the player hotfix 0.0.3-13 pins it.
- **The regression test lives in cuems-audioplayer** (`test/test_mediadecoder_layout.cpp`, a dirty-stack death
  test with embedded fixtures). There is no CI and `debian/rules` builds the player with tests off: run
  `audioplayer_tests` and the 50-arm matrix (`cuems-RELATIONS/baselines/hotfix-0.0.3-13-chlayout/`) on .111 before
  any player release or mediadecoder pin bump.
- **The player's `.gitmodules` says `branch = ffmpeg-4.0`** (`740e733`, without the fix): `git submodule update
  --remote` would silently move the pin to it. Bump pins explicitly.
- The videocomposer also pins this repo but uses only `MediaFileReader`, `VideoDecoder` and `FFmpegUtils`, never
  `AudioDecoder`.

