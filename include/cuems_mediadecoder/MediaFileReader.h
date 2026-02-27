/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright (C) 2020-2026 Stage Lab Coop.
 * Author: Ion Reguera <ion@stagelab.coop>
 *
 * This file is part of cuems-videocomposer.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CUEMS_MEDIADECODER_MEDIAFILEREADER_H
#define CUEMS_MEDIADECODER_MEDIAFILEREADER_H

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

namespace cuems_mediadecoder {

/**
 * MediaFileReader - Common media source operations
 * 
 * Handles format opening, stream detection, and seeking operations
 * that are common to both audio and video decoding.
 * 
 * Supports both local files and network streams:
 * - Local files: regular video/audio files
 * - Network streams: RTSP, HTTP, UDP, etc. (live or on-demand)
 * - Video input devices: V4L2, capture cards (via format parameter)
 * 
 * Uses FFmpeg API v2 (codecpar + avcodec_alloc_context3)
 */
class MediaFileReader {
public:
    MediaFileReader();
    ~MediaFileReader();

    // Disable copy constructor and assignment
    MediaFileReader(const MediaFileReader&) = delete;
    MediaFileReader& operator=(const MediaFileReader&) = delete;

    /**
     * Open a media source (file, URL, or device)
     * @param path Path to media file, URL, or device
     * @param format Optional format name (e.g., "v4l2", "decklink", "dshow")
     *               If empty, FFmpeg will auto-detect from path/URL
     * @return true on success, false on failure
     * 
     * Supports multiple source types:
     * - Local files: "/path/to/video.mp4"
     * - Network streams: "rtsp://...", "http://...", "udp://...", etc.
     * - V4L2 devices: "/dev/video0" (use format="v4l2")
     * - Capture cards: device-specific paths (use appropriate format)
     */
    bool open(const std::string& path, const std::string& format = "");

    /**
     * Close the media file and release resources
     */
    void close();

    /**
     * Check if file is open and ready
     * @return true if ready, false otherwise
     */
    bool isReady() const;

    /**
     * Find a stream of the specified type
     * @param mediaType Stream type (AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO, etc.)
     * @return Stream index, or -1 if not found
     */
    int findStream(enum AVMediaType mediaType);

    /**
     * Get codec parameters for a stream
     * @param streamIndex Stream index
     * @return AVCodecParameters pointer, or nullptr if invalid
     */
    AVCodecParameters* getCodecParameters(int streamIndex);

    /**
     * Get stream information
     * @param streamIndex Stream index
     * @return AVStream pointer, or nullptr if invalid
     */
    AVStream* getStream(int streamIndex);

    /**
     * Seek to a specific timestamp
     * @param timestamp Timestamp in stream's time_base units
     * @param streamIndex Stream index to seek in
     * @param flags Seek flags (AVSEEK_FLAG_BACKWARD, etc.)
     * @return true on success, false on failure
     */
    bool seek(int64_t timestamp, int streamIndex, int flags = AVSEEK_FLAG_BACKWARD);

    /**
     * Seek to a specific timestamp in seconds
     * @param timeSeconds Time in seconds
     * @param streamIndex Stream index to seek in
     * @param flags Seek flags
     * @return true on success, false on failure
     */
    bool seekToTime(double timeSeconds, int streamIndex, int flags = AVSEEK_FLAG_BACKWARD);

    /**
     * Seek multiple streams to the same time (useful for A/V sync)
     * @param timeSeconds Time in seconds
     * @param streamIndices Vector of stream indices to seek
     * @param flags Seek flags
     * @return true if all seeks succeeded, false if any failed
     */
    bool seekStreamsToTime(double timeSeconds, const std::vector<int>& streamIndices, int flags = AVSEEK_FLAG_BACKWARD);

    /**
     * Read a packet from the file
     * @param packet AVPacket to fill (must be allocated with av_packet_alloc)
     * @return 0 on success, < 0 on error or EOF
     */
    int readPacket(AVPacket* packet);

    /**
     * Get file duration in seconds
     * @return Duration in seconds, or 0.0 if unknown
     */
    double getDuration() const;

    /**
     * Get format context (for advanced operations)
     * @return AVFormatContext pointer, or nullptr if not open
     */
    AVFormatContext* getFormatContext() const { return formatCtx_; }

    /**
     * Get number of streams
     * @return Number of streams
     */
    unsigned int getStreamCount() const;

private:
    AVFormatContext* formatCtx_;
    std::string currentFile_;
    bool ready_;
};

} // namespace cuems_mediadecoder

#endif // CUEMS_MEDIADECODER_MEDIAFILEREADER_H

