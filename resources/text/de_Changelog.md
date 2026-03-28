### Änderungsprotokoll

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