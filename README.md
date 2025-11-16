# cuems-mediadecoder

Common media decoder module for cuems-videocomposer and cuems-audioplayer

Provides unified media reading and decoding capabilities for both local files and network streams.

## Overview

This module provides shared FFmpeg operations that are common to both projects:
- Media file format operations (opening, stream detection, seeking)
- Video codec decoding
- Audio codec decoding
- Format conversion utilities

## Design Principles

1. **FFmpeg API v2**: Uses modern FFmpeg API (`codecpar` + `avcodec_alloc_context3`)
2. **No hardware audio decoding**: Software decoding only for audio
3. **HAP support**: Prepared for future direct compressed texture upload (not implemented yet)
4. **Minimal abstraction**: Exposes FFmpeg concepts clearly while hiding repetitive code

## Components

### MediaFileReader
Common format/stream operations:
- Source opening (`avformat_open_input`, `avformat_find_stream_info`)
  - Supports both local files and network streams (RTSP, HTTP, UDP, etc.)
- Stream detection (audio/video)
- Seeking support (single stream or multiple streams)
  - Note: Live streams typically don't support seeking (check `isSeekable()`)
- Packet reading (works for both files and live streams)
- Live stream detection (`isLiveStream()`)
- Timecode-friendly: `seekStreamsToTime()` for coordinated A/V seeking

### VideoDecoder
Video-specific decoding:
- Codec opening (software and hardware)
- Frame decoding
- Format conversion (libswscale)

### AudioDecoder
Audio-specific decoding:
- Codec opening (software only)
- Sample decoding
- Format conversion to float (libswresample)

### FFmpegUtils
Common utilities:
- Error code translation

## Usage

### In CMakeLists.txt

```cmake
add_subdirectory(src/cuems-mediadecoder)
target_link_libraries(your_target cuems-mediadecoder)
```

### In C++ code

```cpp
#include "cuems_mediadecoder/MediaFileReader.h"
#include "cuems_mediadecoder/VideoDecoder.h"
#include "cuems_mediadecoder/AudioDecoder.h"

using namespace cuems_mediadecoder;

// Open media file
MediaFileReader reader;
if (!reader.open("video.mp4")) {
    // Handle error
}

// Find video stream
int videoStream = reader.findStream(AVMEDIA_TYPE_VIDEO);
if (videoStream < 0) {
    // No video stream
}

// Get codec parameters
AVCodecParameters* codecParams = reader.getCodecParameters(videoStream);

// Open video decoder
VideoDecoder decoder;
if (!decoder.openCodec(codecParams)) {
    // Handle error
}

// Decode frames...
```

## Timecode-Synced Playback Support

The module is well-suited for timecode-synced A/V playback:
- Frame-based design matches timecode-driven workflows
- `seekStreamsToTime()` allows coordinated seeking of multiple streams
- No internal clock management needed (timecode provides sync)
- Independent decoding paths for video and audio

Example for timecode-synced player:
```cpp
// Get frame from timecode
int64_t tcFrame = syncSource->pollFrame();
double timeSeconds = tcFrame / videoFps;

// Seek both streams to same time
std::vector<int> streams = {videoStream, audioStream};
reader.seekStreamsToTime(timeSeconds, streams);
```

## Future: HAP Direct Compressed Texture Upload

The module is designed to support future HAP direct compressed texture upload:
- `MediaFileReader` can read packets without decoding
- `VideoDecoder` can be extended with HAP-specific methods
- Architecture allows extracting DXT data directly from packets

This will be implemented in a future update.

## Future: MediaPlayer Wrapper Class

A high-level `MediaPlayer` wrapper class may be added in the future to:
- Manage both video and audio decoders automatically
- Provide unified playback interface
- Handle packet routing and demuxing

For now, the low-level components provide maximum flexibility for timecode-synced workflows.

## Live Stream Support

The module is designed to support live media streams (RTSP, HTTP, UDP, etc.):

### Current Support ✅
- **URL-based opening**: `open()` accepts both file paths and URLs
  - Local files: `"/path/to/video.mp4"`
  - Network streams: `"rtsp://...", "http://...", "udp://..."`
- **Device-based opening**: Video input devices via format parameter
  - V4L2: `open("/dev/video0", "v4l2")`
  - Capture cards: `open("device_name", "decklink")` (requires FFmpeg with device support)
  - DirectShow: `open("video=Device", "dshow")` (Windows)
- **Stream detection**: Works identically for files, URLs, and devices
- **Packet reading**: Continuous reading from live streams/devices
- **Decoders**: Video and audio decoders work the same for all sources

### Live Stream Characteristics
- **Seeking**: Live streams typically don't support seeking
  - Use `isSeekable()` to check before attempting to seek
  - Use `isLiveStream()` to detect network sources
- **Duration**: Often unknown for live streams (`getDuration()` returns 0.0)
- **Timecode sync**: Works well - timecode provides sync, no seeking needed

### Example: Live Stream Usage
```cpp
// Network stream
cuems_mediadecoder::MediaFileReader reader;
if (!reader.open("rtsp://example.com/stream")) {
    // Handle error
}

// Video input device (V4L2)
cuems_mediadecoder::MediaFileReader deviceReader;
if (!deviceReader.open("/dev/video0", "v4l2")) {
    // Handle error - check device exists and permissions
}

// Check if it's a live stream
if (reader.isLiveStream()) {
    // Don't attempt seeking
    // Read packets continuously
}

// Decoders work the same way
int videoStream = reader.findStream(AVMEDIA_TYPE_VIDEO);
AVCodecParameters* codecParams = reader.getCodecParameters(videoStream);

cuems_mediadecoder::VideoDecoder decoder;
decoder.openCodec(codecParams);

// Read and decode packets continuously
AVPacket* packet = av_packet_alloc();
while (reader.readPacket(packet) == 0) {
    // Decode packet...
}
```

### Future Enhancements
- Buffering strategies for live streams
- Reconnection handling for dropped connections
- Stream quality adaptation
- Multiple concurrent live streams

## License

Same as parent projects.

