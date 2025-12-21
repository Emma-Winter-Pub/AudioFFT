### Mode Selection
*   **Single Mode**: Processes a single audio file and provides a spectrogram observation window.
*   **Batch Mode**: Processes a large number of audio files without an observation window.

### Single Mode
*   **Log**: Check this box to open the log window. It displays detailed information about the audio file and every step of the processing workflow.
*   **Grid**: Check this box to display time and frequency grid lines on the spectrogram. This setting also applies to the exported image.
*   **Zoom Hz**: By default, the mouse wheel scales only the time axis (horizontal). Checking this option allows the wheel to scale both the time axis and the frequency axis (vertical) simultaneously.
*   **Limit Width**: (Effective only when saving images).
    *   **Checkbox**: Enables the maximum width limitation.
    *   **Spinbox**: Sets the maximum width of the image (Range: 500-10000 pixels). If the generated spectrogram exceeds this width, the program will automatically scale it down to this value during export.
*   **Height**: Sets the vertical resolution (image height in pixels) of the generated spectrogram. Higher values reveal more frequency details but increase memory usage and computation time.
*   **Time Precision**: Sets the time precision (in seconds) of the spectrogram. Smaller values (e.g., 0.01s) generate wider images with richer time details but significantly increase memory usage and computation time.
*   **Open**: Opens the file selection dialog to choose an audio file for processing.
*   **Save**: Opens the "Export Image" dialog. You can select the save location, filename, export format, and adjust "Compression/Quality". Click Save to export the spectrogram with current settings. Some formats have maximum resolution limits.

*   **Spectrogram Viewer**:
    *   **Analyze**: Open an audio file or drag and drop a file into the dark area to start analysis.
    *   **Move**: Hold the left mouse button and drag on the spectrogram to move the view.
    *   **Zoom**: Scroll the mouse wheel to zoom in or out.

### Batch Mode
*   **Input Path / Output Path**: Select the source folder containing audio files and the destination folder for spectrograms. Paths are displayed in the text area.
*   **Scan Subfolders**: If checked, the program will scan subdirectories within the input path.
*   **Keep Structure**: If checked, the directory structure of the input path will be replicated in the output path.
*   **Output Format**: Select the image format for export.
    *   **Compression/Quality**: Appears for specific formats to adjust export parameters.
*   **Grid**: If checked, grid lines will be drawn on the exported images.
*   **Limit Width**:
    *   **Checkbox**: Enables the maximum width limitation.
    *   **Spinbox**: Sets the maximum width (Range: 500-10000 pixels). Images exceeding this width will be scaled down.
*   **Height**: Sets the vertical resolution of the spectrograms.
*   **Time Precision**: Sets the time precision of the spectrograms.
*   **Log Window**: Displays detailed information about the batch processing task.

### Troubleshooting Crashes
*   **Out of Memory**: Processing extremely long audio files or using very high parameters (high "Height", small "Time Precision") may exhaust system memory and cause a crash. Reduce parameters or split the audio file.
*   **Compatibility**: Developed for Windows 10/11 x64. Incompatible environments may cause crashes.
*   **APE Format**: "Single Mode" uses multi-threading for APE format decoding, which may cause crashes in rare cases.
*   **Batch Mode**: Thread safety is enhanced, but processing a large number of files with complex codecs (like APE) concurrently may still carry a risk of crashing.

### Notes
*   **APE Format**: In "Single Mode", APE decoding is multi-threaded, which results in stitching seams in the spectrogram equal to the number of CPU threads. These seams contain artifacts. "Batch Mode" uses single-threaded decoding for APE, ensuring no stitching seams.