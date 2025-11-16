#ifndef CUEMS_MEDIADECODER_UTILS_H
#define CUEMS_MEDIADECODER_UTILS_H

#include <string>

extern "C" {
#include <libavutil/error.h>
}

namespace cuems_mediadecoder {

/**
 * FFmpeg utility functions
 */
class FFmpegUtils {
public:
    /**
     * Convert FFmpeg error code to string
     * @param errnum FFmpeg error code (negative value)
     * @return Error message string
     */
    static std::string getErrorString(int errnum);
};

} // namespace cuems_mediadecoder

#endif // CUEMS_MEDIADECODER_UTILS_H

