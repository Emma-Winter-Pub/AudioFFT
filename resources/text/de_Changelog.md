### Änderungsprotokoll

---
**V1.3    20260725**

**Neu**
*   Einige FFT-Fensterfunktionen hinzugefügt.
*   Einige dBFS-Farbpaletten hinzugefügt.
*   Funktion zum Umkehren der Farbleistenrichtung hinzugefügt.
*   Funktion für negative Farben (Farbumkehrung) hinzugefügt.
*   Indexiertes Farbformat für PNG- und BMP-Bilder hinzugefügt.
*   Funktion für benutzerdefinierte Farbleisten (Custom Color Palettes) hinzugefügt.
*   System-Mediensteuerung für die Windows 10/11-Plattform hinzugefügt.
*   System-Mediensteuerung für die Linux-Plattform hinzugefügt.
*   Modul zur Analyse der Speicherstruktur für die Linux-Plattform hinzugefügt.
*   Installation des „.desktop“-Starters für die Linux-Plattform hinzugefügt.
*   Erkennung der Zeichenkodierung (Character Encoding Detection) hinzugefügt.
*   Funktion zum automatischen Erweitern des Spektrogramms bei der Wiedergabe hinzugefügt.

**Optimierungen**
*   Farbpaletten-Modul refaktorisiert.
*   Einige Standardparameter angepasst.
*   Fensterstapelungs-Eigenschaften (Window Stacking) unter Linux angepasst.
*   Audio-Zuordnungslogik für CUE-Dateien angepasst.
*   Build-Konfigurationen für Kompatibilität mit MSVC- und GCC-Compilern angepasst.
*   Lebenszyklus der Stopp-Operation des Players vom Zurücksetzen des Fortschritts entkoppelt.
*   Globale FFTW-Ressourcenbereinigungsroutinen ergänzt.
*   Mechanismus zur expliziten Speicherfreigabe für die Linux-Plattform ergänzt.
*   Parsen externer Dateipfade unter Linux optimiert.
*   libpng-Exportalgorithmus optimiert.
*   FFT-Algorithmus optimiert.
*   Frequenzbereichs-Mapping-Algorithmus optimiert.
*   Verschachtelungsreihenfolge der Schleifen beim Spektrogramm-Rendering optimiert.
*   Speicherverwaltung des Audio-Resamplers optimiert.
*   Speicher-Vorabzuweisungslogik (Pre-Allocation) für die Mehrthread-Decodierung optimiert.
*   Datenübertragung im CPU-Rendering-Modus optimiert.
*   Aktualisierung des GPU-Vertex-Puffers optimiert.
*   Konstruktionslogik dynamischer Vertices auf der GPU optimiert.

**Fehlerbehebungen**
*   Fehler behoben, bei dem das Spektrum-Profil immer den ersten Frame zeichnete.
*   Fehler behoben, bei dem der Player die Wiedergabe unerwartet beendete.
*   Fehler behoben, bei dem der FFT-Cache nicht automatisch geleert werden konnte.
*   Gefahr von Speicherspitzen (Memory Spikes) beim Zusammenführen von Daten in der Mehrthread-Decodierung behoben.
*   Fehler durch nicht ausgerichteten Speicher (Unaligned Memory) im FFmpeg-Decoder behoben.
*   Data Races und Programmabstürze im Zusammenhang mit FFTW behoben.
*   Speicherleck (Memory Leak) im PNG-Encoder behoben.
*   Unkontrollierbare Abstürze im PNG-Fehler-Callback behoben.
*   Absturzrisiko durch Dangling-Pointer (hängende Zeiger) in der Screenshot-Funktion behoben.
*   Fehler in der COM-Bewertungslogik innerhalb des Speicheranalysemoduls behoben.
*   Division-durch-Null-Fehler in Fensterfunktionen behoben.
*   Problem behoben, bei dem das GPU-Rendering-Modul unter extremen Bedingungen abstürzte.
*   Build-Fehler aufgrund von plattformübergreifenden Inkompatibilitäten behoben.
*   Problem des visuellen Tearing (Bildzerreißen) beim Abspielkopf behoben.
*   Asynchronität von Bild und Ton (Audio-Visual Desync) im Player behoben.
*   Fehler behoben, bei dem Screenshots mit einem visuellen Versatz aufgenommen wurden.
*   Anomalien bei der Ankerpunkterkennung in der parallelen APE-Decodierung behoben.
*   Schwachstelle behoben, bei der die Stapelverarbeitung eine Endlosschleife bei APE-Dateien auslösen konnte.
*   Fehler behoben, bei dem Protokolle während der Stapelverarbeitung ausgelassen wurden.
*   Wiedergabeblockaden beim Wechseln von Arbeitsbereichen unter Linux behoben.
*   Permanenter Deadlock (Verklemmung) behoben, der bei Ausnahmen in der Stapelverarbeitung auftrat.
*   Abstürze durch Speicherzugriffsverletzungen beim Stoppen von Aufgaben oder Beenden der Anwendung behoben.

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
**V1.0    20251104**（bilibili）

**V1.0    20251221**（GitHub）

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