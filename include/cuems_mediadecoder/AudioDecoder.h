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

#ifndef CUEMS_MEDIADECODER_AUDIODECODER_H
#define CUEMS_MEDIADECODER_AUDIODECODER_H

#include "cuems_mediadecoder/MediaFileReader.h"
#include <memory>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

namespace cuems_mediadecoder {

/**
 * AudioDecoder - Audio sample decoding operations
 * 
 * Handles audio codec opening, sample decoding, and format conversion.
 * Uses FFmpeg API v2 (codecpar + avcodec_alloc_context3).
 * 
 * Note: Resampling (libsoxr) is left to the calling code (project-specific).
 * This class handles format conversion to float via libswresample.
 */
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // Disable copy constructor and assignment
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    /**
     * Open audio codec from codec parameters
     * @param codecParams Codec parameters from MediaFileReader
     * @return true on success, false on failure
     */
    bool openCodec(AVCodecParameters* codecParams);

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
     * Get audio properties
     * @param channels Output: number of channels
     * @param sampleRate Output: sample rate
     * @param sampleFmt Output: sample format
     * @return true on success, false if codec not open
     */
    bool getAudioProperties(int& channels, int& sampleRate, AVSampleFormat& sampleFmt) const;

    /**
     * Create a SwrContext for format conversion to float
     * @param outChannels Output channel count (default: same as input)
     * @param outSampleRate Output sample rate (default: same as input)
     * @return SwrContext pointer, or nullptr on failure
     */
    SwrContext* createSwrContext(int outChannels = -1, int outSampleRate = -1);

    /**
     * Create a SwrContext with explicit parameters
     * @param outChannels Output channel count
     * @param outSampleRate Output sample rate
     * @param outSampleFmt Output sample format (default: AV_SAMPLE_FMT_FLT)
     * @return SwrContext pointer, or nullptr on failure
     */
    SwrContext* createSwrContextExplicit(int outChannels, 
                                         int outSampleRate, 
                                         AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_FLT);

    /**
     * Free a SwrContext
     * @param swrCtx SwrContext to free
     */
    static void freeSwrContext(SwrContext* swrCtx);

    /**
     * Convert audio samples using SwrContext
     * @param swrCtx SwrContext for conversion
     * @param out Output buffer
     * @param outCount Output sample count
     * @param in Input frame
     * @return Number of samples converted, or < 0 on error
     */
    static int convertSamples(SwrContext* swrCtx, uint8_t** out, int outCount, const AVFrame* in);

private:
    AVCodecContext* codecCtx_;
};

} // namespace cuems_mediadecoder

#endif // CUEMS_MEDIADECODER_AUDIODECODER_H

