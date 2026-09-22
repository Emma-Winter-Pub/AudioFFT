### **TODO**

**1. Add "Separate All Channels" Feature**
Display all channels after separation.
* **FullLoad:** Decode first, store PCM data in memory/disk, then compute FFT and render channel by channel (render the next channel only after the current one completes).
* **Streaming:** Render channels sequentially (or render concurrently by allocating multiple consumers based on the channel count, which eliminates disk head thrashing).

---

**2. Add Data Computation/Analysis Result Display for Selections**
* **"How to select?":** Right-click and drag with the mouse.
* Selection markers must be independent of the playhead rather than sharing the same indicator (similar to Adobe Audition).

---

**3. Allow playback of the selected region only.**

---

**4. Playhead/Pointer Offset (in microseconds)**
Positive values indicate leading/advancing; negative values indicate lagging/delay. Integrate this into Settings and allow manual adjustment directly within the playback control UI.

---

**5. Add Playlist to Playback Controls**
Support distinct modes: Sequential Mode, Shuffle/Random Mode, and Repeat Mode (1 time, Loop/Infinite).

* **Sequential Modes:**
  1. Sequential (In order)
  2. Reverse order
  3. Inward from both ends (Ends to center)
  4. Single track
  5. Random order

* **Shuffle / Random Modes:**
  1. **Queue Shuffle:** Generate a randomized queue and play it from start to finish.
  2. **Independent Random:** Mathematically true random (pure random selection with replacement).
  3. **Dispersed Shuffle:** Songs by the same artist are spread apart and cannot be played consecutively.
  4. **Avoidance Shuffle:** Recently played songs will definitely not reappear within the next *n* tracks.
  5. **Deprecating/Exile Shuffle:** The more frequently an artist is skipped, the lower the probability their songs will appear.
  6. **Damped Shuffle:** After a song finishes or is skipped, its probability drops sharply and gradually recovers over time or with subsequent skips.

* **Repeat Modes:**
  (1, 2, 3 ... Infinite)

* **Playlist Presets:**
  * *"All Played Audio Files"*: Sorted by the time added by default.
  * *"Containing Folder of Current File"*: (with an option in Settings to include subfolders), sorted by filename by default.
  * *"Custom User Playlists"*.

---

**6. Window Title Bar Dark Theme**
The window title bar color currently follows the OS theme, which does not match the application UI. It needs to be forced to a dark theme.

---

**7. Contrast Adjustment Slider**
Consider adding a contrast adjustment slider (along with other adjustments inspired by Adobe Photoshop) strictly for inspection/preview purposes (without affecting the final exported image).

---

**8. AutoCAD-Style Grid Linetypes**
Consider applying AutoCAD-style linetypes to the grid, allowing users to configure color, line width, and line pattern/style.

---

**9. Window Function Display Submodule**
Consider adding a visualization submodule for window functions: display the $\mathrm{sinc}(x)$ window function plot (window envelope, sidelobe leakage).

---

**10. Folder Context Menu: "Export Spectrograms for All Files in This Folder"**
Right-clicking a folder name or anywhere inside the folder opens "Export Spectrograms for All Files in This Folder", invoking streaming batch processing. Parameters should be populated from the top controls of the spectrogram viewer, navigating to the batch workspace with auto-filled parameters (parameters not present in the UI will use default values).

---

**11. Multiple Files Context Menu: "Export Spectrograms for Selected Files"**
Selecting and right-clicking multiple files opens "Export Spectrograms for Selected Files", invoking batch processing for only the selected files (not needed when a single file is selected).

---

**12. Single File Context Menu: "Save Spectrogram for This File"**
Right-clicking a single file opens "Save Spectrogram for This File".

---

**13. UI Labels**
Rename "Processor" to "CPU", and add "Disk Speed".

---

**14. Column Header Details Submenu**
Right-clicking the file list header should show a right-arrow to open a detail configuration submenu.
* For example, for **"File Size"**:
  * **Unit:** `b` / `B`
  * **Base (Radix):** Base-2 (Binary) / Base-10 (Decimal)
  * **Prefix Scale:** None / K / M / G / T (add parentheses at the end to distinguish Base-2 vs. Base-10)
  * **Decimal Point:** `,` / `.`
  * **Separator:** None / Space / `,` / `.`
  * **Grouping Digits:** 3 / 4
  * **Decimal Places:** 0 / 1 / 2 / 3 / 4 (`0` means integer; or allow a `0–9` range)

---

**15. Frequency Distribution Plot "Auto" Orientation**
Add an "Auto" orientation mode for the frequency distribution plot: when selected, enable vertical orientation if height > width; otherwise, use horizontal orientation.

---

**16. External File Opening & Path Synchronization**
When opening a file from an external source, synchronize the file system views (directory tree, navigation bar/breadcrumbs, and file list) and locate the target file.
* Add a "File Manager" tab in Settings with a new option: `Path Synchronization: [x] Enable`.

---

**17. Frequency Distribution Curve Attachment Issue (Wayland Bug)**
The frequency distribution curve is currently implemented via window overlay/snapping, which is a temporary workaround and has a bug on Wayland where overlaying fails.

---

**18. Embedding OpenGL Directly into Qt (Eliminating Overlay Workaround)**
To embed OpenGL directly back into Qt and remove the overlay workaround:

1. Enable top-level composition and transparency in the widget constructor:
   ```cpp
   setAttribute(Qt::WA_AlwaysStackOnTop, true);
   setAttribute(Qt::WA_TranslucentBackground, true);
   ```

2. Explicitly bind a `QSurfaceFormat` with an alpha channel to the widget during construction:
   ```cpp
   QSurfaceFormat fmt;
   fmt.setAlphaBufferSize(8);
   fmt.setRenderableType(QSurfaceFormat::OpenGL);
   setFormat(fmt);
   ```

3. Output premultiplied alpha in the fragment shader (`overlay.frag`):
   ```glsl
   FragColor = vec4(uColor.rgb * uColor.a, uColor.a);
   ```

4. Ensure the alpha channel is cleared to zero upon clear:
   ```cpp
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT);
   ```

---

**19. Multi-Tab File Lists**
Rename the file list tab title to "Current Folder Name" and support multiple file list tabs. Triggered by right-clicking a folder and selecting "Open in New Tab" (requires maintaining separate navigation path histories per tab).

---

**20. Player Bug (Windows Audio Enhancements / PlayFX)**
In Windows Control Panel > Sound Devices, clicking "Enable / Disable / Toggle" Microsoft Audio Enhancements (PlayFX) interrupts playback. Manual resume fails, and playback can only recover by completely stopping and restarting from the beginning.

---

**21. Preview Interface "Sync Parameters" (Unified Parameters)**
Add a "Unified Parameters" toggle in the preview interface. When enabled, all spectrograms will re-run the processing pipeline whenever parameters are adjusted.

---

**22. Viewport Anchor on View Mode Switch**
When switching view modes, use the first visible file in the current viewport as the "anchor" to prevent the viewport position from jumping/drifting erratically.

---

**23. "Read-Only Mode" Toggle in Settings**
Consider adding a "Read-Only Mode" toggle in Settings to control/allow copy, paste, move, and delete operations.

---

**24. Cache Pool Size Controls for BatchPreviewBatchStream (BPBS)**
Add configuration controls (sliders/inputs) in Settings to adjust the cache pool size for BPBS:
1. **Per-Tab Cache Pool Capacity:** Allocate a dedicated cache pool when a new tab is opened, and deallocate/destroy it when the tab is closed.
2. **Total Cache Pool Capacity:** Global limit across all open tabs.
3. **Safety Boundary:** The total capacity must not exceed the physical RAM limit and must reserve adequate memory headroom for the operating system.

---

**25. Add 2 New View Modes to the File List (Extending the 8 Native Windows Explorer View Modes)**
1. **"Height: 512, Width: 1024":** Highly effective for inspecting and comparing spectrogram overviews across all audio files.
2. **"Height: 1024, Width: 512":** Ideal for closely inspecting high-frequency cutoffs (frequency shelf/rolloff).

---

**26. Performance Issue: UI Blocking During Viewport Changes in File List**
When changing the file list viewport (e.g., scrolling/jumping), the interaction between the "time gap mechanism" (throttling/interval scheduler) and BPBS causes severe UI blocking/stuttering. There is currently no better solution or workaround available.
