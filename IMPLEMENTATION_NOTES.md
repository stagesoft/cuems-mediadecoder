# cuems-mediadecoder Implementation Notes

## Module Structure

The module has been designed and implemented with the following components:

### Core Classes

1. **MediaFileReader** (`MediaFileReader.h/cpp`)
   - Common format/stream operations
   - File opening, stream detection, seeking
   - Uses FFmpeg API v2 (`codecpar`)
   - Handles both audio and video files

2. **VideoDecoder** (`VideoDecoder.h/cpp`)
   - Video codec opening (software and hardware)
   - Frame decoding operations
   - Format conversion via libswscale
   - Prepared for future HAP direct compressed texture upload

3. **AudioDecoder** (`AudioDecoder.h/cpp`)
   - Audio codec opening (software only)
   - Sample decoding operations
   - Format conversion to float via libswresample
   - No hardware audio decoding (as per plan)

4. **FFmpegUtils** (`FFmpegUtils.h/cpp`)
   - Error code translation utilities
   - Common helper functions

## Design Decisions

### FFmpeg API Version
- **Uses API v2**: All code uses `codecpar` + `avcodec_alloc_context3`
- **Future-proof**: Compatible with current and future FFmpeg versions
- **Migration benefit**: Will fix deprecated API usage in videocomposer when integrated

### Hardware Decoding
- **Video**: Supports both hardware and software decoding
- **Audio**: Software decoding only (as specified in plan)
- Hardware setup is left to calling code (project-specific)

### HAP Support
- **Current**: Module works with existing HAP code (via FFmpeg decoder)
- **Future**: Architecture prepared for direct compressed texture upload
- Comments added in `VideoDecoder.h` indicating future methods:
  - `extractDXTFromPacket()` - Extract DXT data from HAP packets
  - `detectHAPVariant()` - Detect HAP variant from packet

## Integration Notes

### For cuems-videocomposer
- `VideoFileInput` will use `MediaFileReader` + `VideoDecoder` internally
- Maintains existing `InputSource` interface (adapter pattern)
- Hardware decoder setup stays in videocomposer (uses `HardwareDecoder` class)

### For cuems-audioplayer
- `AudioFstream` will use `MediaFileReader` + `AudioDecoder` internally
- Maintains existing stream-like interface
- `libsoxr` resampling stays in audioplayer (project-specific)

## Build Configuration

- Static library (matches `mtcreceiver` pattern)
- CMake-based build system
- Optional dependencies: libswscale, libswresample
- C++17 standard

## Next Steps

1. Add module as git submodule to both projects
2. Refactor `VideoFileInput` to use common module
3. Refactor `AudioFstream` to use common module
4. Test thoroughly to ensure no regressions
5. Future: Implement HAP direct compressed texture upload

