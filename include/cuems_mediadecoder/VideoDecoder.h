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

#ifndef CUEMS_MEDIADECODER_VIDEODECODER_H
#define CUEMS_MEDIADECODER_VIDEODECODER_H

#include "cuems_mediadecoder/MediaFileReader.h"
#include <memory>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

namespace cuems_mediadecoder {

/**
 * VideoDecoder - Video frame decoding operations
 * 
 * Handles video codec opening, frame decoding, and format conversion.
 * Uses FFmpeg API v2 (codecpar + avcodec_alloc_context3).
 * 
 * Note: Hardware decoder setup is left to the calling code (project-specific).
 * This class can work with pre-configured hardware or software codec contexts.
 */
class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    // Disable copy constructor and assignment
    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    /**
     * Open video codec from codec parameters
     * @param codecParams Codec parameters from MediaFileReader
     * @param codecContext Optional pre-allocated codec context (for hardware decoding)
     *                     If nullptr, will allocate a new context
     * @return true on success, false on failure
     */
    bool openCodec(AVCodecParameters* codecParams, AVCodecContext* codecContext = nullptr);

    /**
     * Open video codec with hardware device context
     * @param codecParams Codec parameters from MediaFileReader
     * @param hwDeviceCtx Hardware device context (for hardware decoding)
     * @param hwPixFmt Hardware pixel format
     * @return true on success, false on failure
     */
    bool openHardwareCodec(AVCodecParameters* codecParams, 
                           AVBufferRef* hwDeviceCtx, 
                           AVPixelFormat hwPixFmt);

    /**
     * Close codec and release resources
     */
    void close();

    /**
     * Check if codec is open and ready
     * @return true if ready, false otherwise
     */
    bool isReady() const;

    /**
     * Send a packet to the decoder
     * @param packet AVPacket to send (can be nullptr to flush)
     * @return 0 on success, < 0 on error
     */
    int sendPacket(AVPacket* packet);

    /**
     * Receive a decoded frame from the decoder
     * @param frame AVFrame to fill (must be allocated)
     * @return 0 on success, AVERROR(EAGAIN) if more packets needed, < 0 on error
     */
    int receiveFrame(AVFrame* frame);

    /**
     * Flush decoder buffers
     */
    void flush();

    /**
     * Get codec context (for advanced operations)
     * @return AVCodecContext pointer, or nullptr if not open
     */
    AVCodecContext* getCodecContext() const { return codecCtx_; }

    /**
     * Check if using hardware decoding
     * @return true if hardware decoding, false if software
     */
    bool isHardwareDecoding() const { return useHardwareDecoding_; }

    /**
     * Get video properties
     * @param width Output: video width
     * @param height Output: video height
     * @param pixFmt Output: pixel format
     * @return true on success, false if codec not open
     */
    bool getVideoProperties(int& width, int& height, AVPixelFormat& pixFmt) const;

    /**
     * Create a SwsContext for format conversion
     * @param srcWidth Source width
     * @param srcHeight Source height
     * @param srcPixFmt Source pixel format
     * @param dstWidth Destination width
     * @param dstHeight Destination height
     * @param dstPixFmt Destination pixel format
     * @return SwsContext pointer, or nullptr on failure
     */
    SwsContext* createSwsContext(int srcWidth, int srcHeight, AVPixelFormat srcPixFmt,
                                 int dstWidth, int dstHeight, AVPixelFormat dstPixFmt);

    /**
     * Free a SwsContext
     * @param swsCtx SwsContext to free
     */
    static void freeSwsContext(SwsContext* swsCtx);

    // Future: HAP direct compressed texture upload support
    // The following methods will be added when implementing HAP direct upload:
    // - extractDXTFromPacket(AVPacket* packet, uint8_t*& dxtData, size_t& dxtSize)
    // - detectHAPVariant(AVPacket* packet)
    // These will allow extracting DXT1/DXT5 compressed data directly from HAP packets
    // without going through FFmpeg's HAP decoder (which outputs uncompressed RGBA)

private:
    AVCodecContext* codecCtx_;
    bool codecCtxAllocated_;  // Whether we allocated codecCtx_ or it was provided
    bool useHardwareDecoding_;
};

} // namespace cuems_mediadecoder

#endif // CUEMS_MEDIADECODER_VIDEODECODER_H

