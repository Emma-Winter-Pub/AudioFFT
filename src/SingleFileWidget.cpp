#include "SingleFileWidget.h"
#include "SpectrogramViewer.h"
#include "SpectrogramProcessor.h"
#include "SingleFileExportDialog.h"
#include "AppConfig.h"
#include "Utils.h"
#include "RibbonButton.h"
#include "PlayerRamProvider.h"
#include "PlayerFfmpegProvider.h"
#include "GlobalPreferences.h"

#include <algorithm>
#include <QDir>
#include <QMenu>
#include <QTime>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>
#include <QRegularExpression>

SingleFileWidget::SingleFileWidget(QWidget* parent)
    : SpectrogramWidgetBase(parent)
{
    setupBaseUI();
    m_spectrogramViewer = new SpectrogramViewer(this);
    m_viewerLayout->addWidget(m_spectrogramViewer);
    m_fftProvider = new FftSpectrumDataProvider();
    setupWorkerThread();    
    setupConnections();
    m_spectrogramViewer->setShowGrid(m_btnGrid->isChecked());
    m_spectrogramViewer->setVerticalZoomEnabled(m_btnZoom->isChecked());
    m_spectrogramViewer->setComponentsVisible(m_btnComponents->isChecked());
    retranslateBaseUi();
}

SingleFileWidget::~SingleFileWidget() {
    m_workerThread.quit();
    m_workerThread.wait();
    if (m_fftProvider) {
        delete m_fftProvider;
        m_fftProvider = nullptr;
    }
}

void SingleFileWidget::setupConnections() {
    connect(m_spectrogramViewer, &SpectrogramViewer::fileDropped, this, &SingleFileWidget::onFileSelected);
    connect(m_spectrogramViewer, &SpectrogramViewer::seekRequested, this, &SingleFileWidget::seekRequested);
    connect(&m_exportWatcher, &QFutureWatcher<ImageExporter::ExportResult>::finished, this, [this]() {
        handleExportFinished(m_exportWatcher.result());
    });
    connect(this, &SpectrogramWidgetBase::gridVisibilityChanged, m_spectrogramViewer, &SpectrogramViewer::setShowGrid);
    connect(this, &SpectrogramWidgetBase::zoomToggleChanged, m_spectrogramViewer, &SpectrogramViewer::setVerticalZoomEnabled);
    connect(this, &SpectrogramWidgetBase::componentsVisibilityChanged, m_spectrogramViewer, &SpectrogramViewer::setComponentsVisible);
    connect(this, &SpectrogramWidgetBase::curveSettingsChanged, this, [this](CurveType type){
        m_spectrogramViewer->setCurveType(type);
        if (m_fftProvider) {
            m_fftProvider->setCurveType(type);
        }
    });
    connect(this, &SpectrogramWidgetBase::paletteSettingsChanged, this, [this](PaletteType type){
        m_spectrogramViewer->setPaletteType(type);
        if (!m_currentSpectrogram.isNull() && m_currentSpectrogram.format() == QImage::Format_Indexed8) {
            std::vector<QColor> qColors = ColorPalette::getPalette(type);
            if (qColors.size() < 256) qColors = ColorPalette::getPalette(PaletteType::S00);
            QList<QRgb> colorTable;
            colorTable.reserve(256);
            for (const auto& c : qColors) colorTable.append(c.rgb());
            m_currentSpectrogram.setColorTable(colorTable);
            m_spectrogramViewer->setSpectrogramImage(m_currentSpectrogram, false);
            emit logMessageGenerated(getCurrentTimestamp() + " " + tr("配色方案已变更"));
        } else if (hasPcmData()) {
            triggerReprocessing();
        }
    });
    connect(this, &SpectrogramWidgetBase::dbSettingsChanged, this, [this](double min, double max){
        m_spectrogramViewer->setMinDb(min);
        m_spectrogramViewer->setMaxDb(max);
    });
    connect(this, &SpectrogramWidgetBase::parameterChanged, this, &SingleFileWidget::triggerReprocessing);
    connect(this, &SpectrogramWidgetBase::openFileRequested, this, &SingleFileWidget::onOpenFileClicked);
    connect(this, &SpectrogramWidgetBase::saveFileRequested, this, &SingleFileWidget::onSaveClicked);
}

void SingleFileWidget::retranslateBaseUi() {
    SpectrogramWidgetBase::retranslateBaseUi();
    if (m_isCueMode) {
        int displayIndex = (m_currentCueTrackIndex < 0) ? 1 : (m_currentCueTrackIndex + 1);
        m_btnTrack->setText(QString(tr("分轨:%1")).arg(displayIndex, 2, 10, QChar('0')));
    } else {
        if (m_currentTrackIdx == -1) {
            m_btnTrack->setText(tr("音轨:自动"));
        } else {
            m_btnTrack->setText(QString(tr("音轨:%1")).arg(m_currentTrackIdx));
        }
    }
    if (m_currentChannelIdx == -1) {
        m_btnChannel->setText(tr("声道:合并"));
    } else {
        QString chName;
        if (m_currentMetadata.tracks.size() > 0) {
             for(const auto& t : m_currentMetadata.tracks) {
                 if (t.index == m_currentMetadata.currentTrackIndex) {
                     if (m_currentChannelIdx < t.channelNames.size()) 
                         chName = t.channelNames[m_currentChannelIdx];
                     break;
                 }
             }
        }
        if (chName.isEmpty()) chName = QString("Ch%1").arg(m_currentChannelIdx + 1);
        m_btnChannel->setText(tr("声道:") + chName);
    }
}

void SingleFileWidget::onFileSelected(const QString& filePath) {
    if (filePath.isEmpty()) return;
    bool isCue = filePath.endsWith(".cue", Qt::CaseInsensitive);
    if (isCue) {
        if (m_isCueMode && filePath == m_originalCueFilePath && m_isFullCacheAvailable) {
            return;
        }
    }
    else {
        if (!m_isCueMode && filePath == m_currentlyProcessedFile && hasPcmData()) {
            return;
        }
    }
    emit playerProviderReady(nullptr, 0.0);
    emit logMessageGenerated("=======================================================================");
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("启动全量处理"));
    m_currentPcmData = PcmDataVariant();
    m_cachedFftData = SpectrumDataVariant();
    m_fftStateCache.reset();
    if (m_fftProvider) m_fftProvider->setData(nullptr, 0);
    m_currentSpectrogram = QImage();
    m_spectrogramViewer->setSpectrogramImage(QImage(), true);
    m_btnTrack->menu()->clear();
    m_currentCueTrackIndex = -1;
    m_currentCueSheet.reset();
    m_fullCachedPcmData = PcmDataVariant();
    m_isFullCacheAvailable = false;
    m_isLoadingFullForCue = false;
    if (isCue) {
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("解析文件"));
        auto sheetOpt = CueParser::parse(filePath);
        if (!sheetOpt.has_value()) {
            QMessageBox::critical(this, tr("解析失败"), tr("无法解析 CUE 文件，格式可能不正确。"));
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
        m_currentlyProcessedFile = audioPath;
        emit filePathChanged(filePath);
        updateTrackMenuFromCue();
        m_btnTrack->setText(tr("分轨:01"));
        m_btnChannel->setText(tr("声道:合并"));
        m_currentTrackIdx = -1;
        m_currentChannelIdx = -1;
        m_isLoadingFullForCue = true;
        setBaseControlsEnabled(false);
        m_spectrogramViewer->setLoadingState(true);
        m_currentFftSize = getRequiredFftSize(m_currentHeight);
        emit startProcessing(m_currentlyProcessedFile,
            m_isAutoPrecision ? 0.0 : m_currentInterval,
            m_currentHeight,
            m_currentFftSize,
            m_currentCurveType,
            m_currentMinDb,
            m_currentMaxDb,
            m_currentPaletteType,
            m_currentWindowType,
            -1, -1, 0.0, 0.0,
            ProcessMode::DecodeOnly,
            m_currentCueSheet);
        return;
    }
    m_isCueMode = false;
    m_originalCueFilePath.clear();
    m_currentlyProcessedFile = filePath;
    emit filePathChanged(filePath);
    m_currentTrackIdx = -1;
    m_currentChannelIdx = -1;
    m_btnTrack->setText(tr("音轨:自动"));
    m_btnChannel->setText(tr("声道:合并"));
    setBaseControlsEnabled(false);
    m_spectrogramViewer->setLoadingState(true);
    m_currentFftSize = getRequiredFftSize(m_currentHeight);
    emit startProcessing(m_currentlyProcessedFile,
        m_isAutoPrecision ? 0.0 : m_currentInterval,
        m_currentHeight,
        m_currentFftSize,
        m_currentCurveType,
        m_currentMinDb,
        m_currentMaxDb,
        m_currentPaletteType,
        m_currentWindowType,
        -1, -1, 0.0, 0.0,
        ProcessMode::Full,
        std::nullopt);
}

void SingleFileWidget::triggerReprocessing() {
    if (!hasPcmData()) return;
    if (m_isAutoPrecision) updateAutoPrecision();
    setBaseControlsEnabled(false);
    m_spectrogramViewer->setLoadingState(true);
    emit logMessageGenerated("-----------------------------------------------------------------------");
    m_currentFftSize = getRequiredFftSize(m_currentHeight);
    double intervalToSend = m_currentInterval;
    bool canUseCache = false;
    bool hasCachedData = std::visit([](auto&& arg) { return !arg.empty(); }, m_cachedFftData);
    if (hasCachedData && m_fftStateCache.isValid()) {
        bool sizeMatch = (m_fftStateCache.fftSize == m_currentFftSize);
        bool winMatch = (m_fftStateCache.windowType == m_currentWindowType);
        bool intervalMatch = (std::abs(m_fftStateCache.interval - intervalToSend) < 1e-9);
        bool rateMatch = (m_fftStateCache.sampleRate == m_currentMetadata.sampleRate);
        if (sizeMatch && winMatch && intervalMatch && rateMatch) {
            canUseCache = true;
        }
    }
    if (canUseCache) {
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("参数已经变更"));
        emit startReProcessingFromFft(
            m_cachedFftData,
            m_currentMetadata,
            m_currentFftSize,
            m_currentHeight,
            m_currentMetadata.sampleRate,
            m_currentCurveType,
            m_currentMinDb,
            m_currentMaxDb,
            m_currentPaletteType
        );
    } else {
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("参数已经变更"));
        emit startReProcessing(
            m_currentPcmData,
            m_currentMetadata,
            intervalToSend,
            m_currentHeight,
            m_currentFftSize,
            m_currentCurveType,
            m_currentMinDb,
            m_currentMaxDb,
            m_currentPaletteType,
            m_currentWindowType
        );
    }
}

void SingleFileWidget::handleProcessingFinished(
    const QImage& spectrogram,
    const PcmDataVariant& pcmData,
    const SpectrumDataVariant& spectrumData,
    const DecoderTypes::AudioMetadata& metadata)
{
    GlobalPreferences prefs = GlobalPreferences::load();
    if (m_isCueMode && m_isLoadingFullForCue) {
        m_fullCachedPcmData = pcmData;
        m_isFullCacheAvailable = true;
        m_isLoadingFullForCue = false;
        m_currentMetadata = metadata;
        m_currentCueTrackIndex = 0;
        m_cachedFftData = SpectrumDataVariant();
        if (m_currentCueSheet.has_value() && !m_currentCueSheet->tracks.isEmpty()) {
            const auto& track = m_currentCueSheet->tracks[0];
            m_currentPcmData = slicePcmData(m_fullCachedPcmData, m_currentMetadata.sampleRate, track.startSeconds, track.endSeconds);
            double intervalToSend = m_isAutoPrecision ? 0.0 : m_currentInterval;
            emit startReProcessing(
                m_currentPcmData,
                m_currentMetadata,
                intervalToSend,
                m_currentHeight,
                m_currentFftSize,
                m_currentCurveType,
                m_currentMinDb,
                m_currentMaxDb,
                m_currentPaletteType,
                m_currentWindowType
            );
        }
        return;
    }
    m_currentSpectrogram = spectrogram;
    bool hasNewData = std::visit([](auto&& arg) { return !arg.empty(); }, pcmData);
    if (hasNewData) {
        m_currentPcmData = pcmData;
    }
    if (prefs.cacheFftData) {
        m_cachedFftData = spectrumData; 
        if (m_fftProvider) {
            m_fftProvider->setData(&m_cachedFftData, m_currentFftSize);
            m_fftProvider->setSampleRate(metadata.sampleRate);
            m_fftProvider->setDbRange(m_currentMinDb, m_currentMaxDb);
            m_fftProvider->setCurveType(m_currentCurveType);
        }
        size_t rawSize = 0;
        if (std::holds_alternative<SpectrumData32>(spectrumData)) {
            rawSize = std::get<SpectrumData32>(spectrumData).size() * sizeof(float);
        } else {
            rawSize = std::get<SpectrumData64>(spectrumData).size() * sizeof(double);
        }
        emit logMessageGenerated(tr("        [缓存] FFT 数据已保留 (%1)").arg(formatSize(rawSize)));
    } else {
        m_cachedFftData = SpectrumDataVariant();
        m_fftStateCache.reset();
        if (m_fftProvider) {
            m_fftProvider->setData(nullptr, 0);
        }
    }
    if (!m_isCueMode) {
        m_currentMetadata = metadata;
    }
    if (m_isCueMode && m_currentCueSheet.has_value() && m_currentCueTrackIndex >= 0) {
        const auto& track = m_currentCueSheet->tracks[m_currentCueTrackIndex];
        double trackDuration = 0.0;
        if (track.endSeconds > 0.001) {
            trackDuration = track.endSeconds - track.startSeconds;
        }
        else {
            size_t pcmSamples = std::visit([](auto&& arg) { return arg.size(); }, m_currentPcmData);
            if (m_currentMetadata.sampleRate > 0 && pcmSamples > 0) {
                trackDuration = static_cast<double>(pcmSamples) / m_currentMetadata.sampleRate;
            }
        }
        if (trackDuration > 0.0) {
            m_currentMetadata.durationSec = trackDuration;
            m_currentMetadata.preciseDurationMicroseconds = static_cast<long long>(trackDuration * 1000000.0);
        }
    }
    m_currentAudioDuration = m_currentMetadata.durationSec;
    m_preciseDurationStr = formatPreciseDuration(m_currentMetadata.preciseDurationMicroseconds);
    m_currentSampleRate = m_currentMetadata.sampleRate;
    if (m_isAutoPrecision) {
        updateAutoPrecision();
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
            QString label = trk.name;
            QAction* act = trackMenu->addAction(label);
            connect(act, &QAction::triggered, this, [=]() {
                m_btnTrack->setText(QString(tr("音轨:%1")).arg(trk.index));
                onTrackActionTriggered(trk.index);
                });
            if (trk.index == metadata.currentTrackIndex) {
                m_btnTrack->setText(QString(tr("音轨:%1")).arg(trk.index));
            }
        }
    }
    else {
        if (m_currentCueSheet.has_value() && m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
            int displayNum = m_currentCueSheet->tracks[m_currentCueTrackIndex].number;
            m_btnTrack->setText(QString(tr("分轨:%1")).arg(displayNum, 2, 10, QChar('0')));
        }
    }
    QMenu* channelMenu = m_btnChannel->menu();
    channelMenu->clear();
    QAction* actMix = channelMenu->addAction(tr("合并所有声道"));
    connect(actMix, &QAction::triggered, this, [=]() {
        m_btnChannel->setText(tr("声道:合并"));
        onChannelActionTriggered(-1);
        });
    const DecoderTypes::AudioTrack* currentTrackPtr = nullptr;
    for (const auto& trk : m_currentMetadata.tracks) {
        if (trk.index == m_currentMetadata.currentTrackIndex) {
            currentTrackPtr = &trk;
            break;
        }
    }
    QString currentChannelLabel = tr("声道:合并");
    if (currentTrackPtr && currentTrackPtr->channels > 0) {
        for (int i = 0; i < currentTrackPtr->channels; ++i) {
            QString chName = (i < currentTrackPtr->channelNames.size())
                ? currentTrackPtr->channelNames[i]
                : QString("Ch%1").arg(i + 1);
                QAction* act = channelMenu->addAction(chName);
                connect(act, &QAction::triggered, this, [=]() {
                    m_btnChannel->setText(tr("声道:") + chName);
                    onChannelActionTriggered(i);
                    });
                if (i == m_currentChannelIdx) {
                    currentChannelLabel = tr("声道:") + chName;
                }
        }
    }
    m_btnChannel->setText(currentChannelLabel);
    setBaseControlsEnabled(true);
    m_spectrogramViewer->setLoadingState(false);
    m_spectrogramViewer->setSpectrogramImage(spectrogram);
    m_spectrogramViewer->setExternalDataProvider(m_fftProvider);
    m_spectrogramViewer->setDataConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
    m_spectrogramViewer->setAudioProperties(m_currentAudioDuration, m_preciseDurationStr, m_currentSampleRate);
    QSharedPointer<IPlayerProvider> provider;
    bool useFfmpegProvider = false;
    if (m_currentChannelIdx == -1) {
        useFfmpegProvider = true;
    }
    if (useFfmpegProvider) {
        double start = 0.0;
        double end = 0.0;
        if (m_isCueMode && m_currentCueSheet.has_value()) {
            if (m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < m_currentCueSheet->tracks.size()) {
                const auto& t = m_currentCueSheet->tracks[m_currentCueTrackIndex];
                start = t.startSeconds;
                end = t.endSeconds;
            }
        }
        auto ffmpegProvider = new PlayerFfmpegProvider(
            m_currentlyProcessedFile, 
            m_currentTrackIdx, 
            -1, 
            start, 
            end,
            128 * 1024 * 1024
        );
        if (ffmpegProvider->isValid()) {
            provider = QSharedPointer<IPlayerProvider>(ffmpegProvider);
        }
        else {
            delete ffmpegProvider;
            provider = QSharedPointer<IPlayerProvider>(new PlayerRamProvider(m_currentPcmData, m_currentSampleRate));
        }
    }
    else {
        provider = QSharedPointer<IPlayerProvider>(new PlayerRamProvider(m_currentPcmData, m_currentSampleRate));
    }
    if (prefs.cacheFftData) {
        m_fftStateCache.fftSize = m_currentFftSize;
        m_fftStateCache.windowType = m_currentWindowType;
        m_fftStateCache.interval = m_currentInterval;
        m_fftStateCache.sampleRate = m_currentSampleRate;
    }
    emit playerProviderReady(provider, m_currentAudioDuration);
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("图像已经显示"));
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("播放器已就绪"));
}

void SingleFileWidget::onTrackActionTriggered(int index) {
    if (m_isCueMode) {
        if (!m_currentCueSheet.has_value()) return;
        const auto& tracks = m_currentCueSheet->tracks;
        if (index < 0 || index >= tracks.size()) return;
        if (index == m_currentCueTrackIndex && hasPcmData()) return;
        m_currentCueTrackIndex = index;
        const auto& track = tracks[index];
        emit logMessageGenerated("-----------------------------------------------------------------------");
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("切换分轨 %1：%2 - %3")
            .arg(track.number, 2, 10, QChar('0'))
            .arg(track.performer)
            .arg(track.title));
        setBaseControlsEnabled(false);
        m_spectrogramViewer->setLoadingState(true);
        m_currentPcmData = PcmDataVariant();
        double intervalToSend = m_isAutoPrecision ? 0.0 : m_currentInterval;
        m_currentFftSize = getRequiredFftSize(m_currentHeight);
        if (m_isFullCacheAvailable && hasPcmData() == false) {
            emit logMessageGenerated(tr("        读取PCM缓存"));
            m_currentPcmData = slicePcmData(m_fullCachedPcmData, m_currentMetadata.sampleRate, track.startSeconds, track.endSeconds);
            emit startReProcessing(
                m_currentPcmData,
                m_currentMetadata,
                intervalToSend,
                m_currentHeight,
                m_currentFftSize,
                m_currentCurveType,
                m_currentMinDb,
                m_currentMaxDb,
                m_currentPaletteType,
                m_currentWindowType
            );
        }
        else {
            emit startProcessing(m_currentlyProcessedFile,
                intervalToSend, m_currentHeight, m_currentFftSize,
                m_currentCurveType, m_currentMinDb, m_currentMaxDb,
                m_currentPaletteType, m_currentWindowType,
                -1, -1,
                track.startSeconds, track.endSeconds,
                ProcessMode::Full);
        }
        return;
    }
    if (index == m_currentTrackIdx) return;
    int targetPhysicalIndex = (index == -1) ? m_currentMetadata.defaultTrackIndex : index;
    int currentPhysicalIndex = m_currentMetadata.currentTrackIndex;
    if (targetPhysicalIndex == currentPhysicalIndex && hasPcmData()) {
        m_currentTrackIdx = index;
        retranslateBaseUi();
        return;
    }
    m_currentTrackIdx = index;
    m_currentChannelIdx = -1;
    if (!m_currentlyProcessedFile.isEmpty()) {
        emit logMessageGenerated("-----------------------------------------------------------------------");
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("切换音轨"));
        setBaseControlsEnabled(false);
        m_spectrogramViewer->setLoadingState(true);
        emit startProcessing(m_currentlyProcessedFile,
            m_isAutoPrecision ? 0.0 : m_currentInterval,
            m_currentHeight, m_currentFftSize,
            m_currentCurveType, m_currentMinDb, m_currentMaxDb,
            m_currentPaletteType, m_currentWindowType,
            m_currentTrackIdx, m_currentChannelIdx, 0.0, 0.0,
            ProcessMode::Full);
    }
}

void SingleFileWidget::onChannelActionTriggered(int channelIdx) {
    if (channelIdx == m_currentChannelIdx) return;
    m_currentChannelIdx = channelIdx;
    emit logMessageGenerated("-----------------------------------------------------------------------");
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("切换声道"));
    if (m_isCueMode && m_currentCueSheet.has_value()) {
        const auto& tracks = m_currentCueSheet->tracks;
        if (m_currentCueTrackIndex >= 0 && m_currentCueTrackIndex < tracks.size()) {
            const auto& track = tracks[m_currentCueTrackIndex];
            setBaseControlsEnabled(false);
            m_spectrogramViewer->setLoadingState(true);
            emit startProcessing(m_currentlyProcessedFile,
                m_currentInterval, m_currentHeight, m_currentFftSize,
                m_currentCurveType, m_currentMinDb, m_currentMaxDb,
                m_currentPaletteType, m_currentWindowType,
                -1, m_currentChannelIdx,
                track.startSeconds, track.endSeconds,
                ProcessMode::Full);
            return;
        }
    }
    if (!m_currentlyProcessedFile.isEmpty()) {
        setBaseControlsEnabled(false);
        m_spectrogramViewer->setLoadingState(true);
        emit startProcessing(m_currentlyProcessedFile,
            m_currentInterval, m_currentHeight, m_currentFftSize,
            m_currentCurveType, m_currentMinDb, m_currentMaxDb,
            m_currentPaletteType, m_currentWindowType,
            m_currentTrackIdx, m_currentChannelIdx, 0.0, 0.0,
            ProcessMode::Full);
    }
}

void SingleFileWidget::onOpenFileClicked() {
    const QString fileFilter = tr("所有文件 (*.*)");
    QString filePath = QFileDialog::getOpenFileName(this, tr("打开音频文件"), m_lastOpenPath, fileFilter);
    if (!filePath.isEmpty()) {
        m_lastOpenPath = QFileInfo(filePath).path();
        onFileSelected(filePath);
    }
}

void SingleFileWidget::onSaveClicked() {
    if (m_currentSpectrogram.isNull()) {
        QMessageBox::warning(this, tr("错误"), tr("没有可以保存的频谱图。"));
        return;
    }
    QImage imageToSave = m_currentSpectrogram;
    if (m_currentHeight != imageToSave.height()) {
        imageToSave = imageToSave.scaled(
            imageToSave.width(),
            m_currentHeight,
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation
        );
    }
    if (m_widthLimitEnabled && imageToSave.width() > m_widthLimitValue) {
        imageToSave = imageToSave.scaled(
            m_widthLimitValue,
            imageToSave.height(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation
        );
    }
    bool drawComponents = m_btnComponents->isChecked();
    int finalW = imageToSave.width();
    int finalH = imageToSave.height();
    if (drawComponents) {
        finalW += AppConfig::PNG_SAVE_LEFT_MARGIN + AppConfig::PNG_SAVE_RIGHT_MARGIN;
        finalH += AppConfig::PNG_SAVE_TOP_MARGIN + AppConfig::PNG_SAVE_BOTTOM_MARGIN;
    }
    bool jpgAvail = (finalW <= AppConfig::JPEG_MAX_DIMENSION && finalH <= AppConfig::JPEG_MAX_DIMENSION);
    bool webpAvail = (finalW <= AppConfig::MY_WEBP_MAX_DIMENSION && finalH <= AppConfig::MY_WEBP_MAX_DIMENSION);
    bool avifAvail = (finalW <= AppConfig::MY_AVIF_MAX_DIMENSION && finalH <= AppConfig::MY_AVIF_MAX_DIMENSION);
    QString defaultBaseName;
    QFileInfo fileInfo(m_currentlyProcessedFile);
    QString sourceExtension = fileInfo.suffix();
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
            defaultBaseName = fileInfo.completeBaseName(); 
        }
        if (!sourceExtension.isEmpty()) {
            defaultBaseName += "." + sourceExtension;
        }
        defaultBaseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    } 
    else {
        defaultBaseName = fileInfo.fileName();
    }
    SingleFileExportDialog dialog(m_lastSavePath, defaultBaseName, jpgAvail, webpAvail, avifAvail, this);
    if (dialog.exec() != QDialog::Accepted) return;
    QString outputFilePath = dialog.getSelectedFilePath();
    m_lastSavePath = QFileInfo(outputFilePath).path();
    if (QFile::exists(outputFilePath)) {
        QMessageBox msgBox(QMessageBox::Question, tr("覆盖"), tr("文件已存在，是否覆盖？"), QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setButtonText(QMessageBox::Yes, tr("是"));
        msgBox.setButtonText(QMessageBox::No, tr("否"));
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() != QMessageBox::Yes) {
            return;
        }
    }
    setBaseControlsEnabled(false);
    if (m_btnSave) m_btnSave->setText(tr("保存中"));
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("正在保存图像"));
    QString fName = defaultBaseName;  // fileInfo.fileName()
    bool showGrid = m_btnGrid->isChecked();
    QString fmt = dialog.getSelectedFormatIdentifier();
    int quality = dialog.qualityLevel();
    auto future = QtConcurrent::run([=]() {
        return ImageExporter::exportImage(
            imageToSave, fName, m_currentAudioDuration, showGrid, m_preciseDurationStr, m_currentSampleRate,
            quality, outputFilePath, fmt, m_currentCurveType, m_currentMinDb, m_currentMaxDb, m_currentPaletteType,
            drawComponents
        );
        });
    m_exportWatcher.setFuture(future);
}

void SingleFileWidget::handleProcessingFailed(const QString& errorMessage) {
    setBaseControlsEnabled(true);
    m_spectrogramViewer->setLoadingState(false);
    emit logMessageGenerated(getCurrentTimestamp() + " " + tr("处理失败：") + errorMessage);
    QMessageBox::critical(this, tr("失败"), errorMessage);
}

void SingleFileWidget::handleExportFinished(const ImageExporter::ExportResult& result) {
    setBaseControlsEnabled(true);
    if (m_btnSave) m_btnSave->setText(tr("保存"));
    if (result.success) {
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("保存成功：%1").arg(result.outputFilePath));
        QMessageBox msgBox(QMessageBox::Information, tr("成功"), tr("图像保存成功"), QMessageBox::Ok, this);
        msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
        msgBox.exec();
    }
    else {
        emit logMessageGenerated(getCurrentTimestamp() + " " + tr("保存失败：%1").arg(result.message));
        QMessageBox msgBox(QMessageBox::Critical, tr("失败"), result.message, QMessageBox::Ok, this);
        msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
        msgBox.exec();
    }
}

void SingleFileWidget::setupWorkerThread() {
    m_processor = new SpectrogramProcessor;
    m_processor->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_processor, &QObject::deleteLater);
    connect(this, &SingleFileWidget::startProcessing, m_processor, &SpectrogramProcessor::processFile);
    connect(this, &SingleFileWidget::startReProcessing, m_processor, &SpectrogramProcessor::reProcessFromPcm);
    connect(this, &SingleFileWidget::startReProcessingFromFft, m_processor, &SpectrogramProcessor::reProcessFromFft);
    connect(m_processor, &SpectrogramProcessor::logMessage, this, &SingleFileWidget::logMessageGenerated, Qt::QueuedConnection);
    connect(m_processor, &SpectrogramProcessor::processingFinished, this, &SingleFileWidget::handleProcessingFinished, Qt::QueuedConnection);
    connect(m_processor, &SpectrogramProcessor::processingFailed, this, &SingleFileWidget::handleProcessingFailed, Qt::QueuedConnection);
    m_workerThread.start();
}

void SingleFileWidget::updateAutoPrecision() {
    if (!m_isAutoPrecision) return;
    if (m_currentSampleRate <= 0) {
        m_btnInterval->setText(tr("精度:自动"));
        m_currentInterval = CoreConfig::DEFAULT_TIME_INTERVAL_S;
        return;
    }
    int fftSize = getRequiredFftSize(m_currentHeight);
    double val = static_cast<double>(fftSize) / static_cast<double>(m_currentSampleRate);
    m_btnInterval->setText(QString(tr("精度:%1")).arg(val, 0, 'f', 3));
    m_currentInterval = val;
}

bool SingleFileWidget::hasPcmData() const {
    return std::visit([](auto&& arg) { return !arg.empty(); }, m_currentPcmData);
}

void SingleFileWidget::setPlayheadPosition(double seconds) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setPlayheadPosition(seconds);
    }
}

void SingleFileWidget::setPlayheadVisible(bool visible) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setPlayheadVisible(visible);
    }
}

QString SingleFileWidget::findAudioFileForCue(const QString& cuePath, const QString& cueAudioTarget) {
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
        return fileList.last().absoluteFilePath();
    }
    return QString();
}

void SingleFileWidget::updateTrackMenuFromCue() {
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
            m_btnTrack->setText(QString(tr("分轨:%1")).arg(t.number, 2, 10, QChar('0')));
            onTrackActionTriggered(i);
        });
    }
}

PcmDataVariant SingleFileWidget::slicePcmData(const PcmDataVariant& source, int sampleRate, double startSec, double endSec) {
    if (sampleRate <= 0) return PcmData32();
    size_t startSample = static_cast<size_t>(startSec * sampleRate);
    auto worker = [&](auto&& srcVec) -> PcmDataVariant {
        using T = typename std::decay_t<decltype(srcVec)>::value_type;
        using ResultVec = std::vector<T, AlignedAllocator<T, 64>>;
        if (srcVec.empty() || startSample >= srcVec.size()) {
            return ResultVec();
        }
        size_t endSample = srcVec.size();
        if (endSec > 0.001) {
            endSample = static_cast<size_t>(endSec * sampleRate);
        }
        if (endSample > srcVec.size()) endSample = srcVec.size();
        if (endSample <= startSample) return ResultVec();
        size_t count = endSample - startSample;
        ResultVec destVec;
        try {
            destVec.reserve(count);
            destVec.insert(destVec.end(), srcVec.begin() + startSample, srcVec.begin() + startSample + count);
            size_t bytes = count * sizeof(T);
            size_t alignedBytes = (bytes + 63) & ~63;
            if (alignedBytes > bytes) {
                size_t alignedCount = alignedBytes / sizeof(T);
                destVec.resize(alignedCount, 0);
            }
        } catch (...) {
            return ResultVec();
        }
        return destVec;
    };
    if (std::holds_alternative<PcmData64>(source)) {
        return worker(std::get<PcmData64>(source));
    } else {
        return worker(std::get<PcmData32>(source));
    }
}

void SingleFileWidget::updateCrosshairStyle(const CrosshairStyle& style, bool enabled) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setCrosshairStyle(style);
        m_spectrogramViewer->setCrosshairEnabled(enabled);
    }
}

void SingleFileWidget::updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setSpectrumProfileStyle(visible, color, lineWidth, filled, alpha, type);
    }
}

void SingleFileWidget::updatePlayheadStyle(const PlayheadStyle& style) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setPlayheadStyle(style);
    }
}

void SingleFileWidget::setProfileFrameRate(int fps) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setProfileFrameRate(fps);
    }
}

void SingleFileWidget::updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setDataConfig(spectrumSrc, probeSrc, precision);
    }
}

void SingleFileWidget::setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) {
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setIndicatorVisibility(showFreq, showTime, showDb);
    }
}

void SingleFileWidget::applyGlobalPreferences(const GlobalPreferences& prefs, bool silent) {
    SpectrogramWidgetBase::applyGlobalPreferences(prefs, silent);
    if (m_spectrogramViewer) {
        m_spectrogramViewer->setShowGrid(prefs.showGrid);
        m_spectrogramViewer->setComponentsVisible(prefs.showComponents);
        m_spectrogramViewer->setVerticalZoomEnabled(prefs.enableZoom);
        m_spectrogramViewer->setPaletteType(prefs.paletteType);
        m_spectrogramViewer->setMinDb(prefs.minDb);
        m_spectrogramViewer->setMaxDb(prefs.maxDb);
        m_spectrogramViewer->setCurveType(prefs.curveType);
        m_spectrogramViewer->setDataConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        m_spectrogramViewer->setGpuEnabled(prefs.enableGpuAcceleration);
    }
    if (m_fftProvider) {
        m_fftProvider->setCurveType(prefs.curveType);
    }
    if (!prefs.cacheFftData) {
        bool hasData = std::visit([](auto&& arg) { return !arg.empty(); }, m_cachedFftData);
        if (hasData) {
            m_cachedFftData = SpectrumDataVariant();
            if (m_fftProvider) m_fftProvider->setData(nullptr, 0);
            emit logMessageGenerated(getCurrentTimestamp() + " " + tr("缓存已根据设置清除"));
        }
    }
}