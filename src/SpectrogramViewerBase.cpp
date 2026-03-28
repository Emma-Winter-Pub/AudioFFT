#include "SpectrogramViewerBase.h"
#include "WindowFunctions.h" 

#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QApplication>
#include <QDateTime>
#include <cmath>
#include <algorithm>

namespace {
    #ifndef M_PI
    #define M_PI 3.14159265358979323846
    #endif
}

SpectrogramViewerBase::SpectrogramViewerBase(QWidget* parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setMinimumSize(400, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    m_spectrumOverlay = new SpectrumOverlay(this);
    m_spectrumOverlay->resize(size());
    m_spectrumOverlay->show();
    m_crosshairOverlay = new CrosshairOverlay(this);
    m_crosshairOverlay->setDrawBounds(getSpectrogramRect());
    m_crosshairOverlay->show();
    m_crosshairOverlay->raise(); 
    m_imageProvider = new ImageSpectrumDataProvider();
    m_spectrumProvider = m_imageProvider;
    m_fpsTimer.start();
}

SpectrogramViewerBase::~SpectrogramViewerBase() {
    if (m_imageProvider) {
        delete m_imageProvider;
        m_imageProvider = nullptr;
    }
}

void SpectrogramViewerBase::setExternalDataProvider(ISpectrumDataProvider* provider) {
    m_externalProvider = provider;
}

void SpectrogramViewerBase::setDataConfig(DataSourceType spectrumSource, DataSourceType probeSource, int probePrecision) {
    m_spectrumSource = spectrumSource;
    m_probeSource = probeSource;
    m_probeDbPrecision = probePrecision;
}

void SpectrogramViewerBase::setSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type) {
    m_showSpectrumProfile = visible;
    if (m_spectrumOverlay) {
        if (!visible) {
            m_spectrumOverlay->clearData();
            m_spectrumOverlay->setVisible(false);
        } else {
            m_spectrumOverlay->setVisible(true);
            m_spectrumOverlay->setStyle(color, lineWidth, filled, alpha, type);
        }
    }
}

void SpectrogramViewerBase::setProfileFrameRate(int fps) {
    if (fps < 1) fps = 1;
    if (fps > 300) fps = 300;
    m_profileFps = fps;
}

void SpectrogramViewerBase::setPlayheadStyle(const PlayheadStyle& style) {
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setPlayheadStyle(style);
    }
}

void SpectrogramViewerBase::setGpuEnabled(bool enabled) {
    if (m_spectrumOverlay) {
        m_spectrumOverlay->setGpuEnabled(enabled);
    }
}

void SpectrogramViewerBase::setCrosshairStyle(const CrosshairStyle& style) {
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setStyle(style);
    }
}

void SpectrogramViewerBase::setAudioProperties(double duration, const QString& preciseDurationStr, int nativeSampleRate) {
    m_audioDurationSec = duration;
    m_preciseDurationStr = preciseDurationStr;
    m_nativeSampleRate = nativeSampleRate;
    m_isAudioLoaded = true;
    update();
}

void SpectrogramViewerBase::setPlayheadPosition(double seconds) {
    if (m_isDraggingPlayhead) return;
    double pps = getPixelsPerSecond();
    QRect viewRect = getSpectrogramRect();
    double oldScreenX = timeToScreenX(m_playheadTime);
    m_playheadTime = seconds;
    double newScreenX_double = timeToScreenX(seconds);
    int newScreenX = static_cast<int>(newScreenX_double);
    if (m_crosshairOverlay) {
        bool visible = m_isPlayheadVisible && (pps > 0);
        m_crosshairOverlay->setPlayheadPosition(newScreenX, visible);
    }
    if (m_isPlayheadVisible && pps > 0) {
        bool wasInside = (oldScreenX < viewRect.right()); 
        bool isOutside = (newScreenX_double >= viewRect.right());
        if (wasInside && isOutside) {
            m_offsetX = -(m_playheadTime * pps);
            clampViewPosition();
            updateOverlayDataUnderMouse(true);
            update();
        }
    }
    if (!m_showSpectrumProfile || !m_spectrumOverlay) return;
    if (m_isMouseInView) return;
    if (!m_isAudioLoaded || newScreenX < viewRect.left() || newScreenX > viewRect.right()) {
        m_spectrumOverlay->clearData();
        return;
    }
    int intervalMs = 1000 / m_profileFps;
    if (m_fpsTimer.elapsed() < intervalMs) return;
    updateSpectrumOverlayFromTime(seconds);
    m_fpsTimer.restart();
}

void SpectrogramViewerBase::updateSpectrumOverlayFromTime(double timeSec) {
    ISpectrumDataProvider* activeProvider = m_imageProvider;
    if (m_spectrumSource == DataSourceType::FftRawData && m_externalProvider) {
        activeProvider = m_externalProvider;
    } else {
        if (m_imageProvider) m_imageProvider->updateImage(getCurrentImage());
        activeProvider = m_imageProvider;
    }
    if (!activeProvider || !m_spectrumOverlay) return;
    const QImage* img = getCurrentImage();
    if (m_spectrumSource == DataSourceType::ImagePixel && img && m_audioDurationSec > 0) {
        int col = static_cast<int>((timeSec / m_audioDurationSec) * img->width());
        if (col < 0) col = 0;
        if (col >= img->width()) col = img->width() - 1;
        if (col == m_lastDrawnSpectrumColumn) return; 
        m_lastDrawnSpectrumColumn = col;
    }
    std::vector<float> data; 
    int viewWidth = getSpectrogramRect().width();
    if (viewWidth < 1) viewWidth = 100;
    if (activeProvider->fetchData(timeSec, m_audioDurationSec, data, viewWidth)) {
        m_spectrumOverlay->setData(data);
    } else {
        m_spectrumOverlay->clearData();
    }
}

void SpectrogramViewerBase::setPlayheadVisible(bool visible) {
    if (m_isPlayheadVisible != visible) {
        m_isPlayheadVisible = visible;
        if (m_crosshairOverlay) {
             double pps = getPixelsPerSecond();
             int screenX = static_cast<int>(timeToScreenX(m_playheadTime));
             bool effectiveVisible = visible && (pps > 0);
             m_crosshairOverlay->setPlayheadPosition(screenX, effectiveVisible);
        }
        updateOverlayDataUnderMouse(true);
    }
}

void SpectrogramViewerBase::setShowGrid(bool show) {
    m_showGrid = show;
    update();
}

void SpectrogramViewerBase::setVerticalZoomEnabled(bool enabled) {
    m_isVerticalZoomEnabled = enabled;
}

void SpectrogramViewerBase::setCurveType(CurveType type) {
    if (m_currentCurveType != type) {
        m_currentCurveType = type;
        update();
    }
}

void SpectrogramViewerBase::setPaletteType(PaletteType type) {
    m_currentPaletteType = type;
    update();
}

void SpectrogramViewerBase::setMinDb(double minDb) {
    m_minDb = minDb;
    update();
}

void SpectrogramViewerBase::setMaxDb(double maxDb) {
    m_maxDb = maxDb;
    update();
}

QRect SpectrogramViewerBase::getSpectrogramRect() const {
    return QRect(
        AppConfig::VIEWER_LEFT_MARGIN,
        AppConfig::VIEWER_TOP_MARGIN,
        width() - AppConfig::VIEWER_LEFT_MARGIN - AppConfig::VIEWER_RIGHT_MARGIN,
        height() - AppConfig::VIEWER_TOP_MARGIN - AppConfig::VIEWER_BOTTOM_MARGIN
    );
}

int SpectrogramViewerBase::getReferenceWidth() const {
    return getContentWidth();
}

double SpectrogramViewerBase::getPixelsPerSecond() const {
    if (m_audioDurationSec <= 0.000001) return 0.0;
    int refW = getReferenceWidth();
    if (refW <= 1) return 0.0;
    return (static_cast<double>(refW) * m_zoomX) / m_audioDurationSec;
}

double SpectrogramViewerBase::timeToScreenX(double seconds) const {
    double pps = getPixelsPerSecond();
    QRect viewRect = getSpectrogramRect();
    return viewRect.left() + (seconds * pps) + m_offsetX;
}

double SpectrogramViewerBase::screenXToTime(double x) const {
    double pps = getPixelsPerSecond();
    if (pps <= 0) return 0.0;
    QRect viewRect = getSpectrogramRect();
    double t = (x - viewRect.left() - m_offsetX) / pps;
    return std::max(0.0, std::min(t, m_audioDurationSec));
}

void SpectrogramViewerBase::clampViewPosition() {
    int contentW = getContentWidth();
    int contentH = getContentHeight();
    if (contentW <= 0 || contentH <= 0) return;
    QRect viewRect = getSpectrogramRect();
    double scaledWidth = contentW * m_zoomX;
    double scaledHeight = contentH * m_zoomY;
    if (scaledWidth <= viewRect.width()) {
        if (m_isViewLocked) {
            m_offsetX = 0;
        } else {
            m_offsetX = (viewRect.width() - scaledWidth) / 2.0;
        }
    } else {
        m_offsetX = std::min(0.0, m_offsetX);
        m_offsetX = std::max(viewRect.width() - scaledWidth, m_offsetX);
    }
    if (scaledHeight <= viewRect.height()) {
        m_offsetY = (viewRect.height() - scaledHeight) / 2.0;
    } else {
        m_offsetY = std::min(0.0, m_offsetY);
        m_offsetY = std::max(viewRect.height() - scaledHeight, m_offsetY);
    }
}

void SpectrogramViewerBase::setComponentsVisible(bool visible) {
    if (m_showComponents != visible) {
        m_showComponents = visible;
        update();
    }
}

void SpectrogramViewerBase::drawOverlay(QPainter& painter) {
    const QRect spectrogramRect = getSpectrogramRect();
    int contentH = getContentHeight();
    painter.setPen(QColor(200, 200, 200));
    QFont axisFont = QApplication::font();
    axisFont.setPointSize(8);
    painter.setFont(axisFont);
    const double maxFreq = static_cast<double>(m_nativeSampleRate) / 2.0;
    const double maxFreqKhz = maxFreq / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, spectrogramRect.height());
    if (m_showComponents) {
        painter.drawText(QRect(0, spectrogramRect.top() - 21, AppConfig::VIEWER_LEFT_MARGIN - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");
    }
    QFontMetrics fm_khz(painter.font());
    const int minLabelSpacing = fm_khz.height();
    int lastDrawnLabelY = -1000;
    for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
        if (khz > maxFreqKhz) break;
        double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
        double curved_ratio = MappingCurves::forward(linear_ratio, m_currentCurveType, maxFreq);
        double y_in_image = (1.0 - curved_ratio) * (contentH - 1);
        int y_on_screen = static_cast<int>(spectrogramRect.top() + m_offsetY + y_in_image * m_zoomY);
        if (m_showComponents && std::abs(y_on_screen - lastDrawnLabelY) >= minLabelSpacing) {
            QRect freqValueRect(0, y_on_screen - 10, AppConfig::VIEWER_LEFT_MARGIN - 5, 20);
            painter.drawText(freqValueRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
            lastDrawnLabelY = y_on_screen;
        }
    }
    if (m_showComponents) {
        QRect colorBarArea(width() - AppConfig::VIEWER_RIGHT_MARGIN + 5, spectrogramRect.top(), AppConfig::VIEWER_COLOR_BAR_WIDTH, spectrogramRect.height());
        QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
        std::vector<QColor> colorMap = ColorPalette::getPalette(m_currentPaletteType);
        if (colorMap.size() < 256) colorMap = ColorPalette::getPalette(PaletteType::S00);
        int steps = std::min(256, (int)colorMap.size());
        for (int i = 0; i < steps; ++i) {
            gradient.setColorAt(1.0 - (double)i / (steps - 1), colorMap[i]);
        }
        painter.fillRect(colorBarArea, gradient);
        painter.drawText(QRect(width() - AppConfig::VIEWER_RIGHT_MARGIN + 14, spectrogramRect.top() - 17, 40, 20), Qt::AlignHCenter, "dB");
        const double dbRange = m_maxDb - m_minDb;
        const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
        if (dbRange > 0 && dbStep > 0) {
            double start_db = std::ceil(m_maxDb / dbStep) * dbStep;
            if (start_db > m_maxDb) start_db -= dbStep;
            for (double db = start_db; db >= m_minDb; db -= dbStep) {
                int y = colorBarArea.top() + static_cast<int>(((m_maxDb - db) / dbRange) * colorBarArea.height());
                if (y >= colorBarArea.top() - 10 && y <= colorBarArea.bottom() + 10) {
                    painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));
                }
            }
        }
    }
    if (m_audioDurationSec > 0 && getContentWidth() > 0) {
        double pixelsPerSec = getPixelsPerSecond();
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        int timeStep = tickLevels.back();
        if (pixelsPerSec > 0) {
            const int minHorizontalSpacing = 50;
            for (const int level : tickLevels) {
                if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; } 
                else { break; }
            }
        }
        if (m_showGrid && pixelsPerSec > 0) {
            painter.save();
            painter.setClipRect(spectrogramRect);
            painter.translate(0.5, 0.5);
            QPen gridPen(AppConfig::VIEWER_GRID_LINE_COLOR, 1, Qt::DotLine);
            painter.setPen(gridPen);
            for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
                if (khz == 0) continue;
                double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
                double curved_ratio = MappingCurves::forward(linear_ratio, m_currentCurveType, maxFreq);
                double y_in_image = (1.0 - curved_ratio) * (contentH - 1);
                int y_on_screen = static_cast<int>(spectrogramRect.top() + m_offsetY + y_in_image * m_zoomY);
                if (y_on_screen >= spectrogramRect.top() && y_on_screen <= spectrogramRect.bottom()) {
                    painter.drawLine(spectrogramRect.left(), y_on_screen, spectrogramRect.right(), y_on_screen);
                }
            }
            double startTimeInView = -(m_offsetX) / pixelsPerSec;
            double endTimeInView = startTimeInView + spectrogramRect.width() / pixelsPerSec;
            long long firstTick = (static_cast<long long>(startTimeInView / timeStep) + 1) * timeStep;
            if (firstTick < timeStep) firstTick = timeStep;
            for (long long t = firstTick; t < endTimeInView && t < m_audioDurationSec; t += timeStep) {
                 int x_pos = static_cast<int>(spectrogramRect.left() + t * pixelsPerSec + m_offsetX);
                 if (x_pos > spectrogramRect.left() && x_pos < spectrogramRect.right()) {
                    painter.drawLine(x_pos, spectrogramRect.top(), x_pos, spectrogramRect.bottom());
                 }
            }
            painter.restore();
        }
        if (m_showComponents) {
            painter.setPen(QColor(200, 200, 200));
            QFontMetrics fm(painter.font());
            QString totalTimeLabel = m_preciseDurationStr.isEmpty() ?
                QDateTime::fromSecsSinceEpoch(static_cast<qint64>(m_audioDurationSec), Qt::UTC).toString(m_audioDurationSec >= 3600 ? "h:mm:ss" : "m:ss") :
                m_preciseDurationStr;
            double scaledWidth = getContentWidth() * m_zoomX;
            bool isAtRightEnd = std::abs(m_offsetX - (spectrogramRect.width() - scaledWidth)) < 20.0;
            bool isFullyVisible = (scaledWidth <= spectrogramRect.width());
            QRect totalRect;
            bool showTotalLabel = isFullyVisible || isAtRightEnd;
            if (showTotalLabel) {
                double totalTimeX_pixel = spectrogramRect.left() + m_audioDurationSec * pixelsPerSec + m_offsetX;
                int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
                const int padding = 5;
                int centerWidth = totalLabelWidth + 2 * padding;
                totalRect = QRect(static_cast<int>(totalTimeX_pixel) - centerWidth / 2, spectrogramRect.bottom() + 5, centerWidth, AppConfig::VIEWER_BOTTOM_MARGIN);
            }
            double startTimeInView = -(m_offsetX) / pixelsPerSec;
            double endTimeInView = startTimeInView + spectrogramRect.width() / pixelsPerSec;
            long long firstTick = (static_cast<long long>(startTimeInView / timeStep) + 1) * timeStep;
            if (firstTick < timeStep) firstTick = timeStep;
            for (long long t = firstTick; t < endTimeInView && t < m_audioDurationSec; t += timeStep) {
                double x_pos_double = spectrogramRect.left() + t * pixelsPerSec + m_offsetX;
                int x_pos = static_cast<int>(x_pos_double);
                QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
                int labelWidth = fm.horizontalAdvance(timeLabel);
                const int padding = 5;
                int centerWidth = labelWidth + 2 * padding;
                QRect labelRect(x_pos - centerWidth / 2, spectrogramRect.bottom() + 5, centerWidth, AppConfig::VIEWER_BOTTOM_MARGIN);
                if (labelRect.right() < spectrogramRect.left() || labelRect.left() > spectrogramRect.right()) continue;
                if (showTotalLabel && !totalRect.isNull() && labelRect.intersects(totalRect)) continue;
                painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);
            }
            if (showTotalLabel && !totalRect.isNull()) {
                painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
            }
        }
    }
}

int SpectrogramViewerBase::calculateBestFreqStep(double maxFreqKhz, int availableHeight) const {
    if (maxFreqKhz <= 0 || availableHeight <= 0) return 1;
    const int minVerticalSpacing = 25;
    int numLabelsPossible = availableHeight / minVerticalSpacing;
    if (numLabelsPossible <= 1) return std::max(1, static_cast<int>(maxFreqKhz));
    double desiredStep = maxFreqKhz / numLabelsPossible;
    if (desiredStep <= 0) return 1;
    double powerOf10 = std::pow(10, std::floor(std::log10(desiredStep)));
    if (powerOf10 < 1) powerOf10 = 1;
    double normalizedStep = desiredStep / powerOf10;
    int finalStep;
    if (normalizedStep < 1.5) finalStep = 1 * powerOf10;
    else if (normalizedStep < 3.5) finalStep = 2 * powerOf10;
    else if (normalizedStep < 7.5) finalStep = 5 * powerOf10;
    else finalStep = 10 * powerOf10;
    return std::max(1, finalStep);
}

int SpectrogramViewerBase::calculateBestDbStep(double dbRange, int availableHeight) const {
    if (dbRange <= 0 || availableHeight <= 0) return 20;
    const int minVerticalSpacing = 30;
    int numLabelsPossible = availableHeight / minVerticalSpacing;
    if (numLabelsPossible <= 1) return std::max(1, static_cast<int>(dbRange));
    double desiredStep = dbRange / numLabelsPossible;
    double powerOf10 = std::pow(10, std::floor(std::log10(desiredStep)));
    if (powerOf10 <= 0) powerOf10 = 1;
    double normalizedStep = desiredStep / powerOf10;
    int finalStep;
    if (normalizedStep < 1.5) finalStep = 1 * powerOf10;
    else if (normalizedStep < 2.2) finalStep = 2 * powerOf10;
    else if (normalizedStep < 3.5) finalStep = static_cast<int>(2.5 * powerOf10);
    else if (normalizedStep < 7.5) finalStep = 5 * powerOf10;
    else finalStep = 10 * powerOf10;
    return std::max(1, finalStep);
}

void SpectrogramViewerBase::updateOverlayDataUnderMouse(bool forceUpdate) {
    if (!m_isAudioLoaded || m_isViewLocked) {
        if (m_crosshairOverlay) m_crosshairOverlay->setIndicatorData("", "", "");
        if (m_spectrumOverlay && !m_isPlayheadVisible) m_spectrumOverlay->clearData();
        return;
    }
    if (m_isMouseInView) {
        QRect rect = getSpectrogramRect();
        SpectrogramViewParams params;
        params.sampleRate = m_nativeSampleRate;
        params.durationSec = m_audioDurationSec;
        params.imageHeight = getContentHeight();
        params.pixelsPerSecond = getPixelsPerSecond();
        params.zoomX = m_zoomX;
        params.zoomY = m_zoomY;
        params.offsetX = m_offsetX;
        params.offsetY = m_offsetY;
        params.curveType = m_currentCurveType;
        params.minDb = m_minDb;
        params.maxDb = m_maxDb;
        m_calculator.updateParams(params);
        QString freqStr = m_calculator.calcFrequency(m_currentMousePos.y(), rect.top());
        QString timeStr = m_calculator.calcTime(m_currentMousePos.x(), rect.left());
        QString dbStr;
        if (m_probeSource == DataSourceType::FftRawData && m_externalProvider) {
            double mouseFreqHz = 0.0;
            double maxFreq = m_nativeSampleRate / 2.0;
            double relativeY = (m_currentMousePos.y() - rect.top() - m_offsetY) / m_zoomY;
            if (relativeY >= 0 && relativeY < params.imageHeight) {
                double normY = (params.imageHeight - 1.0 - relativeY) / (params.imageHeight - 1.0);
                double ratio = MappingCurves::inverse(normY, m_currentCurveType, maxFreq);
                mouseFreqHz = ratio * maxFreq;
            }
            double timeSec = screenXToTime(m_currentMousePos.x());
            double dbVal = m_externalProvider->getDbValueAt(timeSec, m_audioDurationSec, mouseFreqHz, maxFreq);
            if (dbVal <= -199.0) {
                dbStr = "";
            } else {
                int userSig = m_probeDbPrecision;
                int maxSig = m_externalProvider->getMaxSignificantDigits();
                if (userSig == 0) {
                    dbStr = QString::number(std::round(dbVal)) + " dBFS";
                } else {
                    int finalSig = std::min(userSig, maxSig);
                    dbStr = QString::number(dbVal, 'g', finalSig) + " dBFS";
                }
            }
        } 
        else {
            int colorIndex = getColorIndex(m_currentMousePos);
            dbStr = m_calculator.calcDb(colorIndex); 
        }
        if (m_crosshairOverlay) {
            m_crosshairOverlay->setIndicatorData(freqStr, timeStr, dbStr);
        }
        if (m_showSpectrumProfile && m_spectrumOverlay) {
            double timeSec = screenXToTime(m_currentMousePos.x());
            double intervalMs = 1000.0 / m_profileFps;
            if (forceUpdate || m_fpsTimer.elapsed() >= intervalMs) {
                updateSpectrumOverlayFromTime(timeSec);
                m_fpsTimer.restart();
            }
        }
    } 
    else {
        if (m_crosshairOverlay) {
            m_crosshairOverlay->setIndicatorData("", "", "");
        }
        if (m_showSpectrumProfile && m_spectrumOverlay) {
            if (!m_isPlayheadVisible) {
                m_spectrumOverlay->clearData();
            } else {
                updateSpectrumOverlayFromTime(m_playheadTime);
            }
        }
    }
}

void SpectrogramViewerBase::wheelEvent(QWheelEvent* event) {
    if (getContentWidth() <= 1) return;
    double scaleFactor = (event->angleDelta().y() > 0) ? 1.25 : 0.8;
    QRect viewRect = getSpectrogramRect();
    if (viewRect.width() <= 0 || viewRect.height() <= 0) return;
    double fitZoomX = static_cast<double>(viewRect.width()) / getContentWidth();
    double fitZoomY = static_cast<double>(viewRect.height()) / getContentHeight();
    if (fitZoomX > 1.0) fitZoomX = 1.0;
    if (fitZoomY > 1.0) fitZoomY = 1.0;
    double newZoomX = m_zoomX * scaleFactor;
    if (newZoomX > 1.0) newZoomX = 1.0;
    if (newZoomX < fitZoomX) newZoomX = fitZoomX;
    double newZoomY = m_zoomY;
    if (m_isVerticalZoomEnabled) {
        newZoomY = m_zoomY * scaleFactor;
        if (newZoomY > 1.0) newZoomY = 1.0;
        if (newZoomY < fitZoomY) newZoomY = fitZoomY;
    }
    if (std::abs(newZoomX - m_zoomX) < 1e-9 && std::abs(newZoomY - m_zoomY) < 1e-9) return;
    QPointF zoomCenter = event->position();
    double cx = std::max((double)viewRect.left(), std::min((double)viewRect.right(), zoomCenter.x()));
    double cy = std::max((double)viewRect.top(), std::min((double)viewRect.bottom(), zoomCenter.y()));
    double sceneX = (cx - viewRect.left() - m_offsetX) / m_zoomX;
    double sceneY = (cy - viewRect.top() - m_offsetY) / m_zoomY;
    m_zoomX = newZoomX;
    m_zoomY = newZoomY;
    m_offsetX = cx - viewRect.left() - sceneX * m_zoomX;
    m_offsetY = cy - viewRect.top() - sceneY * m_zoomY;
    clampViewPosition();
    setPlayheadPosition(m_playheadTime);
    updateOverlayDataUnderMouse(true); 
    update();
}

void SpectrogramViewerBase::resizeEvent(QResizeEvent* event) {
    if (m_crosshairOverlay) {
        m_crosshairOverlay->resize(size());
        m_crosshairOverlay->setDrawBounds(getSpectrogramRect());
    }
    if (m_spectrumOverlay) {
        m_spectrumOverlay->resize(size());
        m_spectrumOverlay->setDrawRect(getSpectrogramRect());
    }
    QWidget::resizeEvent(event);
    int contentW = getContentWidth();
    int contentH = getContentHeight();
    if (contentW <= 1 || contentH <= 1) return;
    QRect viewRect = getSpectrogramRect();
    if (!viewRect.isValid()) return;
    if (m_isViewLocked) {
        clampViewPosition();
        updateOverlayDataUnderMouse(true); 
        update();
        return;
    }
    double oldZoomX = m_zoomX;
    double oldZoomY = m_zoomY;
    QSize oldSize = event->oldSize();
    if (!oldSize.isValid() || oldSize.width() <= 0 || oldSize.height() <= 0) {
        m_zoomX = static_cast<double>(viewRect.width()) / contentW;
        m_zoomY = static_cast<double>(viewRect.height()) / contentH;
        if (m_zoomX > 1.0) m_zoomX = 1.0;
        if (m_zoomY > 1.0) m_zoomY = 1.0;
        clampViewPosition();
        setPlayheadPosition(m_playheadTime);
        updateOverlayDataUnderMouse(true); 
        update();
        return;
    }
    int oldViewWidth = oldSize.width() - AppConfig::VIEWER_LEFT_MARGIN - AppConfig::VIEWER_RIGHT_MARGIN;
    int oldViewHeight = oldSize.height() - AppConfig::VIEWER_TOP_MARGIN - AppConfig::VIEWER_BOTTOM_MARGIN;
    if (oldViewWidth <= 0 || oldViewHeight <= 0) { 
        clampViewPosition(); 
        setPlayheadPosition(m_playheadTime);
        updateOverlayDataUnderMouse(true); 
        update(); 
        return; 
    }
    double wRatio = static_cast<double>(viewRect.width()) / oldViewWidth;
    double hRatio = static_cast<double>(viewRect.height()) / oldViewHeight;
    if (std::abs(m_zoomX - 1.0) < 1e-9) {
        m_zoomX = 1.0;
    } else {
        m_zoomX *= wRatio;
        if (m_zoomX > 1.0) m_zoomX = 1.0;
    }
    if (std::abs(m_zoomY - 1.0) < 1e-9) {
        m_zoomY = 1.0;
    } else {
        m_zoomY *= hRatio;
        if (m_zoomY > 1.0) m_zoomY = 1.0;
    }
    if (oldZoomX > 1e-9) m_offsetX = m_offsetX * (m_zoomX / oldZoomX);
    if (oldZoomY > 1e-9) m_offsetY = m_offsetY * (m_zoomY / oldZoomY);
    clampViewPosition();
    setPlayheadPosition(m_playheadTime);
    updateOverlayDataUnderMouse(true); 
    update();
}

void SpectrogramViewerBase::enterEvent(QEnterEvent* event){
    QWidget::enterEvent(event);
    if (m_isAudioLoaded && m_crosshairOverlay) {
        m_crosshairOverlay->setCursorVisible(true);
    }
}

void SpectrogramViewerBase::leaveEvent(QEvent* event){
    QWidget::leaveEvent(event);
    unsetCursor(); 
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setCursorVisible(false);
    }
    m_isMouseInView = false;
    updateOverlayDataUnderMouse(true);
}

void SpectrogramViewerBase::mousePressEvent(QMouseEvent* event) {
    if (m_isViewLocked) return;
    if (event->button() == Qt::LeftButton) {
        QRect viewRect = getSpectrogramRect();
        bool inTopBar = (event->pos().y() >= 0 && event->pos().y() <= viewRect.top());
        bool inHandle = false;
        if (m_isPlayheadVisible) {
            int playheadX = static_cast<int>(timeToScreenX(m_playheadTime));
            QRect handleRect(playheadX - 8, 0, 16, viewRect.top());
            if (handleRect.contains(event->pos())) {
                inHandle = true;
            }
        }
        if (m_isAudioLoaded && (inTopBar || inHandle)) {
            double t = screenXToTime(event->pos().x());
            setPlayheadPosition(t); 
            m_isDraggingPlayhead = true;
            setCursor(Qt::ClosedHandCursor); 
            if (m_crosshairOverlay) m_crosshairOverlay->setCursorVisible(false);
            return;
        }
        if (viewRect.contains(event->pos())) {
            m_isDraggingView = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            if (m_crosshairOverlay) m_crosshairOverlay->setCursorVisible(false);
        }
    }
}

void SpectrogramViewerBase::mouseMoveEvent(QMouseEvent* event) {
    m_currentMousePos = event->pos();
    QRect rect = getSpectrogramRect();
    m_isMouseInView = rect.contains(m_currentMousePos);
    if (m_isAudioLoaded && m_crosshairOverlay && !m_crosshairOverlay->isCursorVisible()) {
        m_crosshairOverlay->setCursorVisible(true);
    }
    if (m_isAudioLoaded && m_crosshairOverlay) {
        m_crosshairOverlay->updatePosition(event->pos());
    }
    if (m_isViewLocked) {
        updateOverlayDataUnderMouse(false);
        return;
    }
    if (m_isDraggingPlayhead) {
        double t = screenXToTime(event->pos().x());
        m_isDraggingPlayhead = false; 
        setPlayheadPosition(t); 
        m_isDraggingPlayhead = true;
        updateOverlayDataUnderMouse(false);
        return;
    }
    if (m_isDraggingView) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offsetX += delta.x();
        m_offsetY += delta.y();
        m_lastMousePos = event->pos();
        clampViewPosition();
        setPlayheadPosition(m_playheadTime); 
        updateOverlayDataUnderMouse(true);
        update();
        return;
    }
    updateOverlayDataUnderMouse(false);
    setCursor(Qt::ArrowCursor);
}

void SpectrogramViewerBase::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_isDraggingPlayhead) {
            m_isDraggingPlayhead = false;
            emit seekRequested(m_playheadTime); 
        }
        if (m_isDraggingView) {
            m_isDraggingView = false;
        }
        setCursor(Qt::ArrowCursor);
        if (rect().contains(event->pos()) && m_crosshairOverlay) {
            m_crosshairOverlay->setCursorVisible(true);
        }
    }
}

void SpectrogramViewerBase::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void SpectrogramViewerBase::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()) {
        emit fileDropped(event->mimeData()->urls().first().toLocalFile());
    }
}

void SpectrogramViewerBase::setCrosshairEnabled(bool enabled) {
    m_isCrosshairEnabled = enabled;
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setLinesEnabled(enabled);
        if (m_isAudioLoaded && rect().contains(mapFromGlobal(QCursor::pos()))) {
            m_crosshairOverlay->setCursorVisible(true);
        }
    }
}

void SpectrogramViewerBase::setIndicatorVisibility(bool showFreq, bool showTime, bool showDb){
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setIndicatorVisibility(showFreq, showTime, showDb);
    }
}