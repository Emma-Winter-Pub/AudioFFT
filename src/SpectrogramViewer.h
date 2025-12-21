#pragma once

#include "MappingCurves.h"

#include <QWidget>
#include <QImage>
#include <QPainterPath>


class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QWheelEvent;
class QPaintEvent;
class QResizeEvent;


class SpectrogramViewer : public QWidget {

    Q_OBJECT

public:
    explicit SpectrogramViewer(QWidget *parent = nullptr);
    void setSpectrogramImage(const QImage &image);
    void setAudioProperties(double duration, const QString& preciseDurationStr, int nativeSampleRate);
    void setLoadingState(bool loading);

public slots:
    void setShowGrid(bool show);
    void setVerticalZoomEnabled(bool enabled);
    void setCurveType(CurveType type);

signals:
    void fileDropped(const QString &filePath);

protected:
    void paintEvent(QPaintEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void clampViewPosition();
    QRect getSpectrogramRect() const;
    int calculateBestFreqStep(double maxFreqKhz, int availableHeight) const;
    int calculateBestDbStep(double dbRange, int availableHeight) const;
    void updateCurvePathCache();

    QImage m_spectrogramImage;
    double m_zoomX = 1.0;
    double m_zoomY = 1.0;
    double m_offsetX = 0.0;
    double m_offsetY = 0.0;
    
    bool m_isDragging = false;
    QPoint m_lastMousePos;

    bool m_showGrid = true;
    bool m_isVerticalZoomEnabled = false; 
    double m_audioDurationSec = 0.0;
    QString m_preciseDurationStr;
    bool m_isLoading = false;
    int m_nativeSampleRate = 0;
    CurveType m_currentCurveType;
    QPainterPath m_curvePathCache;

};