# cuems-mediadecoder - Common FFmpeg module
# Static library for shared FFmpeg operations between cuems-videocomposer and cuems-audioplayer

cmake_minimum_required(VERSION 3.10)

# Find required packages
find_package(PkgConfig REQUIRED)

# FFmpeg libraries (required)
pkg_check_modules(FFMPEG REQUIRED 
    libavformat 
    libavcodec 
    libavutil
)

# Optional: libswscale (for video format conversion)
pkg_check_modules(SWSCALE libswscale)
if(SWSCALE_FOUND)
    add_definitions(-DHAVE_SWSCALE)
endif()

# Optional: libswresample (for audio format conversion)
pkg_check_modules(SWRESAMPLE libswresample)
if(SWRESAMPLE_FOUND)
    add_definitions(-DHAVE_SWRESAMPLE)
endif()

# Source files
set(CUEMS_MEDIADECODER_SOURCES
    src/FFmpegUtils.cpp
    src/MediaFileReader.cpp
    src/VideoDecoder.cpp
    src/AudioDecoder.cpp
)

# Create static library
add_library(cuems-mediadecoder STATIC ${CUEMS_MEDIADECODER_SOURCES})

# Include directories
target_include_directories(cuems-mediadecoder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
)

# Link FFmpeg libraries
target_link_libraries(cuems-mediadecoder PUBLIC
    ${FFMPEG_LIBRARIES}
)

# Link optional libraries
if(SWSCALE_FOUND)
    target_link_libraries(cuems-mediadecoder PUBLIC ${SWSCALE_LIBRARIES})
    target_include_directories(cuems-mediadecoder PUBLIC ${SWSCALE_INCLUDE_DIRS})
endif()

if(SWRESAMPLE_FOUND)
    target_link_libraries(cuems-mediadecoder PUBLIC ${SWRESAMPLE_LIBRARIES})
    target_include_directories(cuems-mediadecoder PUBLIC ${SWRESAMPLE_INCLUDE_DIRS})
endif()

# C++ standard
set_target_properties(cuems-mediadecoder PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Compiler flags
target_compile_options(cuems-mediadecoder PRIVATE
    ${FFMPEG_CFLAGS_OTHER}
)

