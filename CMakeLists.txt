cmake_minimum_required(VERSION 3.18)
project(AudioFFT LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_USE_MATH_DEFINES)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Concurrent REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swresample)
find_package(FFTW3 REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libjpeg-turbo REQUIRED)
find_package(TIFF REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(libavif CONFIG REQUIRED)

add_executable(AudioFFT WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/SingleFileWidget.cpp
    src/SpectrogramViewer.cpp
    src/SpectrogramProcessor.cpp
    src/FftProcessor.cpp
    src/AudioDecoder.cpp
    src/MappingCurves.cpp
    src/SpectrogramGenerator.cpp
    src/ParallelDecoder.cpp
    src/StandardDecoder.cpp
    src/ImageExporter.cpp
    src/SpectrogramPainter.cpp
    src/PngImageEncoder.cpp
    src/JpegImageEncoder.cpp
    src/BmpImageEncoder.cpp
    src/QtPngImageEncoder.cpp
    src/WebpImageEncoder.cpp
    src/TiffImageEncoder.cpp
    src/Jp2ImageEncoder.cpp
    src/AvifImageEncoder.cpp
    src/ImageEncoderFactory.cpp
    src/ExportOptionsDialog.cpp
    src/AboutDialog.cpp
    src/BatWidget.cpp
    src/BatProcessor.cpp
    src/BatWorker.cpp
    src/BatAudioDecoder.cpp
    src/BatFftProcessor.cpp
    src/BatSpectrogramGenerator.cpp
    src/BatImageExporter.cpp
    resources/resources.qrc
    resources/app_icon.rc
)

target_include_directories(AudioFFT PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFTW3_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${libjpeg-turbo_INCLUDE_DIRS}
    ${FFmpeg_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    ${OpenJPEG_INCLUDE_DIRS}
    ${WebP_INCLUDE_DIRS}
    ${libavif_INCLUDE_DIRS}
)

target_link_libraries(AudioFFT PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    FFTW3::fftw3
    FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swresample
    PNG::PNG
    ZLIB::ZLIB
    libjpeg-turbo::jpeg
    TIFF::TIFF
    WebP::webp
    openjp2
    avif
)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)