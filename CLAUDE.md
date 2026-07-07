# cuems-mediadecoder

Part of the **CUEMS** ecosystem — see the [`cuems-RELATIONS`](https://github.com/stagesoft/cuems-RELATIONS) repo for the system index, architecture diagram, and protocol/port map.

## Role

Shared FFmpeg demuxing/decoding library for the CUEMS C++ media players. C++17, LGPL-3.0. Added as a **git submodule** (notably inside `cuems-audioplayer`, which uses it to extract/decode audio from video files). Provides `MediaFileReader`, `VideoDecoder`, `AudioDecoder`, and `FFmpegUtils`; supports hardware video decode and timecode-synced seeking. System deps: `libavformat`, `libavcodec`, `libavutil`, `libswresample`.

The repo's own `README.md` carries the full API reference (namespace, per-class docs, CMake integration, compile-time options, return codes). This file only records CUEMS-ecosystem context; there are no CUEMS-specific gotchas beyond building it via the parent player's `git submodule update --init --recursive`.
