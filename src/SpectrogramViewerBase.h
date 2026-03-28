#pragma once

#include "AppConfig.h"
#include "MappingCurves.h"
#include "ColorPalette.h"
#include "CrosshairOverlay.h"
#include "SpectrumOverlay.h"
#include "CrosshairIndicator.h"
#include "SpectrogramCalculator.h"
#include "ISpectrumDataProvider.h"
#include "ImageSpectrumDataProvider.h"
#include "GlobalPreferences.h"

#include <QWidget>
#include <QPoint>
#include <QElapsedTimer>

class SpectrogramViewerBase : public QWidget {
    Q_OBJECT

public:
    explicit SpectrogramViewerBase(QWidget* parent = nullptr);
    virtual ~SpectrogramViewerBase();
    void setAudioProperties(double duration, const QString& preciseDurationStr, int nativeSampleRate);
    void setPlayheadPosition(double seconds);
    void setPlayheadVisible(bool visible);
    virtual void setShowGrid(bool show);
    virtual void setComponentsVisible(bool visible);
    virtual void setVerticalZoomEnabled(bool enabled);
    virtual void setCurveType(CurveType type);
    virtual void setPaletteType(PaletteType type);
    virtual void setMinDb(double minDb);
    virtual void setMaxDb(double maxDb);
    void setCrosshairStyle(const CrosshairStyle& style);
    void setCrosshairEnabled(bool enabled);
    void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb);
    void setSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type);
    void setProfileFrameRate(int fps);
    void setPlayheadStyle(const PlayheadStyle& style);
    void setGpuEnabled(bool enabled);
    void setExternalDataProvider(ISpectrumDataProvider* provider);
    void setDataConfig(DataSourceType spectrumSource, DataSourceType probeSource, int probePrecision);

signals:
    void fileDropped(const QString& filePath);
    void seekRequested(double seconds);

protected:
    virtual int getContentWidth() const = 0;
    virtual int getContentHeight() const = 0;
    virtual const QImage* getCurrentImage() const = 0;
    virtual int getReferenceWidth() const;
    virtual int getColorIndex(const QPoint& viewPos) const { return 0; }
    QRect getSpectrogramRect() const;
    double getPixelsPerSecond() const;
    double timeToScreenX(double seconds) const;
    double screenXToTime(double x) const;
    void clampViewPosition(); 
    void drawOverlay(QPainter& painter); 
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    int calculateBestFreqStep(double maxFreqKhz, int availableHeight) const;
    int calculateBestDbStep(double dbRange, int availableHeight) const;
    double m_zoomX = 1.0;
    double m_zoomY = 1.0;
    double m_offsetX = 0.0;
    double m_offsetY = 0.0;
    double m_audioDurationSec = 0.0;
    QString m_preciseDurationStr;
    int m_nativeSampleRate = 0;
    bool m_isDraggingView = false;
    bool m_isDraggingPlayhead = false;
    QPoint m_lastMousePos;
    bool m_showGrid = true;
    bool m_showComponents = true;
    bool m_isVerticalZoomEnabled = false;
    bool m_isPlayheadVisible = false;
    double m_playheadTime = 0.0;
    CurveType m_currentCurveType = CurveType::XX;
    PaletteType m_currentPaletteType = PaletteType::S01;
    double m_minDb = AppConfig::DEFAULT_MIN_DB;
    double m_maxDb = AppConfig::DEFAULT_MAX_DB;
    bool m_isViewLocked = false; 
    const int HANDLE_HEIGHT = 16;
    CrosshairOverlay* m_crosshairOverlay = nullptr;
    bool m_isAudioLoaded = false;
    bool m_isCrosshairEnabled = true;
    SpectrogramCalculator m_calculator;
    QPoint m_currentMousePos;
    bool m_isMouseInView = false;
    SpectrumOverlay* m_spectrumOverlay = nullptr;
    ISpectrumDataProvider* m_spectrumProvider = nullptr;
    ImageSpectrumDataProvider* m_imageProvider = nullptr;
    ISpectrumDataProvider* m_externalProvider = nullptr;
    QElapsedTimer m_fpsTimer;
    int m_lastDrawnSpectrumColumn = -1;
    bool m_showSpectrumProfile = true;
    int m_profileFps = 30;
    DataSourceType m_spectrumSource = DataSourceType::ImagePixel;
    DataSourceType m_probeSource = DataSourceType::ImagePixel;
    int m_probeDbPrecision = 1;
    void updateSpectrumOverlayFromTime(double timeSec);

private:
    void updateOverlayDataUnderMouse(bool forceUpdate = false);
};