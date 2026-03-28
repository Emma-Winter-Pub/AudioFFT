### Changelog

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