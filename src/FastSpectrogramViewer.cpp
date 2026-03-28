#include "FastSpectrogramViewer.h"
#include "FastUtils.h"

#include <QPainter>
#include <QApplication>
#include <QResizeEvent>
#include <cstring> 

FastSpectrogramViewer::FastSpectrogramViewer(QWidget *parent)
    : SpectrogramViewerBase(parent)
{
    if (m_crosshairOverlay) {
        m_crosshairOverlay->setDrawBounds(getSpectrogramRect());
    }
    reset();
}

int FastSpectrogramViewer::getContentWidth() const {
    return m_isProcessing ? m_currentWriteX : m_canvas.width();
}

int FastSpectrogramViewer::getContentHeight() const {
    return m_canvas.height();
}

void FastSpectrogramViewer::reset() {
    m_canvas = QImage(1, 1, QImage::Format_RGB32);
    m_canvas.fill(Qt::black);
    m_currentWriteX = 0;
    m_isProcessing = false;
    m_isViewLocked = false;
    m_zoomX = 1.0;
    m_zoomY = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    m_audioDurationSec = 0.0;
    m_preciseDurationStr.clear();
    m_nativeSampleRate = 0;
    m_isAudioLoaded = false;
    update();
}

void FastSpectrogramViewer::initCanvas(int estimatedWidth, int height) {
    if (estimatedWidth < 100) estimatedWidth = 100;
    if (height < 10) height = 10;
    m_canvas = QImage(estimatedWidth, height, QImage::Format_Indexed8);
    updateColorTable();
    m_canvas.fill(0);
    m_currentWriteX = 0;
    m_isProcessing = true;
    m_isViewLocked = true;
    QRect viewRect = getSpectrogramRect();
    if (viewRect.isValid()) {
        m_zoomX = static_cast<double>(viewRect.width()) / estimatedWidth;
        m_zoomY = static_cast<double>(viewRect.height()) / height;
        if (m_zoomX > 1.0) m_zoomX = 1.0;
        if (m_zoomY > 1.0) m_zoomY = 1.0;
    } else {
        m_zoomX = 1.0; m_zoomY = 1.0;
    }
    m_offsetX = 0.0;
    m_offsetY = 0.0;
    update();
}

void FastSpectrogramViewer::appendChunk(const QImage& chunk) {
    if (chunk.isNull() || m_canvas.isNull() || m_canvas.format() != QImage::Format_Indexed8) return;
    int chunkW = chunk.width(); 
    int chunkH = chunk.height();
    if (chunkH != m_canvas.height()) return;
    if (m_currentWriteX + chunkW > m_canvas.width()) {
        int newWidth = std::max(m_canvas.width() + chunkW, static_cast<int>(m_canvas.width() * 1.5));
        QImage newCanvas(newWidth, chunkH, QImage::Format_Indexed8);
        newCanvas.setColorTable(m_canvas.colorTable());
        newCanvas.fill(0);
        for(int y=0; y<chunkH; ++y) {
            std::memcpy(newCanvas.scanLine(y), m_canvas.constScanLine(y), m_currentWriteX);
        }
        m_canvas = newCanvas;
        if (m_isProcessing) {
            QRect viewRect = getSpectrogramRect();
            if (viewRect.isValid() && m_canvas.width() > 0) {
                m_zoomX = static_cast<double>(viewRect.width()) / m_canvas.width();
                if (m_zoomX > 1.0) m_zoomX = 1.0;
            }
        }
    }
    for (int y = 0; y < chunkH; ++y) {
        uchar* dest = m_canvas.scanLine(y) + m_currentWriteX;
        const uchar* src = chunk.constScanLine(y);
        std::memcpy(dest, src, chunkW);
    }
    m_currentWriteX += chunkW;
    update();
}

int FastSpectrogramViewer::getColorIndex(const QPoint& viewPos) const {
    if (m_canvas.isNull()) return 0;
    QRect rect = getSpectrogramRect();
    if (!rect.contains(viewPos)) return 0;
    double imageX = (viewPos.x() - rect.left() - m_offsetX) / m_zoomX;
    double imageY = (viewPos.y() - rect.top()  - m_offsetY) / m_zoomY;
    int x = static_cast<int>(imageX);
    int y = static_cast<int>(imageY);
    int effectiveWidth = m_isProcessing ? m_currentWriteX : m_canvas.width();
    if (x < 0 || x >= effectiveWidth) return 0;
    if (y < 0 || y >= m_canvas.height()) return 0;
    if (m_canvas.format() == QImage::Format_Indexed8) {
        return m_canvas.pixelIndex(x, y);
    } 
    return 0;
}

void FastSpectrogramViewer::setFinished() {
    m_isProcessing = false;
    m_isViewLocked = false;
    if (m_currentWriteX > 0 && m_currentWriteX < m_canvas.width()) {
        m_canvas = m_canvas.copy(0, 0, m_currentWriteX, m_canvas.height());
    }
    QRect viewRect = getSpectrogramRect();
    if (viewRect.isValid() && getContentWidth() > 0 && getContentHeight() > 0) {
        double fitX = static_cast<double>(viewRect.width()) / getContentWidth();
        double fitY = static_cast<double>(viewRect.height()) / getContentHeight();
        m_zoomX = (getContentWidth() > viewRect.width()) ? fitX : 1.0;
        m_zoomY = (getContentHeight() > viewRect.height()) ? fitY : 1.0;
        m_offsetX = 0.0;
        m_offsetY = 0.0;
    }
    clampViewPosition();
    update();
}

void FastSpectrogramViewer::setPaletteType(PaletteType type) {
    SpectrogramViewerBase::setPaletteType(type);
    updateColorTable();
}

int FastSpectrogramViewer::getReferenceWidth() const {
    return m_canvas.width();
}

void FastSpectrogramViewer::updateColorTable() {
    if (m_canvas.format() != QImage::Format_Indexed8) return;
    std::vector<QColor> qColors = ColorPalette::getPalette(m_currentPaletteType);
    if (qColors.size() < 256) qColors = ColorPalette::getPalette(PaletteType::S00);
    QList<QRgb> colorTable;
    colorTable.reserve(256);
    for (const auto& color : qColors) colorTable.append(color.rgb());
    m_canvas.setColorTable(colorTable);
}

void FastSpectrogramViewer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(34, 41, 51));
    if (getContentWidth() <= 1 && !m_isProcessing) {
        painter.setPen(QColor(180, 180, 180));
        QFont infoFont = QApplication::font();
        infoFont.setPointSize(15);
        painter.setFont(infoFont);
        painter.drawText(rect(), Qt::AlignCenter, tr("点击打开按钮  或拖拽文件到此区域"));
        return;
    }
    const QRect spectrogramRect = getSpectrogramRect();
    if (spectrogramRect.width() < 1 || spectrogramRect.height() < 1) return;
    int validWidth = getContentWidth();
    if (validWidth > 0) {
        double sceneX = -m_offsetX / m_zoomX;
        double sceneY = -m_offsetY / m_zoomY;
        double sceneWidth = spectrogramRect.width() / m_zoomX;
        double sceneHeight = spectrogramRect.height() / m_zoomY;
        QRectF sourceRectF(std::max(0.0, sceneX), std::max(0.0, sceneY), sceneWidth, sceneHeight);
        QRectF imageRectF(0, 0, validWidth, m_canvas.height());
        sourceRectF = sourceRectF.intersected(imageRectF);
        if (!sourceRectF.isEmpty()) {
            QRectF destinationRect(
                spectrogramRect.left() + m_offsetX + sourceRectF.x() * m_zoomX,
                spectrogramRect.top() + m_offsetY + sourceRectF.y() * m_zoomY,
                sourceRectF.width() * m_zoomX,
                sourceRectF.height() * m_zoomY
            );
            painter.drawImage(destinationRect, m_canvas, sourceRectF);
        }
    }
    drawOverlay(painter);
}

void FastSpectrogramViewer::resizeEvent(QResizeEvent* event) {
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
    if (m_isProcessing) {
        m_zoomX = static_cast<double>(viewRect.width()) / m_canvas.width();
        m_zoomY = static_cast<double>(viewRect.height()) / m_canvas.height();
        if (m_zoomX > 1.0) m_zoomX = 1.0;
        if (m_zoomY > 1.0) m_zoomY = 1.0;
        m_offsetX = 0.0; 
        m_offsetY = 0.0;
        clampViewPosition(); 
        setPlayheadPosition(m_playheadTime);
        update();
        return;
    }
    if (m_isViewLocked) {
        clampViewPosition();
        setPlayheadPosition(m_playheadTime);
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
        
        update();
        return;
    }
    int oldViewWidth = oldSize.width() - FastConfig::VIEWER_LEFT_MARGIN - FastConfig::VIEWER_RIGHT_MARGIN;
    int oldViewHeight = oldSize.height() - FastConfig::VIEWER_TOP_MARGIN - FastConfig::VIEWER_BOTTOM_MARGIN;
    if (oldViewWidth <= 0 || oldViewHeight <= 0) { 
        clampViewPosition(); 
        setPlayheadPosition(m_playheadTime);
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
    update();
}