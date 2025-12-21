#include "SpectrogramViewer.h"
#include "AppConfig.h"
#include "MappingCurves.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <QPainter>
#include <QDateTime>
#include <QMimeData>
#include <QDropEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QLinearGradient>
#include <QDragEnterEvent>


int SpectrogramViewer::calculateBestFreqStep(double maxFreqKhz, int availableHeight) const {
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


int SpectrogramViewer::calculateBestDbStep(double dbRange, int availableHeight) const {
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


SpectrogramViewer::SpectrogramViewer(QWidget* parent)
    : QWidget(parent),
      m_currentCurveType(CurveType::Function0)
{
    setAcceptDrops(true);
    setMinimumSize(400, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_spectrogramImage = QImage(1, 1, QImage::Format_RGB32);
    m_spectrogramImage.fill(Qt::black);
    
    setMouseTracking(true);
}


QRect SpectrogramViewer::getSpectrogramRect() const {
    return QRect(
        AppConfig::VIEWER_LEFT_MARGIN,
        AppConfig::VIEWER_TOP_MARGIN,
        width() - AppConfig::VIEWER_LEFT_MARGIN - AppConfig::VIEWER_RIGHT_MARGIN,
        height() - AppConfig::VIEWER_TOP_MARGIN - AppConfig::VIEWER_BOTTOM_MARGIN);
}


void SpectrogramViewer::setSpectrogramImage(const QImage& image) {
    m_spectrogramImage = image.isNull() ? QImage(1, 1, QImage::Format_RGB32) : image;

    if (!m_spectrogramImage.isNull() && m_spectrogramImage.width() > 1) {
        QRect viewRect = getSpectrogramRect();

        double newZoomX, newZoomY;

        if (m_spectrogramImage.width() > viewRect.width()) {
            newZoomX = static_cast<double>(viewRect.width()) / m_spectrogramImage.width();} 
        else {
            newZoomX = 1.0;}

        if (m_spectrogramImage.height() > viewRect.height()) {
            newZoomY = static_cast<double>(viewRect.height()) / m_spectrogramImage.height();} 
        else {
            newZoomY = 1.0;}
        
        if (m_isVerticalZoomEnabled) {
             m_zoomX = m_zoomY = std::min(newZoomX, newZoomY);}
        else {
            m_zoomX = newZoomX;
            m_zoomY = newZoomY;}}
    else {
        m_zoomX = m_zoomY = 1.0;}

    m_offsetX = 0;
    m_offsetY = 0;
    clampViewPosition();
    update();
}


void SpectrogramViewer::setAudioProperties(double duration, const QString& preciseDurationStr, int nativeSampleRate) {
    m_audioDurationSec = duration;
    m_preciseDurationStr = preciseDurationStr;
    m_nativeSampleRate = nativeSampleRate;
    update();
}


void SpectrogramViewer::setShowGrid(bool show) {
    m_showGrid = show;
    update();
}


void SpectrogramViewer::setLoadingState(bool loading) {
    m_isLoading = loading;
    update();
}


void SpectrogramViewer::setVerticalZoomEnabled(bool enabled) {
    m_isVerticalZoomEnabled = enabled;
}


void SpectrogramViewer::setCurveType(CurveType type){
    if (m_currentCurveType != type) {
        m_currentCurveType = type;
        update();}
}


void SpectrogramViewer::paintEvent(QPaintEvent* event){
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), QColor(34, 41, 51));

    if (m_isLoading) {
        painter.setPen(QColor(180, 180, 180));
        QFont f = font();
        f.setPointSize(15);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter, tr("Processing audio file... This may take some time"));
        return;}
    if (m_spectrogramImage.width() <= 1 || m_nativeSampleRate == 0) {
        painter.setPen(QColor(180, 180, 180));
        QFont f = font();
        f.setPointSize(15);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter, tr("Please use the Open button or drag an audio file into here"));
        return;}

    const QRect spectrogramRect = getSpectrogramRect();
    if (spectrogramRect.width() < 1 || spectrogramRect.height() < 1) return;

    double sceneX = -m_offsetX / m_zoomX;
    double sceneY = -m_offsetY / m_zoomY;
    double sceneWidth = spectrogramRect.width() / m_zoomX;
    double sceneHeight = spectrogramRect.height() / m_zoomY;
    QRect sourceRect(
        std::max(0, static_cast<int>(sceneX)),
        std::max(0, static_cast<int>(sceneY)),
        static_cast<int>(sceneWidth + 1.0),
        static_cast<int>(sceneHeight + 1.0));
    sourceRect = sourceRect.intersected(m_spectrogramImage.rect());

    if (sourceRect.isValid()) {
        QImage visibleChunk = m_spectrogramImage.copy(sourceRect);
        QRectF destinationRect(
            spectrogramRect.left() + m_offsetX + sourceRect.x() * m_zoomX,
            spectrogramRect.top() + m_offsetY + sourceRect.y() * m_zoomY,
            sourceRect.width() * m_zoomX,
            sourceRect.height() * m_zoomY);
        painter.drawImage(destinationRect, visibleChunk);}
    
    painter.setPen(QColor(200, 200, 200));
    painter.setFont(QFont("Arial", 8));

    const double maxFreqKhz = (m_nativeSampleRate / 2.0) / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, spectrogramRect.height());
    
    painter.drawText(QRect(0, spectrogramRect.top() - 21, AppConfig::VIEWER_LEFT_MARGIN - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");
    
    QFontMetrics fm_khz(painter.font());
    const int minLabelSpacing = fm_khz.height();
    int lastDrawnLabelY = -1000;

    for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
        if (khz > maxFreqKhz) break;
        
        double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
        double curved_ratio = MappingCurves::map(linear_ratio, m_currentCurveType);
        
        double y_in_image = (1.0 - curved_ratio) * (m_spectrogramImage.height() - 1);
        int y_on_screen = static_cast<int>(spectrogramRect.top() + m_offsetY + y_in_image * m_zoomY);

        if (std::abs(y_on_screen - lastDrawnLabelY) >= minLabelSpacing) {
            QRect freqValueRect(0, y_on_screen - 10, AppConfig::VIEWER_LEFT_MARGIN - 5, 20);
            painter.drawText(freqValueRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
            lastDrawnLabelY = y_on_screen;}}
    
    double linear_ratio_max = 1.0;
    if (maxFreqKhz > 0) { // Avoid division by zero if sample rate is 0
        double max_khz_rounded = std::floor(maxFreqKhz / freqStep) * freqStep;
        linear_ratio_max = max_khz_rounded / maxFreqKhz;}
    double curved_ratio_max = MappingCurves::map(linear_ratio_max, m_currentCurveType);
    double y_in_image_max = (1.0 - curved_ratio_max) * (m_spectrogramImage.height() - 1);
    int y_on_screen_max = static_cast<int>(spectrogramRect.top() + m_offsetY + y_in_image_max * m_zoomY);

    if (std::abs(y_on_screen_max - lastDrawnLabelY) >= minLabelSpacing) {
        painter.drawText(QRect(0, y_on_screen_max - 10, AppConfig::VIEWER_LEFT_MARGIN - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(static_cast<int>(maxFreqKhz)));}

    QRect colorBarArea(width() - AppConfig::VIEWER_RIGHT_MARGIN + 5, spectrogramRect.top(), AppConfig::VIEWER_COLOR_BAR_WIDTH, spectrogramRect.height());
    QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
    auto colorMap = AppConfig::getColorMap();
    for (size_t i = 0; i < colorMap.size(); ++i) {
        gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);}
    painter.fillRect(colorBarArea, gradient);
    
    painter.drawText(QRect(width() - AppConfig::VIEWER_RIGHT_MARGIN + 14, spectrogramRect.top() - 17, 40, 20), Qt::AlignHCenter, "dB");
    
    const double dbRange = AppConfig::MAX_DB - AppConfig::MIN_DB;
    const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
    if (dbRange > 0 && dbStep > 0) {
        double start_db = std::ceil(AppConfig::MAX_DB / dbStep) * dbStep;
        for (double db = start_db; db >= AppConfig::MIN_DB; db -= dbStep) {
            int y = colorBarArea.top() + static_cast<int>(((AppConfig::MAX_DB - db) / dbRange) * colorBarArea.height());
            painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));}}
    
    if (m_audioDurationSec > 0) {
        double pixelsPerSec = static_cast<double>(m_spectrogramImage.width()) * m_zoomX / m_audioDurationSec;
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 50;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; } else { break; }}

        if (m_showGrid) {
            painter.save();
            painter.setClipRect(spectrogramRect);
            painter.translate(0.5, 0.5);
            
            QPen gridPen(AppConfig::VIEWER_GRID_LINE_COLOR, 1, Qt::DotLine);
            painter.setPen(gridPen);

            for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
                if (khz == 0) continue;
                
                double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
                double curved_ratio = MappingCurves::map(linear_ratio, m_currentCurveType);
                
                double y_in_image = (1.0 - curved_ratio) * (m_spectrogramImage.height() - 1);
                int y_on_screen = static_cast<int>(spectrogramRect.top() + m_offsetY + y_in_image * m_zoomY);

                if (y_on_screen >= spectrogramRect.top() && y_on_screen <= spectrogramRect.bottom()) {
                    painter.drawLine(spectrogramRect.left(), y_on_screen, spectrogramRect.right(), y_on_screen);}}
            
            double startTimeInView = -(m_offsetX) / pixelsPerSec;
            double endTimeInView = startTimeInView + spectrogramRect.width() / pixelsPerSec;
            long long firstTick = (static_cast<long long>(startTimeInView / timeStep) + 1) * timeStep;
            
            if (firstTick < timeStep) firstTick = timeStep;
            
            for (long long t = firstTick; t < endTimeInView && t < m_audioDurationSec; t += timeStep) {
                 int x_pos = static_cast<int>(spectrogramRect.left() + t * pixelsPerSec + m_offsetX);
                 if (x_pos > spectrogramRect.left() && x_pos < spectrogramRect.right()) {
                    painter.drawLine(x_pos, spectrogramRect.top(), x_pos, spectrogramRect.bottom());}}

            painter.restore();}
        
        painter.setPen(QColor(200, 200, 200));
        QFontMetrics fm(painter.font());
        QString totalTimeLabel = m_preciseDurationStr.isEmpty() ?
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(m_audioDurationSec), Qt::UTC).toString(m_audioDurationSec >= 3600 ? "h:mm:ss" : "m:ss") :
            m_preciseDurationStr;
            
        double scaledWidth = m_spectrogramImage.width() * m_zoomX;
        bool isFullyVisible = (scaledWidth <= spectrogramRect.width());
        bool isAtRightEnd = std::abs(m_offsetX - (spectrogramRect.width() - scaledWidth)) < 1.0;
        
        QRect totalRect;
        bool showTotalLabel = isFullyVisible || isAtRightEnd;

        if (showTotalLabel) {
            double totalTimeX_pixel = spectrogramRect.left() + m_audioDurationSec * pixelsPerSec + m_offsetX;
            int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
            const int padding = 5;
            int centerWidth = totalLabelWidth + 2 * padding;
            totalRect = QRect(static_cast<int>(totalTimeX_pixel) - centerWidth / 2, spectrogramRect.bottom() + 5, centerWidth, AppConfig::VIEWER_BOTTOM_MARGIN);}
        
        double startTimeInView = -(m_offsetX) / pixelsPerSec;
        double endTimeInView = startTimeInView + spectrogramRect.width() / pixelsPerSec;
        long long firstTick = (static_cast<long long>(startTimeInView / timeStep) + 1) * timeStep;

        if (firstTick < timeStep) {
            firstTick = timeStep;}

        for (long long t = firstTick; t < endTimeInView && t < m_audioDurationSec; t += timeStep) {
            double x_pos_double = spectrogramRect.left() + t * pixelsPerSec + m_offsetX;
            int x_pos = static_cast<int>(x_pos_double);

            QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
            int labelWidth = fm.horizontalAdvance(timeLabel);
            const int padding = 5;
            int centerWidth = labelWidth + 2 * padding;
            QRect labelRect(x_pos - centerWidth / 2, spectrogramRect.bottom() + 5, centerWidth, AppConfig::VIEWER_BOTTOM_MARGIN);

            if (labelRect.right() < spectrogramRect.left() || labelRect.left() > spectrogramRect.right()) continue;

            if (showTotalLabel && !totalRect.isNull() && labelRect.intersects(totalRect)) {
                continue;}
            
            painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);}

        if (showTotalLabel && !totalRect.isNull()) {
            painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);}
    }
}


void SpectrogramViewer::wheelEvent(QWheelEvent* event) {
    if (m_spectrogramImage.isNull() || m_spectrogramImage.width() <= 1) return;

    double scaleFactor = (event->angleDelta().y() > 0) ? 1.25 : 0.8;
    double newZoomX = m_zoomX * scaleFactor;
    double newZoomY = m_isVerticalZoomEnabled ? (m_zoomY * scaleFactor) : m_zoomY;

    if (newZoomX > 1.0) newZoomX = 1.0;
    if (newZoomY > 1.0) newZoomY = 1.0;

    QRect viewRect = getSpectrogramRect();

    double minZoomX = std::min(1.0, static_cast<double>(viewRect.width()) / m_spectrogramImage.width());
    double minZoomY = std::min(1.0, static_cast<double>(viewRect.height()) / m_spectrogramImage.height());

    if (m_isVerticalZoomEnabled) {
        double minZoom = std::min(minZoomX, minZoomY);
        if (newZoomX < minZoom - 1e-9) {
            newZoomX = newZoomY = minZoom;}}
    else {
        if (newZoomX < minZoomX - 1e-9) {
            newZoomX = minZoomX;}
        if (newZoomY < minZoomY - 1e-9) {
            newZoomY = minZoomY;}}

    if (std::abs(newZoomX - m_zoomX) < 1e-9 && std::abs(newZoomY - m_zoomY) < 1e-9) return;

    QPointF mousePos = event->position();
    QRect spectrogramRect = getSpectrogramRect();
    double sceneX = (mousePos.x() - spectrogramRect.left() - m_offsetX) / m_zoomX;
    double sceneY = (mousePos.y() - spectrogramRect.top() - m_offsetY) / m_zoomY;

    m_zoomX = newZoomX;
    m_zoomY = newZoomY;

    m_offsetX = mousePos.x() - spectrogramRect.left() - sceneX * m_zoomX;
    m_offsetY = mousePos.y() - spectrogramRect.top() - sceneY * m_zoomY;

    clampViewPosition();
    update();
}


void SpectrogramViewer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    if (m_spectrogramImage.isNull() || m_spectrogramImage.width() <= 1) {
        return;}

    QSize oldViewSize = event->oldSize();
    int oldViewWidth = oldViewSize.width() - AppConfig::VIEWER_LEFT_MARGIN - AppConfig::VIEWER_RIGHT_MARGIN;
    int oldViewHeight = oldViewSize.height() - AppConfig::VIEWER_TOP_MARGIN - AppConfig::VIEWER_BOTTOM_MARGIN;
    if (oldViewWidth <= 0 || oldViewHeight <= 0) {
        clampViewPosition();
        update();
        return;}

    double oldFitZoomX = static_cast<double>(oldViewWidth) / m_spectrogramImage.width();
    double oldFitZoomY = static_cast<double>(oldViewHeight) / m_spectrogramImage.height();

    double ratioX = 1.0;
    if (oldFitZoomX > 1e-9) ratioX = m_zoomX / oldFitZoomX;

    double ratioY = 1.0;
    if (oldFitZoomY > 1e-9) ratioY = m_zoomY / oldFitZoomY;

    QRect newViewRect = getSpectrogramRect();
    double newFitZoomX = static_cast<double>(newViewRect.width()) / m_spectrogramImage.width();
    double newFitZoomY = static_cast<double>(newViewRect.height()) / m_spectrogramImage.height();

    m_zoomX = std::min(1.0, newFitZoomX * ratioX);
    m_zoomY = std::min(1.0, newFitZoomY * ratioY);

    clampViewPosition();
    update();
}


void SpectrogramViewer::clampViewPosition() {
    if (m_spectrogramImage.isNull()) return;

    QRect viewRect = getSpectrogramRect();
    double scaledWidth = m_spectrogramImage.width() * m_zoomX;
    double scaledHeight = m_spectrogramImage.height() * m_zoomY;

    if (scaledWidth <= viewRect.width()) {
        m_offsetX = (viewRect.width() - scaledWidth) / 2.0;}
    else {
        m_offsetX = std::min(0.0, m_offsetX);
        m_offsetX = std::max(viewRect.width() - scaledWidth, m_offsetX);}

    if (scaledHeight <= viewRect.height()) {
        m_offsetY = (viewRect.height() - scaledHeight) / 2.0;}
    else {
        m_offsetY = std::min(0.0, m_offsetY);
        m_offsetY = std::max(viewRect.height() - scaledHeight, m_offsetY);}
}


void SpectrogramViewer::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) { event->acceptProposedAction(); }
}


void SpectrogramViewer::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()) {
        emit fileDropped(event->mimeData()->urls().first().toLocalFile());}
}


void SpectrogramViewer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QRect spectrogramRect = getSpectrogramRect();
        if (spectrogramRect.contains(event->pos())) {
            m_isDragging = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept(); }
        else {event->ignore();}}
    else {event->ignore();}
}


void SpectrogramViewer::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        int dx = event->pos().x() - m_lastMousePos.x();
        int dy = event->pos().y() - m_lastMousePos.y();
        m_offsetX += dx;
        m_offsetY += dy;
        m_lastMousePos = event->pos();
        clampViewPosition();
        update();}
}


void SpectrogramViewer::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);}
}