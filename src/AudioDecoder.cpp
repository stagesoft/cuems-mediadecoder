#include "cuems_mediadecoder/AudioDecoder.h"
#include "cuems_mediadecoder/FFmpegUtils.h"
#include <cstring>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/version.h>
#include <libswresample/version.h>
}

namespace cuems_mediadecoder {

AudioDecoder::AudioDecoder()
    : codecCtx_(nullptr)
{
}

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::openCodec(AVCodecParameters* codecParams) {
    if (!codecParams) {
        return false;
    }

    close();

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        return false;
    }

    // Allocate codec context
    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        return false;
    }

    // Copy codec parameters to context (FFmpeg API v2)
    int ret = avcodec_parameters_to_context(codecCtx_, codecParams);
    if (ret < 0) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        return false;
    }

    // Open codec
    ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
        return false;
    }

    return true;
}

void AudioDecoder::close() {
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
}

bool AudioDecoder::isReady() const {
    return codecCtx_ != nullptr;
}

int AudioDecoder::sendPacket(AVPacket* packet) {
    if (!codecCtx_) {
        return AVERROR(EINVAL);
    }

    return avcodec_send_packet(codecCtx_, packet);
}

int AudioDecoder::receiveFrame(AVFrame* frame) {
    if (!codecCtx_ || !frame) {
        return AVERROR(EINVAL);
    }

    return avcodec_receive_frame(codecCtx_, frame);
}

void AudioDecoder::flush() {
    // codec->flush was removed in FFmpeg 4.0+, but avcodec_flush_buffers() is always available
    if (codecCtx_) {
        avcodec_flush_buffers(codecCtx_);
    }
}

bool AudioDecoder::getAudioProperties(int& channels, int& sampleRate, AVSampleFormat& sampleFmt) const {
    if (!codecCtx_) {
        return false;
    }

    #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
        channels = codecCtx_->ch_layout.nb_channels;
    #else
        channels = codecCtx_->channels;
    #endif
    sampleRate = codecCtx_->sample_rate;
    sampleFmt = codecCtx_->sample_fmt;
    return true;
}

SwrContext* AudioDecoder::createSwrContext(int outChannels, int outSampleRate) {
    if (!codecCtx_) {
        return nullptr;
    }

    // Use input parameters as defaults
    if (outChannels < 0) {
        #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            outChannels = codecCtx_->ch_layout.nb_channels;
        #else
            outChannels = codecCtx_->channels;
        #endif
    }
    if (outSampleRate < 0) {
        outSampleRate = codecCtx_->sample_rate;
    }

    return createSwrContextExplicit(outChannels, outSampleRate, AV_SAMPLE_FMT_FLT);
}

SwrContext* AudioDecoder::createSwrContextExplicit(int outChannels, 
                                                     int outSampleRate, 
                                                     AVSampleFormat outSampleFmt) {
    if (!codecCtx_) {
        return nullptr;
    }

    SwrContext* swrCtx = nullptr;

#if LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4, 7, 100)
    // FFmpeg 5.1+ API with AVChannelLayout
    AVChannelLayout outChLayout;
    AVChannelLayout inChLayout;
    
    av_channel_layout_default(&outChLayout, outChannels);
    
    // Handle unknown/unspecified input channel layout (like mpv does)
    if (codecCtx_->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC || 
        codecCtx_->ch_layout.nb_channels == 0) {
        // Channel layout is unknown, use default layout for channel count
        int inChannels;
        #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            inChannels = codecCtx_->ch_layout.nb_channels;
        #else
            inChannels = codecCtx_->channels;
        #endif
        av_channel_layout_default(&inChLayout, inChannels);
    } else {
        av_channel_layout_copy(&inChLayout, &codecCtx_->ch_layout);
    }

    // Create resampler context
    int ret = swr_alloc_set_opts2(&swrCtx,
                                  &outChLayout, outSampleFmt, outSampleRate,
                                  &inChLayout, codecCtx_->sample_fmt, codecCtx_->sample_rate,
                                  0, nullptr);

    // Clean up temporary channel layouts
    av_channel_layout_uninit(&outChLayout);
    av_channel_layout_uninit(&inChLayout);
    
    if (ret < 0 || !swrCtx) {
        return nullptr;
    }

    // Initialize resampler
    ret = swr_init(swrCtx);
    if (ret < 0) {
        swr_free(&swrCtx);
        return nullptr;
    }
#else
    // FFmpeg 4.x/5.0 API with channel_layout (uint64_t)
    uint64_t outChannelLayout = av_get_default_channel_layout(outChannels);
    uint64_t inChannelLayout;
    
    // Get input channel layout
    #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
        // FFmpeg 6.0+ uses ch_layout
        inChannelLayout = av_get_default_channel_layout(codecCtx_->ch_layout.nb_channels);
    #else
        // FFmpeg 4.x/5.x uses channel_layout
        inChannelLayout = codecCtx_->channel_layout;
        if (!inChannelLayout) {
            // Fallback: construct from channel count
            inChannelLayout = av_get_default_channel_layout(codecCtx_->channels);
        }
    #endif

    // Create resampler context (old API)
    swrCtx = swr_alloc_set_opts(nullptr,
                                outChannelLayout, outSampleFmt, outSampleRate,
                                inChannelLayout, codecCtx_->sample_fmt, codecCtx_->sample_rate,
                                0, nullptr);

    if (!swrCtx) {
        return nullptr;
    }

    // Initialize resampler
    int ret = swr_init(swrCtx);
    if (ret < 0) {
        swr_free(&swrCtx);
        return nullptr;
    }
#endif

    return swrCtx;
}

void AudioDecoder::freeSwrContext(SwrContext* swrCtx) {
    if (swrCtx) {
        swr_free(&swrCtx);
    }
}

int AudioDecoder::convertSamples(SwrContext* swrCtx, uint8_t** out, int outCount, const AVFrame* in) {
    if (!swrCtx || !out || !in) {
        return AVERROR(EINVAL);
    }

    return swr_convert(swrCtx, out, outCount, 
                       const_cast<const uint8_t**>(in->data), in->nb_samples);
}

} // namespace cuems_mediadecoder

