#include "cuems_mediadecoder/VideoDecoder.h"
#include "cuems_mediadecoder/FFmpegUtils.h"
#include <cstring>

namespace cuems_mediadecoder {

VideoDecoder::VideoDecoder()
    : codecCtx_(nullptr)
    , codecCtxAllocated_(false)
    , useHardwareDecoding_(false)
{
}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::openCodec(AVCodecParameters* codecParams, AVCodecContext* codecContext) {
    if (!codecParams) {
        return false;
    }

    // Close any existing codec
    close();

    // If codec context provided, use it (for hardware decoding)
    if (codecContext) {
        codecCtx_ = codecContext;
        codecCtxAllocated_ = false;
        useHardwareDecoding_ = (codecCtx_->hw_device_ctx != nullptr);
    } else {
        // Allocate new codec context (software decoding)
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            return false;
        }

        codecCtx_ = avcodec_alloc_context3(codec);
        if (!codecCtx_) {
            return false;
        }
        codecCtxAllocated_ = true;

        // Copy codec parameters to context (FFmpeg API v2)
        int ret = avcodec_parameters_to_context(codecCtx_, codecParams);
        if (ret < 0) {
            avcodec_free_context(&codecCtx_);
            codecCtx_ = nullptr;
            return false;
        }

        // Enable multi-threaded decoding for software codecs
        // EXCEPTION: AV1 (libdav1d) has issues with threading - disable it completely
        if (codecParams->codec_id == AV_CODEC_ID_AV1) {
            // AV1/libdav1d: No threading to avoid seek/flush issues
            codecCtx_->thread_count = 1;
            codecCtx_->thread_type = 0;
        } else {
            // Other codecs: Use full multi-threading for performance
            codecCtx_->thread_count = 0;  // Auto-detect optimal thread count
            if (codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
                codecCtx_->thread_type |= FF_THREAD_FRAME;
            }
            if (codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
                codecCtx_->thread_type |= FF_THREAD_SLICE;
            }
            // If codec doesn't advertise threading, still try auto-threading
            if (codecCtx_->thread_type == 0) {
                codecCtx_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
            }
        }

        // Open codec
        ret = avcodec_open2(codecCtx_, codec, nullptr);
        if (ret < 0) {
            avcodec_free_context(&codecCtx_);
            codecCtx_ = nullptr;
            return false;
        }

        useHardwareDecoding_ = false;
    }

    return true;
}

bool VideoDecoder::openHardwareCodec(AVCodecParameters* codecParams,
                                      AVBufferRef* hwDeviceCtx,
                                      AVPixelFormat hwPixFmt) {
    if (!codecParams || !hwDeviceCtx) {
        return false;
    }

    close();

    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        return false;
    }

    // Allocate codec context
    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        return false;
    }
    codecCtxAllocated_ = true;

    // Copy codec parameters (FFmpeg API v2)
    int ret = avcodec_parameters_to_context(codecCtx_, codecParams);
    if (ret < 0) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        return false;
    }

    // Set hardware device context
    codecCtx_->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
    if (!codecCtx_->hw_device_ctx) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        return false;
    }

    // Set hardware pixel format callback
    codecCtx_->get_format = [](AVCodecContext* ctx, const AVPixelFormat* pix_fmts) -> AVPixelFormat {
        const AVPixelFormat* p;
        AVPixelFormat hwPixFmt = *reinterpret_cast<AVPixelFormat*>(ctx->opaque);
        for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == hwPixFmt) {
                return *p;
            }
        }
        return AV_PIX_FMT_NONE;
    };
    codecCtx_->opaque = reinterpret_cast<void*>(static_cast<intptr_t>(hwPixFmt));

    // Open codec
    ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        return false;
    }

    useHardwareDecoding_ = true;
    return true;
}

void VideoDecoder::close() {
    if (codecCtx_ && codecCtxAllocated_) {
        avcodec_free_context(&codecCtx_);
    }
    codecCtx_ = nullptr;
    codecCtxAllocated_ = false;
    useHardwareDecoding_ = false;
}

bool VideoDecoder::isReady() const {
    return codecCtx_ != nullptr;
}

int VideoDecoder::sendPacket(AVPacket* packet) {
    if (!codecCtx_) {
        return AVERROR(EINVAL);
    }

    return avcodec_send_packet(codecCtx_, packet);
}

int VideoDecoder::receiveFrame(AVFrame* frame) {
    if (!codecCtx_ || !frame) {
        return AVERROR(EINVAL);
    }

    return avcodec_receive_frame(codecCtx_, frame);
}

void VideoDecoder::flush() {
    // codec->flush was removed in FFmpeg 4.0+, but avcodec_flush_buffers() is always available
    if (codecCtx_) {
        avcodec_flush_buffers(codecCtx_);
    }
}

bool VideoDecoder::getVideoProperties(int& width, int& height, AVPixelFormat& pixFmt) const {
    if (!codecCtx_) {
        return false;
    }

    width = codecCtx_->width;
    height = codecCtx_->height;
    pixFmt = codecCtx_->pix_fmt;
    return true;
}

SwsContext* VideoDecoder::createSwsContext(int srcWidth, int srcHeight, AVPixelFormat srcPixFmt,
                                            int dstWidth, int dstHeight, AVPixelFormat dstPixFmt) {
    return sws_getContext(
        srcWidth, srcHeight, srcPixFmt,
        dstWidth, dstHeight, dstPixFmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
}

void VideoDecoder::freeSwsContext(SwsContext* swsCtx) {
    if (swsCtx) {
        sws_freeContext(swsCtx);
    }
}

} // namespace cuems_mediadecoder

