<!--
SPDX-FileCopyrightText: 2026 Stagelab Coop SCCL
SPDX-License-Identifier: GPL-3.0-or-later
-->

# cuems-mediadecoder

**Current release: development (no version tag or `project(VERSION)` in CMake yet)** — see [CHANGELOG.md](./CHANGELOG.md).

**Shared FFmpeg demuxing and decoding library for CueMS video and audio players.**

[![License: LGPL v3](https://img.shields.io/badge/License-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://github.com/stagesoft/cuems-mediadecoder)

## Table of Contents

| Section | Subsections |
|---|---|
| [Overview](#overview) | |
| [Architecture](#architecture) | [Module layout](#module-layout) · [Data and control flow](#data-and-control-flow) · [Key data structures](#key-data-structures) · [Threading and process model](#threading-and-process-model) |
| [Core Concepts](#core-concepts) | |
| [Design Goals](#design-goals) | |
| [API documentation](#api-documentation) | [Namespace](#namespace) · [MediaFileReader](#mediafilereader) · [VideoDecoder](#videodecoder) · [AudioDecoder](#audiodecoder) · [FFmpegUtils](#ffmpegutils) · [CMake integration](#cmake-integration) · [Compile-time options](#compile-time-options) · [Return codes and errors](#return-codes-and-errors) |
| [Installation](#installation) | [System dependencies](#system-dependencies) · [Build from source](#build-from-source) · [Embed in a parent CMake project](#embed-in-a-parent-cmake-project) |
| [Usage](#usage) | [Minimal file decode loop](#minimal-file-decode-loop) · [Hardware video decode](#hardware-video-decode) · [Timecode-synced seeking](#timecode-synced-seeking) · [Live streams and devices](#live-streams-and-devices) |
| [Development](#development) | |
| [Contributors](#contributors) | |
| [Release notes](#release-notes) | |
| [Future developments](#future-developments) | [Automated tests](#automated-tests) · [CI tests and coverage](#ci-tests-and-coverage) · [API documentation site](#api-documentation-site) · [Packaging and distribution](#packaging-and-distribution) |
| [Copyright notice](#copyright-notice) | |
| [License](#license) | |

* **Source / issues:** [stagesoft/cuems-mediadecoder](https://github.com/stagesoft/cuems-mediadecoder) on GitHub
* **Consumers:** [cuems-videocomposer](https://github.com/stagesoft/cuems-videocomposer), [cuems-audioplayer](https://github.com/stagesoft/cuems-audioplayer) (integration via submodule / `add_subdirectory`)

`cuems-mediadecoder` is a **C++17 static library** that centralises FFmpeg demuxing and
decoding logic shared by the CueMS (Cue Management System) **video** and **audio** players.
It exposes thin wrappers around `libavformat` / `libavcodec` using the modern FFmpeg API
(`AVCodecParameters` + `avcodec_alloc_context3`), while leaving show-level concerns — MTC
sync, hardware device enumeration, GPU upload, and high-quality resampling — to the parent
applications.

It is composed of:

* **`cuems_mediadecoder` (static library)** — four translation units built from `src/` with
  public headers under `include/cuems_mediadecoder/`.
* **`MediaFileReader`** — open files, URLs, or capture devices; discover streams; read
  packets; seek (including coordinated A/V seek).
* **`VideoDecoder`** — decode video packets to `AVFrame`; optional hardware path; libswscale
  helpers.
* **`AudioDecoder`** — decode audio packets (software only); libswresample helpers targeting
  float PCM.
* **`FFmpegUtils`** — human-readable FFmpeg error strings.

There is **no daemon**, **no CLI**, and **no network/IPC API** in this repository — the
external surface is the C++ headers plus the CMake target `cuems-mediadecoder`.

[↑ Back to Table of Contents](#table-of-contents)

---

## Overview

Typical embedding in a timecode-driven player:

```text
  Timecode / show logic (parent app)
           │
           ▼
  ┌────────────────────┐
  │  MediaFileReader   │  avformat_open_input / av_read_frame
  │  (demux)           │  seekStreamsToTime() on cue
  └─────────┬──────────┘
            │ AVPacket* (per stream index)
     ┌──────┴──────┐
     ▼             ▼
┌──────────┐  ┌──────────┐
│ Video    │  │ Audio    │  avcodec_send_packet / receive_frame
│ Decoder  │  │ Decoder  │
└────┬─────┘  └────┬─────┘
     │ AVFrame     │ AVFrame
     ▼             ▼
 sws_scale     swr_convert  (helpers on decoder classes)
     │             │
     ▼             ▼
  GPU / texture   Mixer / libsoxr (parent app)
```

* **Demux once** — `MediaFileReader` owns the `AVFormatContext` and delivers compressed
  packets.
* **Decode per medium** — `VideoDecoder` and `AudioDecoder` are independent instances;
  the caller routes packets by `stream_index`.
* **No internal clock** — the library does not pace playback; parents drive reads and
  seeks from MTC or UI.
* **Static linking** — one `.a` per build tree, matching other CueMS native modules (for
  example `mtcreceiver`).

[↑ Back to Table of Contents](#table-of-contents)

---

## Architecture

### Module layout

| Path | Role |
|---|---|
| `include/cuems_mediadecoder/MediaFileReader.h` | Public demuxer API |
| `include/cuems_mediadecoder/VideoDecoder.h` | Public video decoder API |
| `include/cuems_mediadecoder/AudioDecoder.h` | Public audio decoder API |
| `include/cuems_mediadecoder/FFmpegUtils.h` | Public utilities |
| `src/MediaFileReader.cpp` | `avformat_*` wrapper implementation |
| `src/VideoDecoder.cpp` | `avcodec_*` video path + `sws_*` helpers |
| `src/AudioDecoder.cpp` | `avcodec_*` audio path + `swr_*` helpers |
| `src/FFmpegUtils.cpp` | `av_strerror` wrapper |
| `CMakeLists.txt` | `cuems-mediadecoder` static target, pkg-config FFmpeg deps |

#### Class `MediaFileReader`

| Member | Role |
|---|---|
| **`MediaFileReader()`** | Construct closed reader. |
| **`~MediaFileReader()`** | Close and release `AVFormatContext`. |
| **`open(path, format)`** | Open file path, URL, or device; optional `av_find_input_format` name (for example `"v4l2"`). |
| **`close()`** | `avformat_close_input` and reset state. |
| **`isReady()`** | `true` when `avformat_find_stream_info` succeeded. |
| **`findStream(mediaType)`** | Return first stream index for `AVMEDIA_TYPE_*`, or `-1`. |
| **`getCodecParameters(streamIndex)`** | `AVCodecParameters*` for decoder setup. |
| **`getStream(streamIndex)`** | `AVStream*` for time bases and metadata. |
| **`seek(timestamp, streamIndex, flags)`** | `av_seek_frame` in stream time_base units. |
| **`seekToTime(timeSeconds, streamIndex, flags)`** | Seek one stream by seconds. |
| **`seekStreamsToTime(timeSeconds, streamIndices, flags)`** | Seek multiple streams to the same wall time (A/V sync). |
| **`readPacket(packet)`** | `av_read_frame`; returns `0` on success, `< 0` on error/EOF. |
| **`getDuration()`** | Container duration in seconds, or `0.0` if unknown. |
| **`getFormatContext()`** | Expose `AVFormatContext*` for advanced demuxer options. |
| **`getStreamCount()`** | `nb_streams` or `0` if closed. |

Copy construction and assignment are deleted.

#### Class `VideoDecoder`

| Member | Role |
|---|---|
| **`VideoDecoder()`** | Construct closed decoder. |
| **`~VideoDecoder()`** | Free owned `AVCodecContext`. |
| **`openCodec(codecParams, codecContext)`** | Open software decoder, or adopt a caller-provided context (hardware). |
| **`openHardwareCodec(codecParams, hwDeviceCtx, hwPixFmt)`** | Allocate context, attach `hw_device_ctx`, set `get_format` callback. |
| **`close()`** | Release context if allocated by this class. |
| **`isReady()`** | `true` when `codecCtx_` is non-null. |
| **`sendPacket(packet)`** | `avcodec_send_packet` (`nullptr` flushes). |
| **`receiveFrame(frame)`** | `avcodec_receive_frame`. |
| **`flush()`** | `avcodec_flush_buffers`. |
| **`getCodecContext()`** | Expose `AVCodecContext*`. |
| **`isHardwareDecoding()`** | `true` when `hw_device_ctx` was set. |
| **`getVideoProperties(width, height, pixFmt)`** | Read geometry and pixel format from open context. |
| **`createSwsContext(...)`** | Allocate bilinear `SwsContext`. |
| **`freeSwsContext(swsCtx)`** | `sws_freeContext`. |

Software `openCodec` enables FFmpeg frame/slice threading except for **AV1**, where
`thread_count = 1` avoids libdav1d seek/flush issues.

#### Class `AudioDecoder`

| Member | Role |
|---|---|
| **`AudioDecoder()`** | Construct closed decoder. |
| **`~AudioDecoder()`** | Free `AVCodecContext`. |
| **`openCodec(codecParams)`** | Software audio decoder only (`avcodec_find_decoder`). |
| **`close()`** | Release context. |
| **`isReady()`** | Context allocated and opened. |
| **`sendPacket(packet)`** | `avcodec_send_packet`. |
| **`receiveFrame(frame)`** | `avcodec_receive_frame`. |
| **`flush()`** | `avcodec_flush_buffers`. |
| **`getCodecContext()`** | Expose context. |
| **`getAudioProperties(channels, sampleRate, sampleFmt)`** | Query open codec parameters (FFmpeg 59+ channel layout aware). |
| **`createSwrContext(outChannels, outSampleRate)`** | Build resampler to float; `-1` keeps input rate/channel count. |
| **`createSwrContextExplicit(outChannels, outSampleRate, outSampleFmt)`** | Full control over output format; handles unknown input channel layouts. |
| **`freeSwrContext(swrCtx)`** | `swr_free`. |
| **`convertSamples(swrCtx, out, outCount, in)`** | `swr_convert` wrapper. |

#### Class `FFmpegUtils`

| Member | Role |
|---|---|
| **`getErrorString(errnum)`** | Map negative FFmpeg error code to `std::string` via `av_strerror`. |

### Data and control flow

1. **Open** — Parent calls `MediaFileReader::open`. FFmpeg probes streams (`avformat_find_stream_info`).
2. **Discover** — `findStream(AVMEDIA_TYPE_VIDEO)` / `AUDIO`; pass `getCodecParameters(i)` to decoders.
3. **Decode loop** — `readPacket` → if `packet->stream_index` matches video, `VideoDecoder::sendPacket` / `receiveFrame` (drain with `EAGAIN` as usual for FFmpeg); same for audio.
4. **Seek** — On cue jump, parent computes time in seconds → `seekStreamsToTime` → `flush()` on both decoders → resume reading.
5. **Convert** — Parent creates `SwsContext` / `SwrContext` from static helpers, converts frames for GPU or mixer.

### Key data structures

| Type | Owner | Notes |
|---|---|---|
| `AVFormatContext*` | `MediaFileReader` | Demuxer; freed on `close()`. |
| `AVCodecParameters*` | FFmpeg (via format context) | Borrowed; valid while reader is open. |
| `AVPacket*` | Caller | Must be `av_packet_alloc()`; filled by `readPacket`. |
| `AVCodecContext*` | `VideoDecoder` / `AudioDecoder` | Freed on `close()` unless externally supplied to `openCodec`. |
| `AVFrame*` | Caller | Filled by `receiveFrame`. |
| `SwsContext*` | Caller | Created/freed via `VideoDecoder` static helpers. |
| `SwrContext*` | Caller | Created/freed via `AudioDecoder` static helpers. |

### Threading and process model

| Topic | Behaviour |
|---|---|
| **Process model** | Library only — linked into player processes; no standalone binary. |
| **Instance threading** | No mutexes inside the library; **one thread per reader/decoder pair** unless the parent serialises access. |
| **FFmpeg internal threads** | Software decoders may spawn FFmpeg worker threads (`thread_count = 0` auto, except AV1 = 1). |
| **Hardware video** | Parent creates `AVBufferRef` hardware device; `openHardwareCodec` or pre-built `AVCodecContext` passed to `openCodec`. |
| **Live streams** | Continuous `readPacket` loop; seeking may fail at runtime — check `getDuration()` and demuxer capabilities; dedicated `isLiveStream()` / `isSeekable()` helpers are **not** implemented yet. |

[↑ Back to Table of Contents](#table-of-contents)

---

## Core Concepts

* **Demux vs decode** — `MediaFileReader` handles containers and compressed packets; decoders handle codec bitstreams.
* **FFmpeg API v2** — Stream discovery uses `codecpar`; decoders copy parameters with `avcodec_parameters_to_context`.
* **Send/receive discipline** — Modern `avcodec_send_packet` / `avcodec_receive_frame` loop; callers handle `AVERROR(EAGAIN)`.
* **Coordinated seek** — `seekStreamsToTime` aligns multiple stream indices to the same timestamp for frame-accurate A/V jumps.
* **Parent-owned sync** — MTC or engine logic outside this library decides *when* to read and seek; the library provides mechanisms only.
* **Optional conversion libs** — `libswscale` / `libswresample` are probed at configure time (`HAVE_SWSCALE`, `HAVE_SWRESAMPLE`); video/audio helper methods require them at link time.
* **Integration boundary** — GPU paths, HAP texture upload, and `libsoxr` remain in videocomposer/audioplayer by design.

[↑ Back to Table of Contents](#table-of-contents)

---

## Design Goals

* **Share FFmpeg code** between videocomposer and audioplayer without duplicating error-prone demuxer setup.
* **Stay close to FFmpeg** — thin wrappers, not a opaque media framework; advanced callers use `getFormatContext()` / `getCodecContext()`.
* **FFmpeg 4.x–6.x** — version guards for `AVInputFormat`, channel layouts, and `swr_alloc_set_opts2`.
* **Software audio only** — no hardware audio decode path in this module.
* **Flexible video hardware** — hardware device creation stays in the parent; this library accepts prepared contexts.
* **Timecode-friendly** — multi-stream seek and no internal playback clock.
* **Static library** — predictable embedding in CueMS native builds.
* **Prepared extension points** — comments in `VideoDecoder.h` reserve future HAP packet-level DXT extraction (not implemented).

[↑ Back to Table of Contents](#table-of-contents)

---

## API documentation

### Namespace

All public types live in namespace `cuems_mediadecoder`.

Headers (include with quotes or via `target_include_directories`):

```cpp
#include "cuems_mediadecoder/MediaFileReader.h"
#include "cuems_mediadecoder/VideoDecoder.h"
#include "cuems_mediadecoder/AudioDecoder.h"
#include "cuems_mediadecoder/FFmpegUtils.h"
```

There is **no** installed pkg-config file for `cuems-mediadecoder` itself — consumers depend on the CMake target.

### MediaFileReader

See [Architecture — MediaFileReader](#class-mediafilereader) for the full method list.

**`open(path, format)`**

| Argument | Semantics |
|---|---|
| `path` | Filesystem path, URL (`rtsp://`, `http://`, `udp://`, …), or device node. |
| `format` | Optional FFmpeg input short name; empty string lets FFmpeg auto-detect. |

Returns `true` on success, `false` on any `avformat_open_input` / `avformat_find_stream_info` failure.

### VideoDecoder

See [Architecture — VideoDecoder](#class-videodecoder).

**`openCodec(codecParams, codecContext)`** — When `codecContext` is non-null, the decoder adopts it (typical for parent-managed hardware). When null, allocates a software context, copies parameters, applies threading rules, and calls `avcodec_open2`.

**`openHardwareCodec(codecParams, hwDeviceCtx, hwPixFmt)`** — Sets `get_format` to select `hwPixFmt` from the codec's offered list.

### AudioDecoder

See [Architecture — AudioDecoder](#class-audiodecoder).

Resampling defaults to **`AV_SAMPLE_FMT_FLT`** in `createSwrContextExplicit`. Unknown input channel layouts are replaced with a default layout derived from the channel count (same strategy as documented for mpv-style playback).

### FFmpegUtils

**`getErrorString(int errnum)`** — Pass negative FFmpeg return values (for example from `readPacket` or `sendPacket`).

### CMake integration

**Target:** `cuems-mediadecoder` (STATIC)

**Public include directory:** `include/`

**Transitive link libraries:** `${FFMPEG_LIBRARIES}` and, when found, `${SWSCALE_LIBRARIES}` / `${SWRESAMPLE_LIBRARIES}`.

Example in a parent `CMakeLists.txt`:

```cmake
add_subdirectory(third_party/cuems-mediadecoder)
target_link_libraries(my_player PRIVATE cuems-mediadecoder)
```

### Compile-time options

| Define | When set |
|---|---|
| `HAVE_SWSCALE` | `pkg_check_modules(SWSCALE libswscale)` succeeded |
| `HAVE_SWRESAMPLE` | `pkg_check_modules(SWRESAMPLE libswresample)` succeeded |

C++ standard: **17** (`CXX_STANDARD 17`).

### Return codes and errors

| API | Success | Common failures |
|---|---|---|
| `MediaFileReader::open` | `true` | `false` (open or `find_stream_info` failed) |
| `MediaFileReader::readPacket` | `0` | `< 0` (EOF or error; use `FFmpegUtils::getErrorString`) |
| `MediaFileReader::seek*` | `true` | `false` (`av_seek_frame` failed — common on live inputs) |
| `VideoDecoder::sendPacket` / `receiveFrame` | `0` | `< 0`; `AVERROR(EAGAIN)` means retry after more input/output |
| `AudioDecoder::sendPacket` / `receiveFrame` | same as video | same |
| `AudioDecoder::convertSamples` | sample count `≥ 0` | `< 0` on resampler error |

There are **no** process exit codes — this is not an executable.

[↑ Back to Table of Contents](#table-of-contents)

---

## Installation

### System dependencies

Debian/Ubuntu example:

```bash
sudo apt-get install -y \
  build-essential cmake pkg-config \
  libavformat-dev libavcodec-dev libavutil-dev \
  libswscale-dev libswresample-dev
```

FFmpeg **4.0 or newer** is required (API guards assume `LIBAVFORMAT_VERSION_INT >= 58` for const input formats).

### Build from source

```bash
git clone https://github.com/stagesoft/cuems-mediadecoder.git
cd cuems-mediadecoder
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The static archive is written under `build/` (exact path depends on the generator; for Unix Makefiles: `build/libcuems-mediadecoder.a`).

There is **no** `install()` rule or systemd unit in this repository today.

### Embed in a parent CMake project

```bash
# In the parent tree
git submodule add https://github.com/stagesoft/cuems-mediadecoder.git third_party/cuems-mediadecoder
```

```cmake
add_subdirectory(third_party/cuems-mediadecoder)
target_link_libraries(cuems-videocomposer PRIVATE cuems-mediadecoder)
```

This repository has **no** `.gitmodules` of its own and **no** submodules to initialise when working only in `cuems-mediadecoder`.

[↑ Back to Table of Contents](#table-of-contents)

---

## Usage

### Minimal file decode loop

```cpp
#include "cuems_mediadecoder/MediaFileReader.h"
#include "cuems_mediadecoder/VideoDecoder.h"
#include "cuems_mediadecoder/FFmpegUtils.h"

using namespace cuems_mediadecoder;

MediaFileReader reader;
if (!reader.open("/path/to/clip.mp4")) { /* handle */ }

const int videoIdx = reader.findStream(AVMEDIA_TYPE_VIDEO);
if (videoIdx < 0) { /* no video */ }

VideoDecoder decoder;
if (!decoder.openCodec(reader.getCodecParameters(videoIdx))) { /* handle */ }

AVPacket* packet = av_packet_alloc();
AVFrame* frame = av_frame_alloc();

while (reader.readPacket(packet) >= 0) {
    if (packet->stream_index != videoIdx) {
        av_packet_unref(packet);
        continue;
    }
    if (decoder.sendPacket(packet) < 0) { /* log FFmpegUtils::getErrorString */ break; }
    while (decoder.receiveFrame(frame) == 0) {
        /* consume frame */
        av_frame_unref(frame);
    }
    av_packet_unref(packet);
}

av_frame_free(&frame);
av_packet_free(&packet);
```

### Hardware video decode

Hardware device creation stays in the parent (for example cuems-videocomposer `HardwareDecoder`).
Either pass a fully configured `AVCodecContext*` to `openCodec`, or call `openHardwareCodec`
with `AVBufferRef* hw_device_ctx` and the negotiated `AVPixelFormat`.

### Timecode-synced seeking

```cpp
std::vector<int> streams = { videoIdx, audioIdx };
double timeSeconds = static_cast<double>(tcFrame) / fps;
reader.seekStreamsToTime(timeSeconds, streams);
videoDecoder.flush();
audioDecoder.flush();
```

### Live streams and devices

```cpp
MediaFileReader reader;
reader.open("rtsp://example.com/live");          // URL
reader.open("/dev/video0", "v4l2");              // capture device
```

For live inputs, avoid seeking unless the demuxer reports a known duration and seek succeeds.
`getDuration()` returns `0.0` when `formatCtx_->duration == AV_NOPTS_VALUE`.

[↑ Back to Table of Contents](#table-of-contents)

---

## Development

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

There is **no** `enable_testing()` or `tests/` directory yet. Planned workflow (see
[Future developments](#future-developments)):

```bash
ctest --test-dir build --output-on-failure
```

Style and review expectations: [CONTRIBUTORS.md](./CONTRIBUTORS.md).

[↑ Back to Table of Contents](#table-of-contents)

---

## Contributors

Contributions follow **spec-first** design for non-trivial changes, **Conventional Commits
v1.0**, **DCO sign-off** on every commit, PRs against **`main`**, and review by a
repository owner.

| Topic | Where it is defined |
|---|---|
| Prerequisites (CMake, FFmpeg, C++17) | [CONTRIBUTORS.md §1](./CONTRIBUTORS.md#1-prerequisites) |
| Tier 1 vs Tier 2 changes | [CONTRIBUTORS.md §3](./CONTRIBUTORS.md#3-contribution-tiers) |
| TDD sequence (when tests exist) | [CONTRIBUTORS.md §6](./CONTRIBUTORS.md#6-tdd-workflow--non-negotiable) |
| SPDX on new files (`LGPL-3.0-or-later` for C++) | [CONTRIBUTORS.md §14](./CONTRIBUTORS.md#14-license) |
| Reviewers | Ion Reguera ([@ibiltari](https://github.com/ibiltari)), Adrià Masip ([@backenv](https://github.com/backenv)) |

Please read [CONTRIBUTORS.md](./CONTRIBUTORS.md) before opening a pull request.

[↑ Back to Table of Contents](#table-of-contents)

---

## Release notes

See [CHANGELOG.md](./CHANGELOG.md) for the full history.

### Unreleased (development on `main`)

Active integration toward **cuems-videocomposer** and **cuems-audioplayer**. The tree
ships a complete demux/decode split (`MediaFileReader`, `VideoDecoder`, `AudioDecoder`,
`FFmpegUtils`), FFmpeg 4.x–6.x compatibility guards, multi-threaded software decoding
(with an AV1 threading exception), and LGPL SPDX headers on all sources. No version tag
or packaged release exists yet.

### Initial module (`c1bb945`)

First import of the shared FFmpeg module: static library layout, live URL and device
opening via `MediaFileReader::open`, coordinated `seekStreamsToTime`, and software-only
audio decoding with optional swscale/swresample helpers.

[↑ Back to Table of Contents](#table-of-contents)

---

## Future developments

The following items are **planned but not implemented** in this repository. Each subsection
ends with the badge to enable once the pipeline is live.

### Automated tests

* Add `tests/` with GoogleTest or Catch2 targets wired through `enable_testing()` /
  `add_test` in `CMakeLists.txt`.
* Cover demuxer open/seek, packet loop EOF, AV1 threading guard, and SwrContext layout
  fallbacks with fixture media under `tests/data/` (small synthetic clips).
* Document `ctest` invocation in README *Development* (already reserved above).

No badge yet — nothing to link until a workflow exists.

### CI tests and coverage

Planned `.github/workflows/tests.yml` on `ubuntu-latest`:

1. Install `cmake`, `g++`, `pkg-config`, and FFmpeg `-dev` packages.
2. Configure with `--coverage`, build, run `ctest --output-on-failure`.
3. `lcov` capture, strip `/usr/*` and `*/tests/*`, upload to Codecov.
4. One-time activation at [codecov.io/gh/stagesoft/cuems-mediadecoder](https://codecov.io/gh/stagesoft/cuems-mediadecoder).

Target badge once live:

[![Tests](https://github.com/stagesoft/cuems-mediadecoder/actions/workflows/tests.yml/badge.svg)](https://github.com/stagesoft/cuems-mediadecoder/actions/workflows/tests.yml)

[![codecov](https://codecov.io/gh/stagesoft/cuems-mediadecoder/graph/badge.svg)](https://codecov.io/gh/stagesoft/cuems-mediadecoder)

### API documentation site

Planned Doxygen (or MkDocs wrapping generated HTML) published to GitHub Pages from `main`,
with a `docs.yml` workflow separate from tests (avoid dual `gh-pages` writers).

Target badge once live:

[![Deploy API documentation](https://github.com/stagesoft/cuems-mediadecoder/actions/workflows/docs.yml/badge.svg)](https://github.com/stagesoft/cuems-mediadecoder/actions/workflows/docs.yml)

Planned site URL: `https://stagesoft.github.io/cuems-mediadecoder/`

### Packaging and distribution

* `debian/` metadata for `libcuems-mediadecoder-dev` (headers + static archive) aligned
  with CueMS player packages on `debian/bookworm`.
* Optional shared-library packaging if consumers require dynamic linking (today: **static only**).
* Parent-repo submodules pinned to tagged releases once versioning starts.

No PyPI badge — this is a native C++ library, not a Python package.

[↑ Back to Table of Contents](#table-of-contents)

---

## Copyright notice

Copyright © 2020–2026 Stagelab Coop SCCL. Author of the original library headers and
implementations: Ion Reguera (`ion@stagelab.coop`).

This work is part of **cuems-mediadecoder**. Library source files are free software
licensed under the **GNU Lesser General Public License** v3.0 or later. You can
redistribute and/or modify them under the terms of the LGPL.

This program is distributed in the hope that it will be useful, but **without any
warranty**; without even the implied warranty of **merchantability** or **fitness for a
particular purpose**. See the GNU Lesser General Public License for more details.

The SPDX short form for **library source** is: `SPDX-License-Identifier: LGPL-3.0-or-later`.

[↑ Back to Table of Contents](#table-of-contents)

---

## License

| Artifact | License |
|---|---|
| C++ headers and sources (`include/`, `src/`) | [LGPL-3.0-or-later](https://www.gnu.org/licenses/lgpl-3.0.html) |
| Markdown documentation (`README.md`, `CHANGELOG.md`, `CONTRIBUTORS.md`) | GPL-3.0-or-later (SPDX on file) |
| Repository `LICENSE` file | [GPL-3.0-or-later](https://www.gnu.org/licenses/gpl-3.0.html) — full GPL v3 text (same as [cuems-dmxplayer](https://github.com/stagesoft/cuems-dmxplayer)) |

See the [LICENSE](./LICENSE) file for the complete GNU General Public License v3 text.

Linking the static library into GPL-licensed CueMS player binaries is intended; if you
distribute modified versions of the library itself, LGPL source-provision rules apply to
those modifications.

When integrating, preserve existing copyright and license notices in upstream headers.

[↑ Back to Table of Contents](#table-of-contents)
