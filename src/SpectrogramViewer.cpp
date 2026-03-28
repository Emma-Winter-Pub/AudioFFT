#include "SpectrogramViewer.h"
#include "AppConfig.h"

#include <QPainter>
#include <QApplication>

SpectrogramViewer::SpectrogramViewer(QWidget* parent)
    : SpectrogramViewerBase(parent)
{
    m_spectrogramImage = QImage(1, 1, QImage::Format_RGB32);
    m_spectrogramImage.fill(Qt::black);
}

int SpectrogramViewer::getContentWidth() const {
    return m_spectrogramImage.width();
}

int SpectrogramViewer::getContentHeight() const {
    return m_spectrogramImage.height();
}

int SpectrogramViewer::getColorIndex(const QPoint& viewPos) const {
    if (m_spectrogramImage.isNull()) return 0;
    QRect rect = getSpectrogramRect();
    if (!rect.contains(viewPos)) return 0;
    double imageX = (viewPos.x() - rect.left() - m_offsetX) / m_zoomX;
    double imageY = (viewPos.y() - rect.top()  - m_offsetY) / m_zoomY;
    int x = static_cast<int>(imageX);
    int y = static_cast<int>(imageY);
    if (x < 0 || x >= m_spectrogramImage.width()) return 0;
    if (y < 0 || y >= m_spectrogramImage.height()) return 0;
    if (m_spectrogramImage.format() == QImage::Format_Indexed8) {
        return m_spectrogramImage.pixelIndex(x, y);
    } 
    return 0;
}

void SpectrogramViewer::setSpectrogramImage(const QImage& image, bool resetView) {
    m_spectrogramImage = image.isNull() ? QImage(1, 1, QImage::Format_RGB32) : image;
    if (image.isNull()) {
        m_isAudioLoaded = false;
    }
    if (resetView) {
        if (getContentWidth() > 1 && getContentHeight() > 1) {
            QRect viewRect = getSpectrogramRect();
            if (viewRect.isValid()) {
                m_zoomX = static_cast<double>(viewRect.width()) / getContentWidth();
                m_zoomY = static_cast<double>(viewRect.height()) / getContentHeight();
                if (m_zoomX > 1.0) m_zoomX = 1.0;
                if (m_zoomY > 1.0) m_zoomY = 1.0;
            }
        } else {
            m_zoomX = 1.0; m_zoomY = 1.0;
        }
        m_offsetX = 0.0;
        m_offsetY = 0.0;
        m_playheadTime = 0.0;
    }
    clampViewPosition();
    update();
}

void SpectrogramViewer::setLoadingState(bool loading) {
    m_isLoading = loading;
    m_isViewLocked = loading;
    update();
}

void SpectrogramViewer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing); 
    painter.fillRect(rect(), QColor(34, 41, 51));
    if (m_isLoading) {
        painter.setPen(QColor(180, 180, 180));
        QFont infoFont = QApplication::font();
        infoFont.setPointSize(15);
        painter.setFont(infoFont);
        painter.drawText(rect(), Qt::AlignCenter, tr("正在处理文件  这可能需要一些时间"));
        return; 
    }
    if (getContentWidth() <= 1 || m_nativeSampleRate == 0) {
        painter.setPen(QColor(180, 180, 180));
        QFont infoFont = QApplication::font();
        infoFont.setPointSize(15);
        painter.setFont(infoFont);
        painter.drawText(rect(), Qt::AlignCenter, tr("点击打开按钮  或拖拽文件到此区域"));
        return;
    }
    const QRect spectrogramRect = getSpectrogramRect();
    if (spectrogramRect.width() < 1 || spectrogramRect.height() < 1) return;
    double sceneX = -m_offsetX / m_zoomX;
    double sceneY = -m_offsetY / m_zoomY;
    double sceneWidth = spectrogramRect.width() / m_zoomX;
    double sceneHeight = spectrogramRect.height() / m_zoomY;
    QRectF sourceRectF(std::max(0.0, sceneX), std::max(0.0, sceneY), sceneWidth, sceneHeight);
    QRectF imageRectF(0, 0, m_spectrogramImage.width(), m_spectrogramImage.height());
    sourceRectF = sourceRectF.intersected(imageRectF);
    if (!sourceRectF.isEmpty()) {
        QRectF destinationRect(
            spectrogramRect.left() + m_offsetX + sourceRectF.x() * m_zoomX,
            spectrogramRect.top() + m_offsetY + sourceRectF.y() * m_zoomY,
            sourceRectF.width() * m_zoomX,
            sourceRectF.height() * m_zoomY
        );
        painter.drawImage(destinationRect, m_spectrogramImage, sourceRectF);
    }   
    drawOverlay(painter);
}