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

find_package(Qt6 COMPONENTS Widgets Concurrent Multimedia LinguistTools OpenGLWidgets REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swresample)
find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libjpeg-turbo REQUIRED)
find_package(TIFF REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(libavif CONFIG REQUIRED)

set(APP_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/AboutDialog.cpp
    src/RibbonButton.cpp

    src/SpectrogramWidgetBase.cpp
    src/SpectrogramViewerBase.cpp
    src/CrosshairOverlay.cpp
    src/CrosshairConfig.cpp
    src/GlobalSettingsDialog.cpp
    src/GlobalSettingsDialogUI.cpp
    src/GlobalPreferences.cpp
    src/SpectrumOverlay.cpp
    src/ImageSpectrumDataProvider.cpp
    src/SpectrumRendererCpu.cpp
    src/SpectrumRendererGpu.cpp

    src/GlobalFftwLock.cpp
    src/StorageUtils.cpp
    src/FFmpegMemLoader.cpp
    src/WindowFunctions.cpp
    src/MappingCurves.cpp
    src/ColorPalette.cpp
    src/CrosshairIndicator.cpp
    src/SpectrogramCalculator.cpp
    src/FftSpectrumDataProvider.cpp
    src/CueParser.cpp

    src/ImageEncoderFactory.cpp
    src/PngImageEncoder.cpp
    src/QtPngImageEncoder.cpp
    src/JpegImageEncoder.cpp
    src/BmpImageEncoder.cpp
    src/WebpImageEncoder.cpp
    src/TiffImageEncoder.cpp
    src/Jp2ImageEncoder.cpp
    src/AvifImageEncoder.cpp

    src/SingleFileWidget.cpp
    src/SpectrogramViewer.cpp
    src/SingleFileExportDialog.cpp
    src/SpectrogramProcessor.cpp
    src/SpectrogramGenerator.cpp
    src/SpectrogramPainter.cpp
    src/ImageExporter.cpp
    src/UnifiedAudioDecoder.cpp
    src/PipelineFactory.cpp
    src/ApePipeline.cpp
    src/FlacPipeline.cpp
    src/DsdPipeline.cpp
    src/FftStrategyFactory.cpp
    src/FftPipeline32Single.cpp
    src/FftPipeline32Multi.cpp
    src/FftPipeline64Single.cpp
    src/FftPipeline64Multi.cpp

    src/FastStreamingWidget.cpp
    src/FastSpectrogramViewer.cpp
    src/FastSpectrogramProcessor.cpp
    src/FastSpectrogramGenerator.cpp
    src/FastAudioDecoder.cpp
    src/FastFft32.cpp
    src/FastFft64.cpp

    src/BatWidget.cpp
    src/BatWidgetSettings.cpp
    src/BatIoScheduler.cpp
    src/BatProcessor.cpp
    src/BatWorker.cpp
    src/BatAudioDecoder.cpp
    src/BatFftProcessor.cpp
    src/BatSpectrogramGenerator.cpp
    src/BatImageExporter.cpp

    src/BatchStreamBuffer.cpp
    src/BatchStreamIoScheduler.cpp
    src/BatchStreamProcessor.cpp
    src/BatchStreamWorker.cpp
    src/BatchStreamDecoder.cpp
    src/BatchStreamFft.cpp
    src/BatchStreamGenerator.cpp
    src/BatchStreamPainter.cpp

    src/PlayerEngine.cpp
    src/PlayerControlComponents.cpp
    src/PlayerController.cpp
    src/PlayerControlBar.cpp
    src/PlayerAudioSource.cpp
    src/PlayerRamProvider.cpp
    src/PlayerFfmpegProvider.cpp
    src/PlayerResampler.cpp

    src/ScreenshotSaveDialog.cpp
    src/ScreenshotManager.cpp

    resources/resources.qrc
    resources/app_icon.rc
)

set(I18N_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations")
file(MAKE_DIRECTORY ${I18N_DIR})

set(TS_FILES
    "${I18N_DIR}/AudioFFT_zh-JT.ts"  # 简体中文
    "${I18N_DIR}/AudioFFT_zh-FT.ts"  # 繁體中文
    "${I18N_DIR}/AudioFFT_ja.ts"     # 日本語
    "${I18N_DIR}/AudioFFT_ko.ts"     # 한국어
    "${I18N_DIR}/AudioFFT_de.ts"     # Deutsch
    "${I18N_DIR}/AudioFFT_en.ts"     # English
    "${I18N_DIR}/AudioFFT_fr.ts"     # Français
    "${I18N_DIR}/AudioFFT_ru.ts"     # Русский
)

qt6_create_translation(QM_FILES ${APP_SOURCES} ${TS_FILES})

add_executable(AudioFFT WIN32
    ${APP_SOURCES}
    ${QM_FILES}
)

add_custom_command(TARGET AudioFFT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:AudioFFT>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} $<TARGET_FILE_DIR:AudioFFT>/translations
    COMMENT "Copying translation files to build directory..."
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
    Qt6::Multimedia
    Qt6::OpenGLWidgets
    FFTW3::fftw3
    FFTW3::fftw3f
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