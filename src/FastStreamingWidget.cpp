#include "FastStreamingWidget.h"
#include "FastSpectrogramViewer.h"
#include "FastSpectrogramProcessor.h"
#include "RibbonButton.h"
#include "SingleFileExportDialog.h"
#include "ImageEncoderFactory.h"
#include "SpectrogramPainter.h"
#include "PlayerFfmpegProvider.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QtConcurrent>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QToolButton>
#include <QDir>
#include <algorithm>

FastStreamingWidget::FastStreamingWidget(QWidget *parent) 
    : SpectrogramWidgetBase(parent)
{
    m_processor = new FastSpectrogramProcessor(this);
    connect(m_processor, &FastSpectrogramProcessor::logMessage, this, &FastStreamingWidget::logMessageGenerated);
    connect(m_processor, &FastSpectrogramProcessor::processingStarted, this, &FastStreamingWidget::onProcessingStarted);
    connect(m_processor, &FastSpectrogramProcessor::chunkReady, this, &FastStreamingWidget::onChunkReady);
    connect(m_processor, &FastSpectrogramProcessor::processingFinished, this, &FastStreamingWidget::onProcessingFinished);
    connect(m_processor, &FastSpectrogramProcessor::processingFailed, this, &FastStreamingWidget::onProcessingFailed);
    setupBaseUI();
    m_viewer = new FastSpectrogramViewer(this);
    m_viewerLayout->addWidget(m_viewer);
    setupConnections();
    m_viewer->setShowGrid(m_btnGrid->isChecked());
    m_viewer->setVerticalZoomEnabled(m_btnZoom->isChecked());
    m_viewer->setComponentsVisible(m_btnComponents->isChecked());
    retranslateBaseUi();
}

FastStreamingWidget::~FastStreamingWidget() {
    stopStream();
}

void FastStreamingWidget::setupConnections() {
    connect(m_viewer, &FastSpectrogramViewer::fileDropped, this, [this](const QString& path) {
        handleFileOpen(path);
        });
    connect(m_viewer, &FastSpectrogramViewer::seekRequested, this, &FastStreamingWidget::seekRequested);
    connect(this, &SpectrogramWidgetBase::gridVisibilityChanged, m_viewer, &FastSpectrogramViewer::setShowGrid);
    connect(this, &SpectrogramWidgetBase::zoomToggleChanged, m_viewer, &FastSpectrogramViewer::setVerticalZoomEnabled);
    connect(this, &SpectrogramWidgetBase::componentsVisibilityChanged, m_viewer, &FastSpectrogramViewer::setComponentsVisible);
    connect(this, &SpectrogramWidgetBase::curveSettingsChanged, m_viewer, &FastSpectrogramViewer::setCurveType);
    connect(this, &SpectrogramWidgetBase::paletteSettingsChanged, this, [this](PaletteType type) {
        m_viewer->setPaletteType(type);
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("配色方案已变更"));
        });
    connect(this, &SpectrogramWidgetBase::dbSettingsChanged, this, [this](double min, double max) {
        m_viewer->setMinDb(min);
        m_viewer->setMaxDb(max);
        });
    connect(this, &SpectrogramWidgetBase::parameterChanged, this, &FastStreamingWidget::restartStream);
    connect(this, &SpectrogramWidgetBase::openFileRequested, this, &FastStreamingWidget::onOpenFileClicked);
    connect(this, &SpectrogramWidgetBase::saveFileRequested, this, &FastStreamingWidget::onSaveClicked);
}

void FastStreamingWidget::retranslateBaseUi() {
    SpectrogramWidgetBase::retranslateBaseUi();
    if (m_isCueMode && m_currentCueSheet.has_value()) {
        int displayNum = m_currentCueTrackIndex + 1;
        if (m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
            displayNum = m_currentCueSheet->tracks[m_currentCueTrackIndex].number;
        }
        m_btnTrack->setText(QString(tr("分轨:%1")).arg(displayNum, 2, 10, QChar('0')));
    }
    else {
        if (m_currentTrackIdx == -1) {
            m_btnTrack->setText(tr("音轨:自动"));
        }
        else {
            m_btnTrack->setText(QString(tr("音轨:%1")).arg(m_currentTrackIdx));
        }
    }
    if (m_currentChannelIdx == -1) {
        m_btnChannel->setText(tr("声道:合并"));
    }
    else {
        QString chName;
        if (m_currentMetadata.tracks.size() > 0) {
            for (const auto& t : m_currentMetadata.tracks) {
                if (t.index == m_currentMetadata.currentTrackIndex) {
                    if (m_currentChannelIdx < t.channelNames.size())
                        chName = t.channelNames[m_currentChannelIdx];
                    break;
                }
            }
        }
        if (chName.isEmpty()) chName = QString("%1").arg(m_currentChannelIdx + 1);
        m_btnChannel->setText(tr("声道:") + chName);
    }
}

void FastStreamingWidget::stopStream() {
    if (m_processor->isRunning()) {
        m_processor->stopProcessing();
        m_processor->wait();
    }
}

void FastStreamingWidget::restartStream() {
    emit logMessageGenerated("-----------------------------------------------------------------------");
    emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("参数已经变更"));
    startStream();
}

void FastStreamingWidget::startStream() {
    if (m_currentFilePath.isEmpty()) return;
    stopStream();
    if (m_isAutoPrecision) updateAutoPrecision();
    emit playerProviderReady(nullptr, 0.0);
    m_viewer->reset();
    int fftSize = getRequiredFftSize(m_currentHeight);
    double intervalParam = m_isAutoPrecision ? 0.0 : m_currentInterval;
    double startSec = 0.0;
    double endSec = 0.0;
    std::optional<CueSheet> cueSheetToSend = std::nullopt;
    if (m_isCueMode && m_currentCueSheet.has_value()) {
        cueSheetToSend = m_currentCueSheet; 
        if (m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
            const auto& t = m_currentCueSheet->tracks[m_currentCueTrackIndex];
            startSec = t.startSeconds;
            endSec = t.endSeconds;
        }
    }
    m_processor->startProcessing(
        m_currentFilePath, 
        intervalParam, 
        m_currentHeight, 
        fftSize, 
        m_currentCurveType, 
        m_currentMinDb, 
        m_currentMaxDb, 
        m_currentPaletteType, 
        m_currentWindowType, 
        m_currentTrackIdx, 
        m_currentChannelIdx,
        startSec, 
        endSec,
        cueSheetToSend
    );
}

void FastStreamingWidget::onProcessingStarted(const FastTypes::FastAudioMetadata& metadata) {
    m_currentMetadata = metadata;
    if (metadata.preciseDurationMicroseconds > 0) {
        m_preciseDurationStr = FastUtils::formatPreciseDuration(metadata.preciseDurationMicroseconds);
    } else {
        m_preciseDurationStr.clear();
    }
    if (!m_isCueMode) {
        QMenu* trackMenu = m_btnTrack->menu();
        trackMenu->clear();
        QAction* actAuto = trackMenu->addAction(tr("自动选择"));
        connect(actAuto, &QAction::triggered, this, [=]() { 
            m_btnTrack->setText(tr("音轨:自动")); 
            onTrackActionTriggered(-1); 
        });
        for (const auto& trk : metadata.tracks) {
            QAction* act = trackMenu->addAction(trk.name);
            connect(act, &QAction::triggered, this, [=]() {
                m_btnTrack->setText(QString(tr("音轨:%1")).arg(trk.index)); 
                onTrackActionTriggered(trk.index);
            });
            if (trk.index == metadata.currentTrackIndex) {
                m_btnTrack->setText(QString(tr("音轨:%1")).arg(trk.index));
            }
        }
    }
    QMenu* chMenu = m_btnChannel->menu();
    chMenu->clear();
    QAction* actMix = chMenu->addAction(tr("合并所有声道"));
    connect(actMix, &QAction::triggered, this, [=]() { 
        m_btnChannel->setText(tr("声道:合并")); 
        onChannelActionTriggered(-1); 
    });
    for (const auto& trk : metadata.tracks) {
        if (trk.index == metadata.currentTrackIndex) {
            for (int i=0; i<trk.channels; ++i) {
                QString name = (i < trk.channelNames.size()) ? trk.channelNames[i] : QString::number(i+1);
                QAction* act = chMenu->addAction(name);
                connect(act, &QAction::triggered, this, [=]() { 
                    m_btnChannel->setText(tr("声道:") + name); 
                    onChannelActionTriggered(i); 
                });
            }
            break;
        }
    }
    updateAutoPrecision();
    int fftSize = getRequiredFftSize(m_currentHeight);
    long long totalSamplesApprox = static_cast<long long>(metadata.sampleRate * metadata.durationSec + 0.5);
    if (totalSamplesApprox < fftSize) {
        QString errorMsg = QString(tr("音频时长过短（总采样%1） 无法进行%2点傅里叶变换"))
                           .arg(totalSamplesApprox).arg(fftSize);
        onProcessingFailed(errorMsg);
        stopStream();
        return; 
    }
    double effectiveInterval = (m_currentInterval <= 0.0) ? (static_cast<double>(fftSize) / metadata.sampleRate) : m_currentInterval;
    int hopSize = std::max(1, static_cast<int>(metadata.sampleRate * effectiveInterval + 0.5));
    int estimatedWidth;
    if (m_isAutoPrecision) {
        estimatedWidth = static_cast<int>((totalSamplesApprox / fftSize) + 1);
    } else {
        estimatedWidth = static_cast<int>(((totalSamplesApprox - fftSize) / hopSize) + 1);
    }
    if (estimatedWidth < 1) estimatedWidth = 1;
    m_viewer->initCanvas(estimatedWidth, m_currentHeight);
    m_viewer->setAudioProperties(metadata.durationSec, m_preciseDurationStr, metadata.sampleRate);
}

void FastStreamingWidget::onChunkReady(const QImage& chunk, double startTime, double duration) {
    Q_UNUSED(startTime); Q_UNUSED(duration);
    m_viewer->appendChunk(chunk);
}

void FastStreamingWidget::onProcessingFinished(double realDuration){
    setBaseControlsEnabled(true);
    m_viewer->setFinished();
    m_currentMetadata.durationSec = realDuration;
    m_currentMetadata.preciseDurationMicroseconds = static_cast<long long>(realDuration * 1000000.0);
    m_preciseDurationStr = FastUtils::formatPreciseDuration(m_currentMetadata.preciseDurationMicroseconds);
    m_viewer->setAudioProperties(m_currentMetadata.durationSec, m_preciseDurationStr, m_currentMetadata.sampleRate);
    const QImage& finalImage = m_viewer->getFullSpectrogram();
    if (!finalImage.isNull()) {
        emit logMessageGenerated(QString(tr("        [分辨率] %1×%2"))
            .arg(finalImage.width())
            .arg(finalImage.height()));
    }
    emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("图像已经显示"));
    double startSec = 0.0;
    double endSec = 0.0;
    if (m_isCueMode && m_currentCueSheet.has_value()) {
        if (m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
            const auto& t = m_currentCueSheet->tracks[m_currentCueTrackIndex];
            startSec = t.startSeconds;
            endSec = t.endSeconds;
        }
    }
    auto rawProvider = new PlayerFfmpegProvider(
        m_currentFilePath, 
        m_currentTrackIdx, 
        m_currentChannelIdx, 
        startSec, 
        endSec,
        16 * 1024 * 1024
    );
    if (rawProvider->isValid()) {
        emit playerProviderReady(QSharedPointer<IPlayerProvider>(rawProvider), m_currentMetadata.durationSec);
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("播放器已就绪"));
    }
    else {
        delete rawProvider;
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("播放器初始化失败"));
    }
}

void FastStreamingWidget::onProcessingFailed(const QString& msg) {
    setBaseControlsEnabled(true);
    emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("处理失败：") + msg);
    QMessageBox::critical(this, tr("流式处理失败"), msg);
}

void FastStreamingWidget::onTrackActionTriggered(int trackIdx) {
    if (m_isCueMode) {
        if (!m_currentCueSheet.has_value()) return;
        if (trackIdx < 0 || trackIdx >= m_currentCueSheet->tracks.size()) return;
        if (trackIdx == m_currentCueTrackIndex) return;
        m_currentCueTrackIndex = trackIdx;
        const auto& t = m_currentCueSheet->tracks[trackIdx];
        QString performer = t.performer;
        if (performer.isEmpty()) performer = m_currentCueSheet->albumPerformer;
        m_btnTrack->setText(QString(tr("分轨:%1")).arg(t.number, 2, 10, QChar('0')));
        emit logMessageGenerated("-----------------------------------------------------------------------");
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("切换分轨：%1 - %2 - %3")
            .arg(t.number, 2, 10, QChar('0'))
            .arg(performer)
            .arg(t.title));
        startStream(); 
        return;
    }
    int targetPhysicalIndex = (trackIdx == -1) ? m_currentMetadata.defaultTrackIndex : trackIdx;
    int currentPhysicalIndex = m_currentMetadata.currentTrackIndex;
    if (targetPhysicalIndex == currentPhysicalIndex) {
        if (trackIdx == -1) m_btnTrack->setText(tr("音轨:自动"));
        else m_btnTrack->setText(QString(tr("音轨:%1")).arg(trackIdx));
        return;
    }
    m_currentTrackIdx = trackIdx;
    m_currentChannelIdx = -1;
    m_btnChannel->setText(tr("声道:合并"));
    emit logMessageGenerated("-----------------------------------------------------------------------");
    emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("切换音轨"));
    startStream();
}

void FastStreamingWidget::onChannelActionTriggered(int channelIdx) {
    if (m_currentChannelIdx == channelIdx) return;
    m_currentChannelIdx = channelIdx;
    emit logMessageGenerated("-----------------------------------------------------------------------");
    emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("切换声道"));
    startStream();
}

void FastStreamingWidget::onOpenFileClicked() {
    const QString fileFilter = tr("所有文件 (*.*)");
    QString path = QFileDialog::getOpenFileName(this, tr("打开文件"), m_lastOpenPath, fileFilter);
    if (!path.isEmpty()) {
        m_lastOpenPath = QFileInfo(path).path();
        handleFileOpen(path);
    }
}

void FastStreamingWidget::onSaveClicked() {
    QImage fullImage = m_viewer->getFullSpectrogram();
    if (fullImage.isNull() || fullImage.width() <= 1) {
        QMessageBox::warning(this, tr("提示"), tr("没有可保存的图像"));
        return;
    }
    QImage imageToSave = fullImage;
    if (m_currentHeight != imageToSave.height())
        imageToSave = imageToSave.scaled(
            imageToSave.width(),
            m_currentHeight,
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation
        );
    if (m_widthLimitEnabled && imageToSave.width() > m_widthLimitValue)
        imageToSave = imageToSave.scaled(
            m_widthLimitValue,
            imageToSave.height(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation
        );
    bool drawComponents = m_btnComponents->isChecked();
    int finalW = imageToSave.width();
    int finalH = imageToSave.height();
    if (drawComponents) {
        finalW += FastConfig::PNG_SAVE_LEFT_MARGIN + FastConfig::PNG_SAVE_RIGHT_MARGIN;
        finalH += FastConfig::PNG_SAVE_TOP_MARGIN + FastConfig::PNG_SAVE_BOTTOM_MARGIN;
    }
    bool jpgAvail = (finalW <= FastConfig::JPEG_MAX_DIMENSION && finalH <= FastConfig::JPEG_MAX_DIMENSION);
    bool webpAvail = (finalW <= FastConfig::MY_WEBP_MAX_DIMENSION && finalH <= FastConfig::MY_WEBP_MAX_DIMENSION);
    bool avifAvail = (finalW <= FastConfig::MY_AVIF_MAX_DIMENSION && finalH <= FastConfig::MY_AVIF_MAX_DIMENSION);
    QString defaultBaseName;
    QFileInfo fi(m_currentFilePath);
    QString sourceExtension = fi.suffix();
    if (m_isCueMode && m_currentCueSheet.has_value() && 
        m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
        const auto& t = m_currentCueSheet->tracks[m_currentCueTrackIndex];
        QString performer = t.performer;
        if (performer.isEmpty()) performer = m_currentCueSheet->albumPerformer;
        if (!performer.isEmpty() && !t.title.isEmpty()) {
            defaultBaseName = QString("%1 - %2").arg(performer).arg(t.title);
        } else if (!t.title.isEmpty()) {
            defaultBaseName = t.title;
        } else {
            defaultBaseName = fi.completeBaseName();
        }
        if (!sourceExtension.isEmpty()) {
            defaultBaseName += "." + sourceExtension;
        }
        defaultBaseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    } 
    else {
        defaultBaseName = fi.fileName();
    }
    SingleFileExportDialog dialog(m_lastSavePath, defaultBaseName, jpgAvail, webpAvail, avifAvail, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString outPath = dialog.getSelectedFilePath();
        m_lastSavePath = QFileInfo(outPath).path();
        if (QFile::exists(outPath)) {
            QMessageBox msgBox(QMessageBox::Question, tr("覆盖"), tr("文件已存在，是否覆盖？"), QMessageBox::Yes | QMessageBox::No, this);
            msgBox.setButtonText(QMessageBox::Yes, tr("是"));
            msgBox.setButtonText(QMessageBox::No, tr("否"));
            msgBox.setDefaultButton(QMessageBox::No);
            if (msgBox.exec() != QMessageBox::Yes) {
                return;
            }
        }
        QString fmt = dialog.getSelectedFormatIdentifier();
        int quality = dialog.qualityLevel();
        bool grid = m_btnGrid->isChecked();
        setBaseControlsEnabled(false);
        if (m_btnSave) m_btnSave->setText(tr("保存中"));
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("正在保存图像"));
        QFuture<bool> future = QtConcurrent::run([=]() {
            SpectrogramPainter painter;
            QImage finalImage = painter.drawFinalImage(
                imageToSave,
                defaultBaseName,
                m_currentMetadata.durationSec,
                grid,
                m_preciseDurationStr,
                m_currentMetadata.sampleRate,
                m_currentCurveType,
                m_currentMinDb,
                m_currentMaxDb,
                m_currentPaletteType,
                drawComponents
            );
            if (finalImage.isNull()) return false;
            auto encoder = ImageEncoderFactory::createEncoder(fmt);
            return encoder ? encoder->encodeAndSave(finalImage, outPath, quality) : false;
            });
        auto watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
            setBaseControlsEnabled(true);
            if (m_btnSave) m_btnSave->setText(tr("保存"));
            if (watcher->result()) {
                emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("保存成功：%1").arg(outPath));
                QMessageBox msgBox(QMessageBox::Information, tr("成功"), tr("保存成功"), QMessageBox::Ok, this);
                msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
                msgBox.exec();
            }
            else {
                emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("保存失败"));
                QMessageBox msgBox(QMessageBox::Critical, tr("失败"), tr("保存图像失败"), QMessageBox::Ok, this);
                msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
                msgBox.exec();
            }
            watcher->deleteLater();
            });
        watcher->setFuture(future);
    }
}

void FastStreamingWidget::updateAutoPrecision() {
    if (!m_isAutoPrecision) return;
    if (m_currentMetadata.sampleRate <= 0) { 
        m_btnInterval->setText(tr("精度:自动")); 
        return; 
    }
    int fftSize = getRequiredFftSize(m_currentHeight);
    double val = static_cast<double>(fftSize) / static_cast<double>(m_currentMetadata.sampleRate);
    m_btnInterval->setText(QString(tr("精度:%1")).arg(val, 0, 'f', 3));
    m_currentInterval = val;
}

void FastStreamingWidget::setPlayheadPosition(double seconds) {
    if (m_viewer) {
        m_viewer->setPlayheadPosition(seconds);
    }
}

void FastStreamingWidget::setPlayheadVisible(bool visible) {
    if (m_viewer) {
        m_viewer->setPlayheadVisible(visible);
    }
}

void FastStreamingWidget::handleFileOpen(const QString& filePath) {
    if (filePath.isEmpty()) return;
    bool isCue = filePath.endsWith(".cue", Qt::CaseInsensitive);
    if (isCue && m_isCueMode && filePath == m_originalCueFilePath) { return; } 
    else if (!isCue && !m_isCueMode && filePath == m_currentFilePath) { return; }
    emit playerProviderReady(nullptr, 0.0);
    if (isCue) {
        emit logMessageGenerated("=======================================================================");
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("启动流式处理"));
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("解析文件"));
        auto sheetOpt = CueParser::parse(filePath);
        if (!sheetOpt.has_value()) {
            QMessageBox::critical(this, tr("解析失败"), tr("无法解析 CUE 文件"));
            return;
        }
        CueSheet sheet = sheetOpt.value();
        QString audioPath = findAudioFileForCue(filePath, sheet.audioFilename);
        emit logMessageGenerated(tr("    文件信息："));
        emit logMessageGenerated(tr("        [路径] %1").arg(filePath));
        emit logMessageGenerated(tr("        [歌手] %1").arg(sheet.albumPerformer));
        emit logMessageGenerated(tr("        [专辑] %1").arg(sheet.albumTitle));
        emit logMessageGenerated(tr("        [日期] %1").arg(sheet.year));
        emit logMessageGenerated(tr("        [关联] %1").arg(audioPath.isEmpty() ? tr("未找到关联文件") : QFileInfo(audioPath).fileName()));
        if (audioPath.isEmpty()) {
            QMessageBox msgBox(QMessageBox::Critical, tr("文件缺失"), 
                tr("无法找到 CUE 对应的音频文件。\n目标: %1").arg(sheet.audioFilename),
                QMessageBox::Ok, this);
            msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
            msgBox.exec();
            return;
        }
        m_isCueMode = true;
        m_currentCueSheet = sheet;
        m_originalCueFilePath = filePath;
        m_currentFilePath = audioPath;
        emit filePathChanged(filePath); 
        updateTrackMenuFromCue();
        m_currentCueTrackIndex = 0;
        int dispNum = sheet.tracks.empty() ? 1 : sheet.tracks[0].number;
        m_btnTrack->setText(QString(tr("分轨:%1")).arg(dispNum, 2, 10, QChar('0')));
        m_currentTrackIdx = -1;
        m_currentChannelIdx = -1;
        m_btnChannel->setText(tr("声道:合并"));
        startStream();
    } 
    else {
        emit logMessageGenerated("=======================================================================");
        emit logMessageGenerated(FastUtils::getCurrentTimestamp() + " " + tr("启动流式处理"));
        m_isCueMode = false;
        m_currentCueSheet.reset();
        m_originalCueFilePath.clear();
        m_currentFilePath = filePath;
        emit filePathChanged(filePath);
        m_currentTrackIdx = -1;
        m_currentChannelIdx = -1;
        m_btnTrack->setText(tr("音轨:自动"));
        m_btnChannel->setText(tr("声道:合并"));
        startStream();
    }
}

QString FastStreamingWidget::findAudioFileForCue(const QString& cuePath, const QString& cueAudioTarget) {
    QFileInfo cueInfo(cuePath);
    QDir dir = cueInfo.dir();
    QString directPath = dir.filePath(cueAudioTarget);
    if (QFile::exists(directPath)) return directPath;
    QString baseName = QFileInfo(cueAudioTarget).completeBaseName();
    if (baseName.isEmpty()) baseName = cueInfo.completeBaseName();
    QStringList filters;
    filters << "*.flac" << "*.ape" << "*.wav" << "*.dsf" << "*.dff" << "*.tak" << "*.tta" << "*.mp3" << "*.m4a";
    for (const auto& filter : filters) {
        QString ext = filter.mid(1);
        QString candidate = dir.filePath(baseName + ext);
        if (QFile::exists(candidate)) return candidate;
    }
    dir.setNameFilters(filters);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Size);
    if (!fileList.isEmpty()) {
        std::sort(fileList.begin(), fileList.end(), [](const QFileInfo& a, const QFileInfo& b) {
            return a.size() < b.size();
            });
        return fileList.last().absoluteFilePath();
    }
    return QString();
}

void FastStreamingWidget::updateTrackMenuFromCue() {
    if (!m_currentCueSheet.has_value()) return;
    QMenu* trackMenu = m_btnTrack->menu();
    trackMenu->clear();
    const auto& tracks = m_currentCueSheet->tracks;
    for (int i = 0; i < tracks.size(); ++i) {
        const auto& t = tracks[i];
        QString performer = t.performer;
        if (performer.isEmpty()) performer = m_currentCueSheet->albumPerformer;
        QString label = QString("%1: %2 - %3")
            .arg(t.number, 2, 10, QChar('0'))
            .arg(performer)
            .arg(t.title);
        QAction* act = trackMenu->addAction(label);
        connect(act, &QAction::triggered, this, [=]() {
            onTrackActionTriggered(i);
        });
    }
}

void FastStreamingWidget::updateCrosshairStyle(const CrosshairStyle& style, bool enabled) {
    if (m_viewer) {
        m_viewer->setCrosshairStyle(style);
        m_viewer->setCrosshairEnabled(enabled);
    }
}

void FastStreamingWidget::updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type) {
    if (m_viewer) {
        m_viewer->setSpectrumProfileStyle(visible, color, lineWidth, filled, alpha, type);
    }
}

void FastStreamingWidget::updatePlayheadStyle(const PlayheadStyle& style) {
    if (m_viewer) {
        m_viewer->setPlayheadStyle(style);
    }
}

void FastStreamingWidget::setProfileFrameRate(int fps) {
    if (m_viewer) {
        m_viewer->setProfileFrameRate(fps);
    }
}

void FastStreamingWidget::updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) {
    if (m_viewer) {
        m_viewer->setDataConfig(spectrumSrc, probeSrc, precision);
    }
}

void FastStreamingWidget::setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) {
    if (m_viewer) {
        m_viewer->setIndicatorVisibility(showFreq, showTime, showDb);
    }
}


void FastStreamingWidget::applyGlobalPreferences(const GlobalPreferences& prefs, bool silent) {
    SpectrogramWidgetBase::applyGlobalPreferences(prefs, silent);
    if (m_viewer) {
        m_viewer->setShowGrid(prefs.showGrid);
        m_viewer->setComponentsVisible(prefs.showComponents);
        m_viewer->setVerticalZoomEnabled(prefs.enableZoom);
        m_viewer->setPaletteType(prefs.paletteType);
        m_viewer->setMinDb(prefs.minDb);
        m_viewer->setMaxDb(prefs.maxDb);
        m_viewer->setCurveType(prefs.curveType);
        m_viewer->setDataConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        m_viewer->setGpuEnabled(prefs.enableGpuAcceleration);
    }
}