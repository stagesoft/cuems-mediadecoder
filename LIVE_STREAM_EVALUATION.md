# Live Stream Support Evaluation

## Summary

The `cuems-mediadecoder` module is **well-suited for live media streams** with minimal changes needed.

## Current Architecture Suitability

### ✅ Excellent Fit

1. **FFmpeg's `avformat_open_input()` already supports URLs**
   - No code changes needed - accepts both file paths and URLs
   - Supports: RTSP, HTTP, UDP, RTP, and more
   - Example: `reader.open("rtsp://example.com/stream")` works out of the box

2. **Decoders are codec-agnostic**
   - `VideoDecoder` and `AudioDecoder` work identically for files and streams
   - Same codec opening, packet sending, and frame receiving
   - No changes needed to decoder classes

3. **Packet reading is identical**
   - `readPacket()` works the same for files and streams
   - Continuous reading from live streams is natural

4. **Timecode sync is ideal**
   - Live streams don't need seeking (timecode provides sync)
   - Frame-based design matches timecode-driven workflows
   - No internal clock management needed

### ⚠️ Considerations

1. **Seeking limitations**
   - Live streams typically don't support seeking
   - Solution: Added `isSeekable()` and `isLiveStream()` methods
   - Applications should check before attempting to seek

2. **Duration unknown**
   - Live streams often have unknown duration
   - Solution: `getDuration()` already returns 0.0 for unknown (works correctly)

3. **Network reliability**
   - Live streams may drop connections
   - Future enhancement: Add reconnection handling
   - Current: FFmpeg handles basic network errors

## Implementation Status

### ✅ Implemented
- URL-based opening (via FFmpeg)
- Device-based opening (V4L2, capture cards via format parameter)
- Stream detection (works for files, URLs, and devices)
- Packet reading (continuous)
- Decoder support (codec-agnostic)
- Live stream detection (`isLiveStream()`)
- Seekability check (`isSeekable()`)
- Format specification for device inputs

### 🔮 Future Enhancements
- Buffering strategies for live streams
- Automatic reconnection on dropped connections
- Stream quality adaptation
- Multiple concurrent live streams
- Low-latency streaming optimizations

## Usage Pattern for Live Streams

```cpp
// Open live stream
cuems_mediadecoder::MediaFileReader reader;
if (!reader.open("rtsp://example.com/stream")) {
    // Handle error
}

// Detect stream type
bool isLive = reader.isLiveStream();
bool canSeek = reader.isSeekable();

// Find streams
int videoStream = reader.findStream(AVMEDIA_TYPE_VIDEO);
int audioStream = reader.findStream(AVMEDIA_TYPE_AUDIO);

// Open decoders (same as files)
AVCodecParameters* videoParams = reader.getCodecParameters(videoStream);
cuems_mediadecoder::VideoDecoder videoDecoder;
videoDecoder.openCodec(videoParams);

// Read packets continuously (no seeking needed for live)
AVPacket* packet = av_packet_alloc();
while (reader.readPacket(packet) == 0) {
    // Decode and process...
}
```

## Video Input Devices (V4L2, Capture Cards)

### ✅ Excellent Support

The module supports live video input from hardware devices:

1. **V4L2 (Video4Linux2) devices**
   - Linux webcams, USB cameras, TV tuners
   - Use format parameter: `open("/dev/video0", "v4l2")`
   - Example: `reader.open("/dev/video0", "v4l2")`

2. **Dedicated capture cards**
   - DeckLink (Blackmagic): `open("DeckLink Mini Recorder", "decklink")`
   - DELTACAST: Uses VideoMaster plugin
   - Other cards: Device-specific formats
   - Requires FFmpeg built with device support

3. **Windows DirectShow**
   - Webcams and capture devices on Windows
   - Use format: `open("video=Device Name", "dshow")`

### Device Characteristics

- **Live sources**: No seeking, continuous reading
- **Real-time**: Frame rate matches capture device
- **Hardware-specific**: May require special FFmpeg builds
- **Format specification**: Usually need to specify format explicitly

### Usage Pattern for Video Input Devices

```cpp
// V4L2 device (Linux)
cuems_mediadecoder::MediaFileReader reader;
if (!reader.open("/dev/video0", "v4l2")) {
    // Handle error - check if device exists and permissions
}

// Check if it's a live source
if (reader.isLiveStream()) {
    // Don't attempt seeking
}

// Find video stream
int videoStream = reader.findStream(AVMEDIA_TYPE_VIDEO);
AVCodecParameters* codecParams = reader.getCodecParameters(videoStream);

// Open decoder (same as files/streams)
cuems_mediadecoder::VideoDecoder decoder;
decoder.openCodec(codecParams);

// Read frames continuously
AVPacket* packet = av_packet_alloc();
while (reader.readPacket(packet) == 0) {
    // Decode and process frames in real-time
}
```

### Device-Specific Considerations

1. **FFmpeg build requirements**
   - V4L2: Usually included in standard Linux builds
   - DeckLink: Requires `--enable-decklink` at compile time
   - DELTACAST: Requires VideoMaster plugin
   - DirectShow: Windows builds usually include it

2. **Permissions**
   - V4L2: May require user to be in `video` group
   - Check device access: `ls -l /dev/video*`

3. **Device enumeration**
   - V4L2: `v4l2-ctl --list-devices`
   - DirectShow: `ffmpeg -list_devices true -f dshow -i dummy`
   - DeckLink: Device names in FFmpeg documentation

4. **Format options**
   - V4L2: May need resolution/framerate via AVDictionary
   - Future enhancement: Add helper methods for device options

## Conclusion

The current architecture is **highly suitable** for live streams and video input devices:

- ✅ **No breaking changes needed** - existing code works
- ✅ **Decoders work identically** - codec-agnostic design
- ✅ **Timecode sync friendly** - no seeking required
- ✅ **Device support** - V4L2, capture cards via format parameter
- ✅ **Future-ready** - can add enhancements without refactoring

The module can handle:
- **Network streams** (RTSP, HTTP, UDP) - works today
- **Video input devices** (V4L2, capture cards) - works with format parameter
- **Local files** - original functionality preserved

Future enhancements can be added incrementally without architectural changes.

