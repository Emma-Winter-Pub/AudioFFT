### Änderungsprotokoll

---
**V1.2    20260610**

**Neu**
*   Modul zur Analyse von Speichergeräten hinzugefügt.
*   Virtualisierte Protokolllistenansicht (Log-Ansicht) hinzugefügt.
*   Regelmäßige Benachrichtigungen während der Scan-Phase der Stapelverarbeitung (Batch-Processing) hinzugefügt.
*   Filtermechanismus für Dateierweiterungen bei Scans der Stapelverarbeitung hinzugefügt.
*   Erkennung und Protokollierung anomaler Dateien während der Stapelverarbeitung hinzugefügt.
*   Option zum Ausschließen von Videodateien in der Stapelverarbeitung hinzugefügt.
*   Option zur Kategorisierung der Ausgabe nach Codierungstyp in der Stapelverarbeitung hinzugefügt.
*   Horizontale Layout-Ausrichtung für das Spektrum-Profil hinzugefügt.
*   Bildraten-Synchronisation zwischen dem Spektrum-Profil und dem Abspielkopf (Playhead) hinzugefügt.
*   Option für die gleichzeitige Ausführung mehrerer Instanzen hinzugefügt.
*   Option zur automatischen Wiedergabe beim Öffnen von Dateien über OS-Dateizuordnungen hinzugefügt.
*   Option zur Auswahl des Standard-Arbeitsbereichs (Workspace) beim Start hinzugefügt.

**Optimierungen**
*   Genauigkeit und Fehlertoleranz des CUE-Parsers optimiert.
*   Rendering-Flüssigkeit des Protokollfensters optimiert.
*   Zugrundeliegenden Zeitsynchronisationsmechanismus des Audio-Players optimiert.
*   Verwaltungsstrategie für asynchrone Hintergrundaufgaben und Thread-Lebenszyklen optimiert.
*   Sicherheitsprüfungen bei der Speicherzuweisung (Memory Allocation) während des Spektrogramm-Renderings optimiert.
*   Aufrufzeitpunkt der Speichergeräteanalyse optimiert.
*   Protokollierung der Einstellungen für die Stapelverarbeitung optimiert.

**Fehlerbehebungen**
*   Problem mit der Anzeige der Audiospurnummer behoben.
*   CUE-Parsing-Fehler behoben.
*   Problem behoben, bei dem Restdaten nach einem Fehler in der dedizierten Dekodierungs-Pipeline nicht bereinigt wurden.
*   Potenzielles Speicherleck (Memory Leak) im JPEG-Bild-Encoder behoben.
*   Zugriffsverletzung (Access Violation) der zugrundeliegenden API beim Trennen des Audio-Ausgabegeräts während der Wiedergabe behoben.
*   Endlose Layout-Antwort-Schleife behoben, die auftrat, wenn sich die Fenster- oder Steuerelementgrößen nicht tatsächlich änderten.
*   Lebenszyklus- und Backpressure-Synchronisationsprobleme in der asynchronen Schreibwarteschlange der Stapelverarbeitung behoben.
*   Problem behoben, bei dem der einseitigen Fourier-Transformation die zweiseitige Energiekompensation fehlte, was zu insgesamt niedrigeren Berechnungen der Spektralenergie führte.
*   Schwerwiegendes Speicherleck und Double-Free-Problem behoben, verursacht durch unsachgemäße Bereinigung von benutzerdefinierten FFmpeg-I/O-Streams.
*   Absturz behoben, der durch asynchrone Hintergrund-Threads verursacht wurde, die beim Wechseln oder Schließen von Fenstern während des Bildexports Dangling-Pointer (hängende Zeiger) erfassten.
*   Speicherfehler beim Exportieren von TIFF- und JPEG-2000-Formaten unter Windows aufgrund fehlender Unterstützung für Unicode-Pfade behoben.
*   Potenziellen Out-of-Bounds-Speicherabsturz unter Windows bei der Erstellung von Screenshots mit dem Mauszeiger behoben, verursacht durch fehlende Validierung von Rückgabewerten der zugrundeliegenden API.
*   Problem behoben, bei dem das Spektrum-Overlay unter GPU-Hardwarebeschleunigung aufgrund eines falschen Timings bei der Initialisierung des OpenGL-Kontextformats möglicherweise fehlerhaft angezeigt wurde.
*   Heap-Speicherbeschädigung und Anwendungsabstürze behoben, die durch den Zugriff auf Dangling-Pointer zerstörter Objekte während der Bereinigung von Stapelverarbeitungsaufgaben verursacht wurden.
*   Speicherleck bei der Initialisierung von I/O-Threads behoben.
*   Fehler in der sequenziellen Logik zum Deaktivieren von UI-Schaltflächen während der Stapelverarbeitung behoben.
*   Problem behoben, bei dem Aufgaben der Stapelverarbeitung während der Scan-Phase nicht pausiert, fortgesetzt oder beendet werden konnten.
*   Berechnungsfehler des Ausgabepfads während der Stapelverarbeitung behoben.
*   Problem behoben, bei dem die Stapelverarbeitungsansicht falsch auf die Leertaste reagierte.

---
**V1.1    20260328**

**Neu**
*   Streaming-Verarbeitung hinzugefügt.
*   Mehrthread-Decodierung für FLAC-, ALAC- und DSD-Formate hinzugefügt.
*   Adaptive 32/64-Bit-Gleitkomma-Berechnungspräzision hinzugefügt.
*   Dynamische Speicherlade-Strategie für den Vollmodus hinzugefügt.
*   Spurwechsel hinzugefügt.
*   Unterstützung für das Öffnen von CUE-Dateien hinzugefügt.
*   CUE-Split-Track-Wechsel hinzugefügt.
*   Kanalwechsel hinzugefügt.
*   Auswahl der FFT-Fensterfunktion hinzugefügt.
*   Auswahl des Farbschemas für das Spektrogramm hinzugefügt.
*   Anpassung des dB-Werts für das Spektrogramm hinzugefügt.
*   Caching-Mechanismus für Fourier-Transformations-Berechnungsergebnisse hinzugefügt.
*   Erinnerung an doppelte Aufgaben für die Stapelverarbeitung hinzugefügt.
*   Player mit Latenzkompensation hinzugefügt.
*   Anpassbarer Fadenkreuz-Cursor hinzugefügt.
*   Sonde mit umschaltbarer Datenquelle hinzugefügt.
*   Anzeige des Frequenzverteilungsdiagramms hinzugefügt.
*   GPU-Hardware-Beschleunigung hinzugefügt.
*   Steuerung zum Ein-/Ausblenden von Komponenten hinzugefügt.
*   Anpassung der Bildwiederholrate hinzugefügt.
*   I/O-Planung für die Stapelverarbeitung hinzugefügt.
*   Screenshot-Funktion hinzugefügt.
*   Einstellungspanel hinzugefügt.
*   Speichern der Benutzerkonfiguration hinzugefügt.
*   Mehrsprachige Unterstützung hinzugefügt: Vereinfachtes Chinesisch, Traditionelles Chinesisch, Japanisch, Koreanisch, Deutsch, Englisch, Französisch und Russisch.
*   Bereich der Höhenwerte erweitert und originale FFT-Punkt-zu-Punkt-Auflösungswerte hinzugefügt.
*   Bereich der Zeitpräzisionswerte erweitert und automatische Null-Überlappungsrate hinzugefügt.
*   Anzahl der Mapping-Funktionen erweitert.

**Optimierungen**
*   Geschwindigkeit der Audio-Decodierung optimiert.
*   Geschwindigkeit der Fourier-Transformation optimiert.
*   Geschwindigkeit der Spektrogramm-Rendering optimiert.
*   Inhalt und Layout des Logs optimiert.
*   Logik und Flüssigkeit des Zooms und Schiebens im Spektrogramm optimiert.
*   Benutzeroberfläche auf Ribbon-Stil geändert.

**Fehlerbehebungen**
*   Fehler in der Mehrthread-Decodierung für das APE-Format behoben.
*   Ungenaue Anzeige der Audiodauer für einige Dateien behoben.
*   Ressourcenlecks von FFmpeg behoben.
*   Programmabstürze durch Thread-Konkurrenz behoben.
*   Programmabstürze durch Fourier-Transformation während der Stapelverarbeitung behoben.
*   Speicherfehler in der Stapelverarbeitung behoben, wenn die Bildgröße die Formatgrenzen überschritten hat.

---
**V1.0    20251221**

*   Unterstützt zwei Arbeitsmodi: Einzeldatei und Stapelverarbeitung.
*   Unterstützt die überwiegende Mehrheit der gängigen Audioformate.
*   Spektrogramm unterstützt Schwenken und Zoomen.
*   Mehrere voreingestellte Frequenz-Mapping-Funktionen.
*   Höhe des Spektrogramms und Zeitpräzision können angepasst werden.
*   Bietet Raster für einfache Ausrichtung und Betrachtung.
*   Unterstützt den Export in mehrere Bildformate.
*   Exportierte Bilder erlauben die Anpassung von Qualität und Kompressionsverhältnis.
*   Unterstützt benutzerdefinierte maximale Bildbreite.
*   Bietet Log-Anzeige.