## Workspace
*   **Full Load**: Loads the entire audio file into memory for the fastest processing speed, suitable for analyzing regular-sized files.
*   **Streaming**: Reads, processes, and renders in chunks with extremely low memory usage, ideal for super-long audio files or computers with limited resources.
*   **Batch**: Generates spectrograms in batches, with built-in Full Load and Streaming dual modes.

## Single File Analysis
### View and Display Controls
*   **Log**: Opens an independent log window to display file information and processing progress in real-time.
*   **Grid**: Overlays frequency and time grid lines on the spectrogram to assist with alignment and reading.
*   **Labels**: Enables peripheral information (axes, title, etc.) around the spectrogram.
*   **ZoomHz(Zoom Frequency)**: Unlocks frequency axis zooming.
*   **MaxW(Max Width)**: Limits the maximum pixel width of the exported image. It will be auto-resized when exceeding the width limit.

### Audio and Image Parameters
*   **Stream/Track**: Switches the audio stream in multi-stream files; if a `.cue` file is loaded, it becomes **Track** (CUE Track).
*   **Ch(Channel)**: Selects a specific channel, or choose "Mix" to mix all channels.
*   **H(Height)**: Sets the vertical pixel height of the image.
*   **Prec(Precision)**: Sets the horizontal time resolution (seconds/pixel). When set to "Auto", the window overlap rate is 0%.
*   **Win(Window)**: Selects the window function for the Fourier transform to control spectral leakage and sidelobe attenuation.
*   **Map(Mapping)**: Selects the scaling curve for the frequency axis (Y-axis) to easily focus on specific frequency bands.
*   **Pal(Palette)**: Selects the color theme for the spectral energy.
*   **dB**: Sets the upper and lower limits (dB) of the mapped energy.

*   **Open**: Opens files containing audio streams as well as CUE files.
*   **Save**: Exports the spectrogram as an image, with customizable compression level and image quality.

## Batch Processing
*   **Input/Output Path**: Sets the source folder and destination folder.
*   **Settings**: Opens the batch task configuration center. You can specify the **Mode** (Full Load / Streaming), **Threads**, FFT parameters, image export format, image quality, and other options.
*   **Task Control**: Controls the task via **Start Task**, **Pause Task / Resume Task**, and **Stop Task** buttons.