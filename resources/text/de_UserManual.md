## Arbeitsbereich
*   **Volllast**: Lädt die gesamte Audiodatei in den Speicher, um die schnellste Verarbeitungsgeschwindigkeit zu erreichen; geeignet für die Analyse von Dateien normaler Größe.
*   **Streaming**: Liest, verarbeitet und rendert blockweise bei extrem niedrigem Speicherverbrauch; ideal für überlange Audiodateien oder Computer mit begrenzten Ressourcen.
*   **Stapel**: Generiert Spektrogramme in Stapelverarbeitung mit integrierten Dual-Modi für Volllast und Streaming.

## Einzeldatei-Analyse
### Ansichts- und Anzeigesteuerung
*   **Protokoll**: Öffnet ein unabhängiges Protokollfenster, um Dateiinformationen und den Verarbeitungsfortschritt in Echtzeit anzuzeigen.
*   **Raster**: Überlagert Frequenz- und Zeitrasterlinien auf dem Spektrogramm, um die Ausrichtung und das Ablesen zu erleichtern.
*   **Beschriftungen**: Aktiviert periphere Informationen (Achsen, Titel usw.) um das Spektrogramm herum.
*   **ZoomHz**: Entsperrt das Zoomen der Frequenzachse.
*   **MaxB (Max-Breite)**: Begrenzt die maximale Pixelbreite des exportierten Bildes. Wird bei Überschreitung der Breitenbegrenzung automatisch skaliert.

### Audio- und Bildparameter
*   **Stream/Track**: Wechselt den Audiostream in Multi-Stream-Dateien; wenn eine `.cue`-Datei geladen wird, ändert es sich zu **Track** (CUE-Track).
*   **Kan (Kanal)**: Wählt einen spezifischen Kanal aus oder wählt „Mix“, um alle Kanäle zu mischen.
*   **H (Höhe)**: Legt die vertikale Pixelhöhe des Bildes fest.
*   **Genau (Genauigkeit)**: Legt die horizontale Zeitauflösung fest (Sekunden/Pixel). Wenn auf „Auto“ gesetzt, beträgt die Fensterüberlappungsrate 0 %.
*   **Fen (Fenster)**: Wählt die Fensterfunktion für die Fourier-Transformation, um spektrale Leckage und Nebenkeulendämpfung zu kontrollieren.
*   **Map (Mapping)**: Wählt die Skalierungskurve für die Frequenzachse (Y-Achse), um sich leicht auf bestimmte Frequenzbänder zu konzentrieren.
*   **Pal (Palette)**: Wählt das Farbschema für die spektrale Energie aus.
*   **dB**: Legt die oberen und unteren Grenzen (dB) der abgebildeten Energie fest.

*   **Öffnen**: Öffnet Dateien, die Audiostreams enthalten, sowie CUE-Dateien.
*   **Speichern**: Exportiert das Spektrogramm als Bild, mit anpassbarer Komprimierungsstufe und Bildqualität.

## Stapelverarbeitung
*   **Eingabepfad/Ausgabepfad**: Legt den Quellordner und den Zielordner fest.
*   **Einstellungen**: Öffnet das Konfigurationszentrum für Stapel-Tasks. Sie können den **Modus** (Volllast / Streaming), die **Threads**, die FFT-Parameter, das Bildexportformat, die Bildqualität und andere Optionen festlegen.
*   **Task-Steuerung**: Steuert den Task über die Schaltflächen **Task starten**, **Task pausieren / Task fortsetzen** und **Task beenden**.