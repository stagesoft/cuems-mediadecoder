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

