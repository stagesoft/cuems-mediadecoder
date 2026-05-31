<!--
SPDX-FileCopyrightText: 2026 Stagelab Coop SCCL
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
This project does not yet publish version tags or a `project(VERSION)` in CMake; entries
below follow git history on `main` until semantic versioning is introduced.

## Unreleased — development on `main`

Ongoing integration work toward shared use by **cuems-videocomposer** and **cuems-audioplayer**.

### Added

- **Initial module** (`c1bb945`) — `MediaFileReader`, `VideoDecoder`, `AudioDecoder`, and `FFmpegUtils` as a CMake static library (`cuems-mediadecoder`) with FFmpeg API v2 (`codecpar` + `avcodec_alloc_context3`). Supports local files, network URLs (RTSP, HTTP, UDP, etc.), and device inputs via an explicit format name (for example `v4l2`). Coordinated multi-stream seeking via `seekStreamsToTime()`.
- **FFmpeg 4.0+ compatibility** (`286f391`) — Version-guarded APIs for `AVInputFormat` constness, flush via `avcodec_flush_buffers()`, and libavcodec channel-layout changes.
- **Audio channel layout handling** (`f83ce02`) — `AudioDecoder::createSwrContextExplicit()` handles unknown input channel layouts and supports downmixing/resampling to float via libswresample, with separate code paths for FFmpeg 4.x/5.0 and 5.1+ (`swr_alloc_set_opts2`).
- **Multi-threaded decoding** (`740e733`) — Software video and audio decoders enable FFmpeg frame/slice threading where supported (`thread_count = 0`, auto-detect).
- **LGPL legal headers** (`9ea1469`) — SPDX `LGPL-3.0-or-later` notices and copyright attribution on all public headers and implementation files.

### Changed

- **AV1 threading** (`6d09b0e`) — `VideoDecoder::openCodec()` forces `thread_count = 1` for `AV_CODEC_ID_AV1` only (libdav1d seek/flush issues); other codecs retain full frame/slice threading.

### Notes

- No automated test suite or CI workflow exists in this repository yet (see README *Future developments*).
- `isLiveStream()` / `isSeekable()` are described in `LIVE_STREAM_EVALUATION.md` but are **not** present in the public API; callers should infer seekability from `getDuration()` and demuxer behaviour until those helpers are added.
