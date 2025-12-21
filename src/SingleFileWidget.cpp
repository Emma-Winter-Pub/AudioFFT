#include "SingleFileWidget.h"
#include "SpectrogramViewer.h"
#include "SpectrogramProcessor.h"
#include "ImageExporter.h" 
#include "ExportOptionsDialog.h"
#include "AppConfig.h"
#include "Utils.h" 

#include <vector>
#include <algorithm>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QFuture>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtConcurrent>
#include <QFutureWatcher>


SingleFileWidget::SingleFileWidget(QWidget* parent)
    : QWidget(parent)
    , m_lastSavePath()
    , m_lastOpenPath()
    , m_currentCurveType(CurveType::Function0)
{
    setupUI();
    setupWorkerThread();
    connect(m_spectrogramViewer, &SpectrogramViewer::fileDropped, this, &SingleFileWidget::onFileSelected);

    connect(&m_exportWatcher, &QFutureWatcher<ImageExporter::ExportResult>::finished, this, [this]() {
        handleExportFinished(m_exportWatcher.result());
        });
}


SingleFileWidget::~SingleFileWidget() {
    m_workerThread.quit();
    m_workerThread.wait();
}


void SingleFileWidget::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 5, 0, 0); 
    mainLayout->setSpacing(5);
    
    auto optionsPanelLayout = new QHBoxLayout();
    optionsPanelLayout->setContentsMargins(5, 0, 5, 0);
    optionsPanelLayout->setSpacing(7); 

    const QString checkBoxStyle = "QCheckBox { spacing: 2px; color: rgb(217, 217, 217); }";
    const QString labelStyle = "QLabel { color: rgb(217, 217, 217); }";
    const QString spinBoxStyle = "QSpinBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 1px; }";
    const QString comboBoxStyle = 
        "QComboBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 3px; }"
        "QComboBox QAbstractItemView { background-color: rgb(78, 90, 110); selection-background-color: #5A687A; color: rgb(217, 217, 217); border: 1px solid #2B333E; }";
    const QString buttonStyle =
        "QPushButton {"
        "    background-color: rgb(78, 90, 110);"
        "    color: rgb(217, 217, 217);"
        "    border: 1px solid #1C222B;"
        "    padding: 2px;"
        "}"
        "QPushButton:hover { background-color: rgb(55, 65, 81); }"
        "QPushButton:pressed { background-color: rgb(30, 36, 45); }"
        "QPushButton:disabled { background-color: rgb(50, 58, 70); color: #888888; }";

    m_showLogCheckBox = new QCheckBox(tr("Log"));
    m_showLogCheckBox->setStyleSheet(checkBoxStyle);
    optionsPanelLayout->addWidget(m_showLogCheckBox);

    m_showGridCheckBox = new QCheckBox(tr("Grid"));
    m_showGridCheckBox->setStyleSheet(checkBoxStyle);
    m_showGridCheckBox->setChecked(false);
    optionsPanelLayout->addWidget(m_showGridCheckBox);
    
    m_enableVerticalZoomCheckBox = new QCheckBox(tr("Zoom Hz"));
    m_enableVerticalZoomCheckBox->setStyleSheet(checkBoxStyle);
    m_enableVerticalZoomCheckBox->setChecked(false);
    optionsPanelLayout->addWidget(m_enableVerticalZoomCheckBox);

    m_enableWidthLimitCheckBox = new QCheckBox(tr("Limit Width"));
    m_enableWidthLimitCheckBox->setStyleSheet(checkBoxStyle);
    m_enableWidthLimitCheckBox->setChecked(false);
    optionsPanelLayout->addWidget(m_enableWidthLimitCheckBox);
    optionsPanelLayout->addSpacing(-5);

    m_maxWidthSpinBox = new QSpinBox();
    m_maxWidthSpinBox->setStyleSheet(spinBoxStyle);
    m_maxWidthSpinBox->setRange(AppConfig::PNG_SAVE_MIN_MAX_WIDTH, AppConfig::PNG_SAVE_MAX_MAX_WIDTH);
    m_maxWidthSpinBox->setValue(AppConfig::PNG_SAVE_DEFAULT_MAX_WIDTH);
    m_maxWidthSpinBox->setFixedWidth(60);
    optionsPanelLayout->addWidget(m_maxWidthSpinBox);
    connect(m_enableWidthLimitCheckBox, &QCheckBox::toggled, m_maxWidthSpinBox, &QWidget::setEnabled);
    
    auto heightLabel = new QLabel(tr("Height"));
    heightLabel->setStyleSheet(labelStyle);
    optionsPanelLayout->addWidget(heightLabel);
    optionsPanelLayout->addSpacing(-5);

    m_imageHeightComboBox = new QComboBox();
    m_imageHeightComboBox->setStyleSheet(comboBoxStyle);
    m_imageHeightComboBox->setFixedWidth(55);
    QStringList heightIntervals = { "8193", "4097", "2049", "1025", "513", "257", "129", "8000", "7500", "7000", "6500", "6000", "5500", "5000", "4500", "4000", "3500", "3000", "2900", "2800", "2700", "2600", "2500", "2400", "2300", "2200", "2100", "2000", "1900", "1800", "1700", "1600", "1500", "1400", "1300", "1200", "1100", "1000", "900", "800", "700", "600", "500",  "400",  "300", "200", "100" };
    m_imageHeightComboBox->addItems(heightIntervals);
    m_imageHeightComboBox->setCurrentText(QString::number(AppConfig::DEFAULT_SPECTROGRAM_HEIGHT));
    optionsPanelLayout->addWidget(m_imageHeightComboBox);
    connect(m_imageHeightComboBox, &QComboBox::currentIndexChanged, this, &SingleFileWidget::onHeightParamsChanged);

    auto intervalLabel = new QLabel(tr("Time Precision"));
    intervalLabel->setStyleSheet(labelStyle);
    optionsPanelLayout->addWidget(intervalLabel);
    optionsPanelLayout->addSpacing(-5);

    m_timeIntervalComboBox = new QComboBox();
    m_timeIntervalComboBox->setStyleSheet(comboBoxStyle);
    m_timeIntervalComboBox->setFixedWidth(60);
    QStringList intervals = { "10", "5", "2.5", "2", "1", "0.5", "0.25", "0.2", "0.1", "0.05", "0.025", "0.02", "0.01" };
    m_timeIntervalComboBox->addItems(intervals);
    m_timeIntervalComboBox->setCurrentText(QString::number(CoreConfig::DEFAULT_TIME_INTERVAL_S));
    optionsPanelLayout->addWidget(m_timeIntervalComboBox);
    connect(m_timeIntervalComboBox, &QComboBox::currentIndexChanged, this, &SingleFileWidget::onTimeIntervalChanged);
    
    optionsPanelLayout->addSpacing(40);

    m_openButton = new QPushButton(tr("Open"));
    m_openButton->setStyleSheet(buttonStyle);
    m_openButton->setFixedWidth(42);
    connect(m_openButton, &QPushButton::clicked, this, &SingleFileWidget::onOpenFileClicked);
    optionsPanelLayout->addWidget(m_openButton);

    m_saveButton = new QPushButton(tr("Save"));
    m_saveButton->setStyleSheet(buttonStyle);
    m_saveButton->setFixedWidth(42); 
    m_saveButton->setEnabled(false);
    connect(m_saveButton, &QPushButton::clicked, this, &SingleFileWidget::saveSpectrogram);
    optionsPanelLayout->addWidget(m_saveButton);
    
    optionsPanelLayout->addStretch();
    
    mainLayout->addLayout(optionsPanelLayout);

    m_spectrogramViewer = new SpectrogramViewer();
    mainLayout->addWidget(m_spectrogramViewer, 1);

    connect(m_showGridCheckBox, &QCheckBox::toggled, m_spectrogramViewer, &SpectrogramViewer::setShowGrid);
    connect(m_enableVerticalZoomCheckBox, &QCheckBox::toggled, m_spectrogramViewer, &SpectrogramViewer::setVerticalZoomEnabled);

    m_spectrogramViewer->setShowGrid(m_showGridCheckBox->isChecked());
    m_spectrogramViewer->setVerticalZoomEnabled(m_enableVerticalZoomCheckBox->isChecked());
}


void SingleFileWidget::setCurveType(CurveType type)
{
    if (m_currentCurveType == type) {
        return;}

    m_currentCurveType = type;

    m_spectrogramViewer->setCurveType(m_currentCurveType);

    if (!m_currentPcmData.empty()) {
        emit logMessageGenerated(QString(58, '-'));
        emit logMessageGenerated(getCurrentTimestamp() + tr(" Mapping function changed. Reprocessing..."));

        setControlsEnabled(false);
        m_spectrogramViewer->setLoadingState(true);

        int currentHeight = m_imageHeightComboBox->currentText().toInt();
        int requiredFftSize = getRequiredFftSize(currentHeight);

        emit startReProcessing(
            m_currentPcmData,
            m_currentAudioDuration,
            m_preciseDurationStr,
            m_currentSampleRate,
            m_timeIntervalComboBox->currentText().toDouble(),
            currentHeight,
            requiredFftSize,
            m_currentCurveType);
    }
}


void SingleFileWidget::onOpenFileClicked() {
    const QString fileFilter = tr("All Files (*.*)");
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Audio File"), m_lastOpenPath, fileFilter);

    if (!filePath.isEmpty()) {
        m_lastOpenPath = QFileInfo(filePath).path();
        onFileSelected(filePath);}
}


void SingleFileWidget::setupWorkerThread() {
    m_processor = new SpectrogramProcessor;
    m_processor->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, m_processor, &QObject::deleteLater);

    connect(this, &SingleFileWidget::startProcessing, m_processor, &SpectrogramProcessor::processFile);
    connect(this, &SingleFileWidget::startReProcessing, m_processor, &SpectrogramProcessor::reProcessFromPcm);

    connect(m_processor, &SpectrogramProcessor::logMessage, this, &SingleFileWidget::logMessageGenerated, Qt::QueuedConnection);
    connect(m_processor, &SpectrogramProcessor::processingFinished, this, &SingleFileWidget::handleProcessingFinished, Qt::QueuedConnection);
    connect(m_processor, &SpectrogramProcessor::processingFailed, this, &SingleFileWidget::handleProcessingFailed, Qt::QueuedConnection);
    m_workerThread.start();
}


int SingleFileWidget::getRequiredFftSize(int height) const {
    const auto& fftOptions = AppConfig::FFT_SIZE_OPTIONS;
    for (int fftSize : fftOptions) {
        if (height <= (fftSize / 2 + 1)) {
            return fftSize;}}
    return fftOptions.back();
}


void SingleFileWidget::onFileSelected(const QString& filePath) {
    if (filePath != m_currentlyProcessedFile) {
        if (m_currentlyProcessedFile.isEmpty()) {
            emit logMessageGenerated("");}
        else { emit logMessageGenerated("=======================================================================");}
        m_currentlyProcessedFile = filePath;}

    emit logMessageGenerated(getCurrentTimestamp() + tr(" Starting processing workflow"));

    emit filePathChanged(filePath);

    m_currentPcmData.clear();
    onProcessingParamsChanged();
}


void SingleFileWidget::onProcessingParamsChanged() {
    if (m_currentlyProcessedFile.isEmpty() || !QFile::exists(m_currentlyProcessedFile)) {
        return;}

    setControlsEnabled(false);
    m_spectrogramViewer->setLoadingState(true);
    m_spectrogramViewer->setSpectrogramImage(QImage());

    int imageHeight = m_imageHeightComboBox->currentText().toInt();
    int fftSize = getRequiredFftSize(imageHeight);

    emit startProcessing(m_currentlyProcessedFile,
        m_timeIntervalComboBox->currentText().toDouble(),
        imageHeight,
        fftSize,
        m_currentCurveType);
}


void SingleFileWidget::onHeightParamsChanged() {
    if (m_currentSpectrogram.isNull() || m_currentFftSize == 0) {
        return;}

    int newHeight = m_imageHeightComboBox->currentText().toInt();
    int newRequiredFftSize = getRequiredFftSize(newHeight);

    if (newRequiredFftSize > m_currentFftSize) {
        if (!m_currentPcmData.empty()) {
            setControlsEnabled(false);
            m_spectrogramViewer->setLoadingState(true);

            emit logMessageGenerated(QString(58, '-'));

            emit startReProcessing(
                m_currentPcmData,
                m_currentAudioDuration,
                m_preciseDurationStr,
                m_currentSampleRate,
                m_timeIntervalComboBox->currentText().toDouble(),
                newHeight,
                newRequiredFftSize,
                m_currentCurveType);}
        else {
            onProcessingParamsChanged();}
    }
}


void SingleFileWidget::onTimeIntervalChanged() {
    if (m_currentSpectrogram.isNull() || m_currentTimeInterval == 0.0) {
        return;}

    double newInterval = m_timeIntervalComboBox->currentText().toDouble();

    if (newInterval < m_currentTimeInterval) {
        if (!m_currentPcmData.empty()) {
            setControlsEnabled(false);
            m_spectrogramViewer->setLoadingState(true);
            int currentHeight = m_imageHeightComboBox->currentText().toInt();
            int requiredFftSize = getRequiredFftSize(currentHeight);

            emit logMessageGenerated(QString(58, '-'));

            emit startReProcessing(
                m_currentPcmData,
                m_currentAudioDuration,
                m_preciseDurationStr,
                m_currentSampleRate,
                newInterval,
                currentHeight,
                requiredFftSize,
                m_currentCurveType);}
        else {
            onProcessingParamsChanged();}
    }
}


void SingleFileWidget::setShowGrid(bool show){m_spectrogramViewer->setShowGrid(show);}


void SingleFileWidget::setVerticalZoomEnabled(bool enabled){m_spectrogramViewer->setVerticalZoomEnabled(enabled);}


void SingleFileWidget::saveSpectrogram(){
    if (m_currentSpectrogram.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("No spectrogram available to save."));
        return;}

    QImage imageToSave = m_currentSpectrogram;
    const int targetHeight = m_imageHeightComboBox->currentText().toInt();
    if (targetHeight != imageToSave.height()) {
        imageToSave = imageToSave.scaled(imageToSave.width(), targetHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}
    const double targetInterval = m_timeIntervalComboBox->currentText().toDouble();
    if (targetInterval > m_currentTimeInterval && m_currentTimeInterval > 0.0) {
        int newWidth = static_cast<int>(imageToSave.width() * (m_currentTimeInterval / targetInterval));
        newWidth = std::max(1, newWidth);
        imageToSave = imageToSave.scaled(newWidth, imageToSave.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}
    if (m_enableWidthLimitCheckBox->isChecked() && imageToSave.width() > m_maxWidthSpinBox->value()) {
        imageToSave = imageToSave.scaled(m_maxWidthSpinBox->value(), imageToSave.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}

    const int finalExportWidth = imageToSave.width() + AppConfig::PNG_SAVE_LEFT_MARGIN + AppConfig::PNG_SAVE_RIGHT_MARGIN;
    const int finalExportHeight = imageToSave.height() + AppConfig::PNG_SAVE_TOP_MARGIN + AppConfig::PNG_SAVE_BOTTOM_MARGIN;

    bool isJpegAvailable = (finalExportWidth <= AppConfig::JPEG_MAX_DIMENSION && finalExportHeight <= AppConfig::JPEG_MAX_DIMENSION);
    bool isWebpAvailable = (finalExportWidth <= AppConfig::MY_WEBP_MAX_DIMENSION && finalExportHeight <= AppConfig::MY_WEBP_MAX_DIMENSION);
    bool isAvifAvailable = (finalExportWidth <= AppConfig::MY_AVIF_MAX_DIMENSION && finalExportHeight <= AppConfig::MY_AVIF_MAX_DIMENSION);

    QFileInfo fileInfo(m_currentlyProcessedFile);
    ExportOptionsDialog dialog(m_lastSavePath, fileInfo.fileName(), isJpegAvailable, isWebpAvailable, isAvifAvailable, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;}

    QString outputFilePath = dialog.getSelectedFilePath();
    int qualityLevel = dialog.qualityLevel();
    QString formatIdentifier = dialog.getSelectedFormatIdentifier();
    m_lastSavePath = QFileInfo(outputFilePath).path();

    if (QFile::exists(outputFilePath)) {
        QMessageBox msgBox(QMessageBox::Question,
                             tr("Confirm Overwrite"),
                             QString(tr("%1 already exists.\n\nOverwrite?"))
                                     .arg(QFileInfo(outputFilePath).fileName()),
                             QMessageBox::NoButton,
                             this);
        QPushButton *okButton = msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
        QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelButton);
        msgBox.exec();
        if (msgBox.clickedButton() == cancelButton) {
            return;}}

    bool shouldProceedToSave = true;

    const qint64 totalPixels = (qint64)finalExportWidth * finalExportHeight;
    const qint64 pixelThreshold = 24000000;
    if (formatIdentifier == "PNG" && qualityLevel > 0 && totalPixels > pixelThreshold)
    {
        const double pixelsInMyriad = totalPixels / 1000000.0;
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Warning"));
        msgBox.setText(QString(tr("The image contains %1 million pixels, Saving will take time.\n\n"
            "Continue?"))
            .arg(pixelsInMyriad, 0, 'f', 0));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("Yes"));
        msgBox.button(QMessageBox::No)->setText(tr("No"));
        msgBox.setDefaultButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No) {
            shouldProceedToSave = false;}
    }

    if (shouldProceedToSave) {

        const QImage finalImageToSave = imageToSave;
        const QString fName = fileInfo.fileName();
        const double duration = m_currentAudioDuration;
        const bool grid = m_showGridCheckBox->isChecked();
        const QString pDuration = m_preciseDurationStr;
        const int sRate = m_currentSampleRate;
        const int qLevel = qualityLevel;
        const QString oPath = outputFilePath;
        const QString fIdentifier = formatIdentifier;
        const CurveType cType = m_currentCurveType;

        auto future = QtConcurrent::run([=]() {
            return ImageExporter::exportImage(
                finalImageToSave,
                fName,
                duration,
                grid,
                pDuration,
                sRate,
                qLevel,
                oPath,
                fIdentifier,
                cType
            );
            });

        m_exportWatcher.setFuture(future);

        setControlsEnabled(false);
        m_saveButton->setText(tr("Saving"));
        emit logMessageGenerated(getCurrentTimestamp() + tr(" Saving image..."));
    }
}


void SingleFileWidget::handleProcessingFinished(const QImage& spectrogram, const std::vector<float>& pcmData, double durationSec, const QString& preciseDurationStr, int nativeSampleRate) {
    m_currentSpectrogram = spectrogram;
    m_currentAudioDuration = durationSec;
    m_preciseDurationStr = preciseDurationStr;
    m_currentSampleRate = nativeSampleRate;
    
    if (!pcmData.empty()) {
        m_currentPcmData = pcmData;}
    
    m_currentSpectrogramHeight = spectrogram.height();
    m_currentFftSize = getRequiredFftSize(m_currentSpectrogramHeight);
    m_currentTimeInterval = m_timeIntervalComboBox->currentText().toDouble();

    setControlsEnabled(true);
    m_spectrogramViewer->setLoadingState(false);
    m_spectrogramViewer->setSpectrogramImage(spectrogram);
    m_spectrogramViewer->setAudioProperties(durationSec, preciseDurationStr, nativeSampleRate);
    
    emit logMessageGenerated(getCurrentTimestamp() + tr(" Image displayed"));
}


void SingleFileWidget::handleProcessingFailed(const QString& errorMessage) {
    setControlsEnabled(true);
    m_spectrogramViewer->setLoadingState(false);
    QMessageBox::critical(this, tr("Processing Failed"), errorMessage);
    emit logMessageGenerated(getCurrentTimestamp() + tr(" Error: ") + errorMessage);
}


void SingleFileWidget::handleExportFinished(const ImageExporter::ExportResult &result) {
    setControlsEnabled(true);
    m_saveButton->setText(tr("Save"));

    if(result.success) {
        emit logMessageGenerated(getCurrentTimestamp() + " " + result.message + tr("\n                        Location:") + result.outputFilePath);

        QFileInfo fileInfo(result.outputFilePath);
        QImage savedImage(result.outputFilePath);

        if (fileInfo.exists() && !savedImage.isNull()) {
            QString resolution = QString("%1×%2").arg(savedImage.width()).arg(savedImage.height());

            int effectiveBitDepth = savedImage.depth();
            if (savedImage.depth() == 32 && !savedImage.hasAlphaChannel()) {
                effectiveBitDepth = 24;}

            QString bitDepth = QString::number(effectiveBitDepth) + tr("Bit");
            QString size = formatSize(fileInfo.size()); 

            QString imageInfoLog = QString(tr("                        Resolution: %1, Bit Depth: %2, Size: %3"))
                                       .arg(resolution)
                                       .arg(bitDepth)
                                       .arg(size);
            emit logMessageGenerated(imageInfoLog);}
        
        QMessageBox msgBox(QMessageBox::Information, tr("Save Successful"), tr("Image saved successfully."), QMessageBox::NoButton, this);
        msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
        msgBox.exec();}
    else {
        emit logMessageGenerated(getCurrentTimestamp() + tr(" Error: ") + result.message + tr("Location: ") + result.outputFilePath);
        QMessageBox::critical(this, tr("Save Failed"), result.message);}
}


void SingleFileWidget::setControlsEnabled(bool enabled) {
    m_showLogCheckBox->setEnabled(enabled);
    m_showGridCheckBox->setEnabled(enabled);
    m_enableVerticalZoomCheckBox->setEnabled(enabled);
    m_enableWidthLimitCheckBox->setEnabled(enabled);
    m_maxWidthSpinBox->setEnabled(enabled && m_enableWidthLimitCheckBox->isChecked());
    m_imageHeightComboBox->setEnabled(enabled);
    m_timeIntervalComboBox->setEnabled(enabled);
    m_openButton->setEnabled(enabled);
    
    m_saveButton->setEnabled(enabled && !m_currentSpectrogram.isNull());
}