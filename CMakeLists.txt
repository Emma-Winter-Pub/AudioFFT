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

find_package(Qt6 COMPONENTS Widgets Concurrent Multimedia Network LinguistTools OpenGLWidgets DBus REQUIRED)

if(WIN32)
    find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avutil swresample)
    set(FFMPEG_LIBRARIES FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swresample)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswresample libavutil)
    set(FFMPEG_LIBRARIES PkgConfig::FFMPEG)
endif()

find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(OpenJPEG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(libavif CONFIG REQUIRED)
find_package(AOM CONFIG REQUIRED)
find_package(libyuv CONFIG REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(PALETTE_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_palettes.py")
set(GENERATED_PALETTE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/GeneratedPalettes.h")
set(PALETTE_DIR_SCI "${CMAKE_CURRENT_SOURCE_DIR}/resources/color/scientific")
set(PALETTE_DIR_LIN "${CMAKE_CURRENT_SOURCE_DIR}/resources/color/linear")

file(GLOB_RECURSE PALETTE_FILES "${PALETTE_DIR_SCI}/*.txt" "${PALETTE_DIR_LIN}/*.txt")

add_custom_command(
    OUTPUT "${GENERATED_PALETTE_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${PALETTE_GEN_SCRIPT}" "${GENERATED_PALETTE_HEADER}" "${PALETTE_DIR_SCI}" "${PALETTE_DIR_LIN}"
    DEPENDS "${PALETTE_GEN_SCRIPT}" ${PALETTE_FILES}
    COMMENT "generate_palettes.py: resources/color -> src/GeneratedPalettes.h"
)

set(APP_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/AboutDialog.cpp
    src/AboutDialog.h
    src/AlignedAllocator.h
    src/AppConfig.h
    src/AudioCodecWeights.h
    src/AudioDecoderAPE.cpp
    src/AudioDecoderAPE.h
    src/AudioDecoderDSD.cpp
    src/AudioDecoderDSD.h
    src/AudioDecoderFactory.cpp
    src/AudioDecoderFactory.h
    src/AudioDecoderFLAC.cpp
    src/AudioDecoderFLAC.h
    src/AudioDecoderTypes.h
    src/AudioExtensionList.h
    src/AudioExtensionWhitelistDialog.cpp
    src/AudioExtensionWhitelistDialog.h
    src/BatchFullLoadAudioDecoder.cpp
    src/BatchFullLoadAudioDecoder.h
    src/BatchFullLoadConfig.h
    src/BatchFullLoadFFmpegRAII.h
    src/BatchFullLoadFFTProcessor.cpp
    src/BatchFullLoadFFTProcessor.h
    src/BatchFullLoadImageExporter.cpp
    src/BatchFullLoadImageExporter.h
    src/BatchFullLoadIoScheduler.cpp
    src/BatchFullLoadIoScheduler.h
    src/BatchFullLoadProcessor.cpp
    src/BatchFullLoadProcessor.h
    src/BatchFullLoadSpectrogramGenerator.cpp
    src/BatchFullLoadSpectrogramGenerator.h
    src/BatchFullLoadTypes.h
    src/BatchFullLoadUtils.h
    src/BatchFullLoadWidget.cpp
    src/BatchFullLoadWidget.h
    src/BatchFullLoadWidgetSettings.cpp
    src/BatchFullLoadWidgetSettings.h
    src/BatchFullLoadWorker.cpp
    src/BatchFullLoadWorker.h
    src/BatchStreamBuffer.cpp
    src/BatchStreamBuffer.h
    src/BatchStreamDecoder.cpp
    src/BatchStreamDecoder.h
    src/BatchStreamFFmpegRAII.h
    src/BatchStreamFFT.cpp
    src/BatchStreamFFT.h
    src/BatchStreamFFTRAII.h
    src/BatchStreamGenerator.cpp
    src/BatchStreamGenerator.h
    src/BatchStreamIoScheduler.cpp
    src/BatchStreamIoScheduler.h
    src/BatchStreamPainter.cpp
    src/BatchStreamPainter.h
    src/BatchStreamProcessor.cpp
    src/BatchStreamProcessor.h
    src/BatchStreamTypes.h
    src/BatchStreamUtils.h
    src/BatchStreamWorker.cpp
    src/BatchStreamWorker.h
    src/ColorPaletteEntity.cpp
    src/ColorPaletteEntity.h
    src/ColorPaletteFactory.cpp
    src/ColorPaletteFactory.h
    src/ColorPaletteOld.cpp
    src/ColorPaletteOld.h
    src/ColorPaletteParser.cpp
    src/ColorPaletteParser.h
    src/ColorUserPaletteLoader.cpp
    src/ColorUserPaletteLoader.h
    src/CrosshairConfig.cpp
    src/CrosshairConfig.h
    src/CrosshairIndicator.cpp
    src/CrosshairIndicator.h
    src/CrosshairOverlay.cpp
    src/CrosshairOverlay.h
    src/CueParser.cpp
    src/CueParser.h
    src/FFmpegMemLoader.cpp
    src/FFmpegMemLoader.h
    src/FFT32RAII.h
    src/FFT64RAII.h
    src/FFTSpectrumDataProvider.cpp
    src/FFTSpectrumDataProvider.h
    src/FFTStrategyFactory.cpp
    src/FFTStrategyFactory.h
    src/FFTTypes.h
    src/FFTW3_FP32_1T_nF.cpp
    src/FFTW3_FP32_1T_nF.h
    src/FFTW3_FP32_nT_nF.cpp
    src/FFTW3_FP32_nT_nF.h
    src/FFTW3_FP64_1T_nF.cpp
    src/FFTW3_FP64_1T_nF.h
    src/FFTW3_FP64_nT_nF.cpp
    src/FFTW3_FP64_nT_nF.h
    src/FFTWGlobalLock.cpp
    src/FFTWGlobalLock.h
    src/FFTWindowFunctions.cpp
    src/FFTWindowFunctions.h
    src/FullLoadAudioDecoder.cpp
    src/FullLoadAudioDecoder.h
    src/FullLoadSpectrogramGenerator.cpp
    src/FullLoadSpectrogramGenerator.h
    src/FullLoadSpectrogramPainter.cpp
    src/FullLoadSpectrogramPainter.h
    src/FullLoadSpectrogramProcessor.cpp
    src/FullLoadSpectrogramProcessor.h
    src/FullLoadSpectrogramViewer.cpp
    src/FullLoadSpectrogramViewer.h
    src/FullLoadUtils.h
    src/FullLoadWidget.cpp
    src/FullLoadWidget.h
    src/GlobalPreferences.cpp
    src/GlobalPreferences.h
    src/GlobalSettingsDialog.cpp
    src/GlobalSettingsDialog.h
    src/GlobalSettingsDialogUI.cpp
    src/GlobalSettingsDialogUI.h
    src/ImageEncoderAvif.cpp
    src/ImageEncoderAvif.h
    src/ImageEncoderBmpQt.cpp
    src/ImageEncoderBmpQt.h
    src/ImageEncoderFactory.cpp
    src/ImageEncoderFactory.h
    src/ImageEncoderJpeg.cpp
    src/ImageEncoderJpeg.h
    src/ImageEncoderJpeg2000.cpp
    src/ImageEncoderJpeg2000.h
    src/ImageEncoderPngLibpng.cpp
    src/ImageEncoderPngLibpng.h
    src/ImageEncoderPngQt.cpp
    src/ImageEncoderPngQt.h
    src/ImageEncoderTiff.cpp
    src/ImageEncoderTiff.h
    src/ImageEncoderWebp.cpp
    src/ImageEncoderWebp.h
    src/ImageExportDialog.cpp
    src/ImageExportDialog.h
    src/ImageExporter.cpp
    src/ImageExporter.h
    src/ImageSpectrumDataProvider.cpp
    src/ImageSpectrumDataProvider.h
    src/LogListModel.cpp
    src/LogListModel.h
    src/LogListView.cpp
    src/LogListView.h
    src/MappingCurves.cpp
    src/MappingCurves.h
    src/PlayerAudioSource.cpp
    src/PlayerAudioSource.h
    src/PlayerControlBar.cpp
    src/PlayerControlBar.h
    src/PlayerControlComponents.cpp
    src/PlayerControlComponents.h
    src/PlayerController.cpp
    src/PlayerController.h
    src/PlayerEngine.cpp
    src/PlayerEngine.h
    src/PlayerFfmpegProvider.cpp
    src/PlayerFfmpegProvider.h
    src/PlayerRamProvider.cpp
    src/PlayerRamProvider.h
    src/PlayerResampler.cpp
    src/PlayerResampler.h
    src/RibbonButton.cpp
    src/RibbonButton.h
    src/ScreenshotManager.cpp
    src/ScreenshotManager.h
    src/ScreenshotSaveDialog.cpp
    src/ScreenshotSaveDialog.h
    src/SpectrogramCalculator.cpp
    src/SpectrogramCalculator.h
    src/SpectrogramViewerBase.cpp
    src/SpectrogramViewerBase.h
    src/SpectrogramWidgetBase.cpp
    src/SpectrogramWidgetBase.h
    src/SpectrumOverlay.cpp
    src/SpectrumOverlay.h
    src/SpectrumRendererCpu.cpp
    src/SpectrumRendererCpu.h
    src/SpectrumRendererGpu.cpp
    src/SpectrumRendererGpu.h
    src/StorageCache.cpp
    src/StorageCache.h
    src/StorageClassifier.cpp
    src/StorageClassifier.h
    src/StoragePlatformUtils.cpp
    src/StoragePlatformUtils.h
    src/StorageTopologyAnalyzer.cpp
    src/StorageTopologyAnalyzer.h
    src/StorageTypes.h
    src/StreamingAudioDecoder.cpp
    src/StreamingAudioDecoder.h
    src/StreamingConfig.h
    src/StreamingFFT32.cpp
    src/StreamingFFT32.h
    src/StreamingFFT64.cpp
    src/StreamingFFT64.h
    src/StreamingSpectrogramGenerator.cpp
    src/StreamingSpectrogramGenerator.h
    src/StreamingSpectrogramProcessor.cpp
    src/StreamingSpectrogramProcessor.h
    src/StreamingSpectrogramViewer.cpp
    src/StreamingSpectrogramViewer.h
    src/StreamingTypes.h
    src/StreamingUtils.h
    src/StreamingWidget.cpp
    src/StreamingWidget.h
    src/TextEncodingHelper.cpp
    src/TextEncodingHelper.h
    src/XAudioDecoder.h
    src/XColorPalette.h
    src/XFullLoadFFTEngine.h
    src/XImageEncoder.h
    src/XPlayerProvider.h
    src/XPlayerSystemControls.h
    src/XSpectrumDataProvider.h
    src/XSpectrumRenderer.h
    src/XStorageBackend.h

    ${GENERATED_PALETTE_HEADER}

    resources/resources.qrc
    resources/app_icon.rc
)

if(WIN32)
    list(APPEND APP_SOURCES
        src/PlayerWindowsSystemControls.cpp
        src/PlayerWindowsSystemControls.h
        src/StorageWindowsBackend.cpp
        src/StorageWindowsBackend.h
        src/StorageWindowsIoctlHelper.cpp
        src/StorageWindowsIoctlHelper.h
        src/StorageWindowsNetworkHelper.cpp
        src/StorageWindowsNetworkHelper.h
        src/StorageWindowsWmiHelper.cpp
        src/StorageWindowsWmiHelper.h
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND APP_SOURCES
        src/PlayerLinuxMprisControls.cpp
        src/PlayerLinuxMprisControls.h
        src/StorageLinuxBackend.cpp
        src/StorageLinuxBackend.h
        src/StorageLinuxTypes.h
        src/StorageLinuxMountHelper.cpp
        src/StorageLinuxMountHelper.h
        src/StorageLinuxTopologyHelper.cpp
        src/StorageLinuxTopologyHelper.h
        src/StorageLinuxAttributeHelper.cpp
        src/StorageLinuxAttributeHelper.h
    )
elseif(APPLE)
    list(APPEND APP_SOURCES
        src/StorageMacOSBackend.cpp
        src/StorageMacOSBackend.h
    )
endif()

set(I18N_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/translations")
file(MAKE_DIRECTORY ${I18N_DIR})

set(TS_FILES
    "${I18N_DIR}/AudioFFT_zh-JT.ts"
    "${I18N_DIR}/AudioFFT_zh-FT.ts"
    "${I18N_DIR}/AudioFFT_ja.ts"
    "${I18N_DIR}/AudioFFT_ko.ts"
    "${I18N_DIR}/AudioFFT_de.ts"
    "${I18N_DIR}/AudioFFT_en.ts"
    "${I18N_DIR}/AudioFFT_fr.ts"
    "${I18N_DIR}/AudioFFT_ru.ts"
)

qt6_create_translation(QM_FILES ${APP_SOURCES} ${TS_FILES})

# qt6_create_translation(QM_FILES ${APP_SOURCES} ${TS_FILES} OPTIONS -no-obsolete)

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
    ${JPEG_INCLUDE_DIRS}
    ${FFmpeg_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    ${OpenJPEG_INCLUDE_DIRS}
    ${WebP_INCLUDE_DIRS}
    ${libavif_INCLUDE_DIRS}
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(LINK_GROUP_START "-Wl,--start-group")
    set(LINK_GROUP_END "-Wl,--end-group")
endif()

target_link_libraries(AudioFFT PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Network
    Qt6::OpenGLWidgets
    Qt6::DBus
    ${LINK_GROUP_START}
    FFTW3::fftw3
    FFTW3::fftw3f
    ${FFMPEG_LIBRARIES}
    avif
    AOM::aom
    yuv
    WebP::webp
    WebP::webpdecoder
    WebP::webpdemux
    WebP::libwebpmux
    WebP::sharpyuv
    openjp2
    TIFF::TIFF
    JPEG::JPEG
    PNG::PNG
    ZLIB::ZLIB
    LibLZMA::LibLZMA
    ${LINK_GROUP_END}
)

if(WIN32)
    target_link_libraries(AudioFFT PRIVATE Mpr Wbemuuid)
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS AudioFFT DESTINATION bin)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/AudioFFT.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/AudioFFT.desktop"
        @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/AudioFFT.desktop"
            DESTINATION share/applications)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/AudioFFT_logo.ico"
            DESTINATION share/pixmaps)
endif()

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)