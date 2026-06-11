### Changelog

---
**V1.2    20260610**

**New**
*   Added storage device analysis module.
*   Added virtualized log list view.
*   Added periodic notifications during the batch processing scan phase.
*   Added file extension filtering mechanism for batch processing scans.
*   Added detection and logging of abnormal files during batch processing.
*   Added option to exclude video files in batch processing.
*   Added option to categorize output by encoding type in batch processing.
*   Added horizontal layout direction for the spectrum profile.
*   Added frame rate synchronization between the spectrum profile and the playhead.
*   Added option to allow multiple instances to run concurrently.
*   Added option to auto-play when opening files via OS file associations.
*   Added option to select the default workspace on startup.

**Optimizations**
*   Optimized the accuracy and fault tolerance of the CUE parser.
*   Optimized the rendering smoothness of the log window.
*   Optimized the underlying time synchronization mechanism of the audio player.
*   Optimized the management strategy for background asynchronous tasks and thread lifecycles.
*   Optimized memory safety allocation checks during spectrogram rendering.
*   Optimized the invocation timing of the storage device analysis.
*   Optimized the settings logging for batch processing.

**Fixes**
*   Fixed issue with the audio track number display.
*   Fixed CUE parsing errors.
*   Fixed issue where residual data was not cleaned up upon dedicated decoding pipeline failure.
*   Fixed potential memory leak in the JPEG image encoder.
*   Fixed underlying API access violation when the audio output device is unplugged during playback.
*   Fixed infinite layout response loop that occurred when window or control sizes did not actually change.
*   Fixed lifecycle and backpressure synchronization issues in the batch processing asynchronous write queue.
*   Fixed issue where the single-sided Fourier transform lacked double-sided energy compensation, resulting in overall lower spectral energy calculations.
*   Fixed severe memory leak and double-free issue caused by improper cleanup of FFmpeg custom I/O streams.
*   Fixed crash caused by background asynchronous threads capturing dangling pointers when switching or closing windows during image export.
*   Fixed save failure when exporting TIFF and JPEG 2000 formats on Windows due to a lack of Unicode path support.
*   Fixed potential out-of-bounds memory crash on Windows when capturing screenshots with the mouse cursor, caused by missing validation of underlying API return values.
*   Fixed issue where the spectrum overlay might display incorrectly under GPU hardware acceleration due to improper OpenGL context format initialization timing.
*   Fixed heap memory corruption and application crashes caused by accessing dangling pointers of destroyed objects during batch processing task cleanup.
*   Fixed memory leak during I/O thread initialization.
*   Fixed sequential logic error for disabling UI buttons during batch processing.
*   Fixed issue where batch processing tasks could not be paused, resumed, or terminated during the scanning phase.
*   Fixed output path calculation errors during batch processing.
*   Fixed issue where the batch processing view incorrectly responded to the spacebar.

---
**V1.1    20260328**

**New**
*   Added streaming processing.
*   Added multi-threaded decoding for FLAC, ALAC, and DSD formats.
*   Added adaptive 32/64-bit floating-point computation precision.
*   Added dynamic memory loading strategy for full mode.
*   Added track switching.
*   Added support for opening CUE files.
*   Added CUE split-track switching.
*   Added channel switching.
*   Added FFT window function selection.
*   Added spectrogram color scheme selection.
*   Added spectrogram dB value adjustment.
*   Added caching mechanism for Fourier transform computation results.
*   Added duplicate task reminder for batch processing.
*   Added player with latency compensation.
*   Added adjustable crosshair cursor.
*   Added probe with switchable data source.
*   Added frequency distribution graph display.
*   Added GPU hardware acceleration.
*   Added component show/hide control.
*   Added frame rate adjustment.
*   Added I/O scheduling for batch processing.
*   Added screenshot functionality.
*   Added settings panel.
*   Added user configuration saving.
*   Added multi-language support: Simplified Chinese, Traditional Chinese, Japanese, Korean, German, English, French, and Russian.
*   Expanded the range of height values and added original FFT point-to-point resolution values.
*   Expanded the range of time precision values and added automatic zero-overlap rate.
*   Expanded the number of mapping functions.

**Optimizations**
*   Optimized audio decoding speed.
*   Optimized Fourier transform speed.
*   Optimized spectrogram rendering speed.
*   Optimized log content and layout.
*   Optimized the logic and smoothness of spectrogram zooming and panning.
*   Changed the user interface to Ribbon style.

**Fixes**
*   Fixed errors in multi-threaded decoding for APE format.
*   Fixed inaccurate audio duration display for some files.
*   Fixed FFmpeg resource leaks.
*   Fixed program crashes caused by thread contention.
*   Fixed program crashes caused by Fourier transform during batch processing.
*   Fixed save failures in batch processing when image size exceeded format limits.

---
**V1.0    20251221**

*   Supports two working modes: single-file and batch processing.
*   Supports the vast majority of common audio formats.
*   Spectrogram supports panning and zooming.
*   Preset multiple frequency mapping functions.
*   Spectrogram height and time precision can be adjusted.
*   Provides grid for easy alignment and viewing.
*   Supports exporting to multiple image formats.
*   Exported images allow adjustment of quality and compression ratio.
*   Supports custom maximum image width.
*   Provides log viewing.