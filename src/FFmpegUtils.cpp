#include "cuems_mediadecoder/FFmpegUtils.h"
#include <cstring>

namespace cuems_mediadecoder {

std::string FFmpegUtils::getErrorString(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}

} // namespace cuems_mediadecoder

