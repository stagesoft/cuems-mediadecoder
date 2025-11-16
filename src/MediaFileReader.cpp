#include "cuems_mediadecoder/MediaFileReader.h"
#include "cuems_mediadecoder/FFmpegUtils.h"

namespace cuems_mediadecoder {

MediaFileReader::MediaFileReader()
    : formatCtx_(nullptr)
    , ready_(false)
{
}

MediaFileReader::~MediaFileReader() {
    close();
}

bool MediaFileReader::open(const std::string& path, const std::string& format) {
    if (path.empty()) {
        return false;
    }

    // Close any existing file
    close();

    currentFile_ = path;
    ready_ = false;

    // Prepare format options if specified
    AVDictionary* formatOpts = nullptr;
    AVInputFormat* inputFormat = nullptr;
    
    if (!format.empty()) {
        inputFormat = av_find_input_format(format.c_str());
        if (!inputFormat) {
            // Format not found - will try auto-detection
            // This is not necessarily an error, FFmpeg might still succeed
        }
    }

    // Open input (file, URL, or device)
    int ret = avformat_open_input(&formatCtx_, path.c_str(), inputFormat, &formatOpts);
    
    // Clean up format options
    if (formatOpts) {
        av_dict_free(&formatOpts);
    }
    
    if (ret < 0) {
        formatCtx_ = nullptr;
        return false;
    }

    // Retrieve stream information
    ret = avformat_find_stream_info(formatCtx_, nullptr);
    if (ret < 0) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
        return false;
    }

    ready_ = true;
    return true;
}

void MediaFileReader::close() {
    if (formatCtx_) {
        avformat_close_input(&formatCtx_);
        formatCtx_ = nullptr;
    }
    currentFile_.clear();
    ready_ = false;
}

bool MediaFileReader::isReady() const {
    return ready_ && formatCtx_ != nullptr;
}

int MediaFileReader::findStream(enum AVMediaType mediaType) {
    if (!formatCtx_) {
        return -1;
    }

    for (unsigned int i = 0; i < formatCtx_->nb_streams; ++i) {
        AVCodecParameters* codecpar = formatCtx_->streams[i]->codecpar;
        if (codecpar && codecpar->codec_type == mediaType) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

AVCodecParameters* MediaFileReader::getCodecParameters(int streamIndex) {
    if (!formatCtx_ || streamIndex < 0 || 
        static_cast<unsigned int>(streamIndex) >= formatCtx_->nb_streams) {
        return nullptr;
    }

    return formatCtx_->streams[streamIndex]->codecpar;
}

AVStream* MediaFileReader::getStream(int streamIndex) {
    if (!formatCtx_ || streamIndex < 0 || 
        static_cast<unsigned int>(streamIndex) >= formatCtx_->nb_streams) {
        return nullptr;
    }

    return formatCtx_->streams[streamIndex];
}

bool MediaFileReader::seek(int64_t timestamp, int streamIndex, int flags) {
    if (!formatCtx_ || streamIndex < 0) {
        return false;
    }

    int ret = av_seek_frame(formatCtx_, streamIndex, timestamp, flags);
    return ret >= 0;
}

bool MediaFileReader::seekToTime(double timeSeconds, int streamIndex, int flags) {
    if (!formatCtx_ || streamIndex < 0 || 
        static_cast<unsigned int>(streamIndex) >= formatCtx_->nb_streams) {
        return false;
    }

    AVStream* stream = formatCtx_->streams[streamIndex];
    if (!stream) {
        return false;
    }

    // Convert seconds to stream time_base
    AVRational timeBaseQ = {1, AV_TIME_BASE};
    int64_t timestamp = av_rescale_q(
        static_cast<int64_t>(timeSeconds * AV_TIME_BASE),
        timeBaseQ,
        stream->time_base
    );

    return seek(timestamp, streamIndex, flags);
}

bool MediaFileReader::seekStreamsToTime(double timeSeconds, const std::vector<int>& streamIndices, int flags) {
    if (!formatCtx_ || streamIndices.empty()) {
        return false;
    }

    // Seek all streams to the same time
    bool allSucceeded = true;
    for (int streamIndex : streamIndices) {
        if (!seekToTime(timeSeconds, streamIndex, flags)) {
            allSucceeded = false;
        }
    }

    return allSucceeded;
}

int MediaFileReader::readPacket(AVPacket* packet) {
    if (!formatCtx_ || !packet) {
        return AVERROR(EINVAL);
    }

    return av_read_frame(formatCtx_, packet);
}

double MediaFileReader::getDuration() const {
    if (!formatCtx_ || formatCtx_->duration == AV_NOPTS_VALUE) {
        return 0.0;
    }

    return static_cast<double>(formatCtx_->duration) / AV_TIME_BASE;
}

unsigned int MediaFileReader::getStreamCount() const {
    if (!formatCtx_) {
        return 0;
    }
    return formatCtx_->nb_streams;
}

} // namespace cuems_mediadecoder

