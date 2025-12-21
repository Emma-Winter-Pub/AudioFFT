#include "BatWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QSlider>
#include <QFileDialog>
#include <QMessageBox>
#include <QSizePolicy> 


BatWidget::BatWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setControlsEnabledForBatch(false); 
}


BatWidget::~BatWidget(){}


void BatWidget::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    const QString checkBoxStyle = "QCheckBox { spacing: 2px; color: rgb(217, 217, 217); }";

    const QString pathDisplayLabelStyle = "QLabel { "
                                          "    background-color: rgb(78, 90, 110); "
                                          "    border: 1px solid #2B333E; "
                                          "    color: #888888; "
                                          "    padding-left: 5px; "
                                          "}";
    const QString pathDisplayLabelActiveStyle = "QLabel { "
                                                "    background-color: rgb(78, 90, 110); "
                                                "    border: 1px solid #2B333E; "
                                                "    color: rgb(217, 217, 217); "
                                                "    padding-left: 5px; "
                                                "}";
    const QString controlButtonStyle = "QPushButton { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: 1px solid #1C222B; padding: 2px 5px 2px 5px; }"
                                       "QPushButton:hover { background-color: rgb(55, 65, 81); }"
                                       "QPushButton:pressed { background-color: rgb(30, 36, 45); }"
                                       "QPushButton:disabled { background-color: rgb(50, 58, 70); color: #888888; }";
    const QString comboBoxStyle = "QComboBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 3px; }"
                                  "QComboBox QAbstractItemView { background-color: rgb(78, 90, 110); selection-background-color: #5A687A; color: rgb(217, 217, 217); border: 1px solid #2B333E; }";
    const QString spinBoxStyle = "QSpinBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 1px; }";
    const QString textEditStyle = "QTextEdit { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: 1px solid #2B333E; }";

    auto inputLayout = new QHBoxLayout();
    m_inputPathSelectButton = new QPushButton(tr("Input Path"));
    m_inputPathSelectButton->setStyleSheet(controlButtonStyle);
    m_inputPathSelectButton->setFixedWidth(90);
    m_inputPathDisplayLabel = new QLabel(tr("Select source folder containing audio files"));
    m_inputPathDisplayLabel->setStyleSheet(pathDisplayLabelStyle);
    m_inputPathDisplayLabel->setFixedHeight(m_inputPathSelectButton->sizeHint().height());
    inputLayout->addWidget(m_inputPathSelectButton);
    inputLayout->addWidget(m_inputPathDisplayLabel, 1);

    auto outputLayout = new QHBoxLayout();
    m_outputPathSelectButton = new QPushButton(tr("Output Path"));
    m_outputPathSelectButton->setStyleSheet(controlButtonStyle);
    m_outputPathSelectButton->setFixedWidth(90);
    m_outputPathDisplayLabel = new QLabel(tr("Select destination folder for spectrograms"));
    m_outputPathDisplayLabel->setStyleSheet(pathDisplayLabelStyle);
    m_outputPathDisplayLabel->setFixedHeight(m_outputPathSelectButton->sizeHint().height());
    outputLayout->addWidget(m_outputPathSelectButton);
    outputLayout->addWidget(m_outputPathDisplayLabel, 1);

    auto folderOptionsLayout = new QHBoxLayout();
    folderOptionsLayout->setContentsMargins(0, 0, 0, 0);
    folderOptionsLayout->setSpacing(7);
    m_includeSubfoldersCheckBox = new QCheckBox(tr("Scan Subfolders"));
    m_includeSubfoldersCheckBox->setChecked(true);
    folderOptionsLayout->addWidget(m_includeSubfoldersCheckBox);
    m_reuseSubfoldersCheckBox = new QCheckBox(tr("Keep Structure"));
    m_reuseSubfoldersCheckBox->setChecked(true);
    folderOptionsLayout->addWidget(m_reuseSubfoldersCheckBox);

    folderOptionsLayout->addSpacing(10);

    auto formatLabel = new QLabel(tr("Output Format"));
    folderOptionsLayout->addWidget(formatLabel);
    folderOptionsLayout->addSpacing(-5);
    m_formatComboBox = new QComboBox();
    m_formatComboBox->clear(); 
    m_formatComboBox->addItem("PNG (libpng)", "PNG");       
    m_formatComboBox->addItem("PNG (Qt)", "QtPNG");         
    m_formatComboBox->addItem("BMP", "BMP");                
    m_formatComboBox->addItem("TIFF", "TIFF");              
    m_formatComboBox->addItem("JPG", "JPG");                
    m_formatComboBox->addItem("JPEG 2000", "JPEG 2000");    
    m_formatComboBox->addItem("WebP", "WebP");              
    m_formatComboBox->addItem("AVIF", "AVIF");  
    m_formatComboBox->setFixedWidth(100);
    folderOptionsLayout->addWidget(m_formatComboBox);
    m_qualityLabel = new QLabel(tr("Compression"));
    folderOptionsLayout->addWidget(m_qualityLabel);
    folderOptionsLayout->addSpacing(-5);
    m_qualitySlider = new QSlider(Qt::Horizontal);
    m_qualitySlider->setMinimumWidth(140);
    QSizePolicy sliderPolicy = m_qualitySlider->sizePolicy();
    sliderPolicy.setHorizontalPolicy(QSizePolicy::Preferred);
    m_qualitySlider->setSizePolicy(sliderPolicy);
    folderOptionsLayout->addWidget(m_qualitySlider);
    m_qualityValueLabel = new QLabel("6");
    m_qualityValueLabel->setMinimumWidth(25);
    m_qualityValueLabel->setStyleSheet("QLabel { color: rgb(217, 217, 217); }");
    folderOptionsLayout->addWidget(m_qualityValueLabel);
    folderOptionsLayout->addStretch();
    auto paramsAndControlLayout = new QHBoxLayout();
    paramsAndControlLayout->setContentsMargins(0, 0, 0, 0);
    paramsAndControlLayout->setSpacing(7);
    m_gridCheckBox = new QCheckBox(tr("Grid"));
    paramsAndControlLayout->addWidget(m_gridCheckBox);
    m_widthLimitCheckBox = new QCheckBox(tr("Limit Width"));
    m_widthLimitCheckBox->setChecked(true);
    paramsAndControlLayout->addWidget(m_widthLimitCheckBox);
    paramsAndControlLayout->addSpacing(-5);
    m_maxWidthSpinBox = new QSpinBox();
    m_maxWidthSpinBox->setRange(500, 10000);
    m_maxWidthSpinBox->setValue(1000);
    m_maxWidthSpinBox->setFixedWidth(60);
    m_maxWidthSpinBox->setEnabled(true);
    connect(m_widthLimitCheckBox, &QCheckBox::toggled, m_maxWidthSpinBox, &QWidget::setEnabled);
    paramsAndControlLayout->addWidget(m_maxWidthSpinBox);
    auto heightLabel = new QLabel(tr("Height"));
    paramsAndControlLayout->addWidget(heightLabel);
    paramsAndControlLayout->addSpacing(-5);
    m_heightComboBox = new QComboBox();
    QStringList heightIntervals = { "8193", "4097", "2049", "1025", "513", "257", "129", "8000", "7500", "7000", "6500", "6000", "5500", "5000", "4500", "4000", "3500", "3000", "2900", "2800", "2700", "2600", "2500", "2400", "2300", "2200", "2100", "2000", "1900", "1800", "1700", "1600", "1500", "1400", "1300", "1200", "1100", "1000", "900", "800", "700", "600", "500",  "400",  "300", "200", "100" };
    m_heightComboBox->addItems(heightIntervals);
    m_heightComboBox->setCurrentText("513");
    m_heightComboBox->setFixedWidth(55);
    paramsAndControlLayout->addWidget(m_heightComboBox);
    auto intervalLabel = new QLabel(tr("Time Precision"));
    paramsAndControlLayout->addWidget(intervalLabel);
    paramsAndControlLayout->addSpacing(-5);
    m_timeIntervalComboBox = new QComboBox();
    m_timeIntervalComboBox->addItems({"10", "5", "2.5", "2", "1", "0.5", "0.25", "0.2", "0.1", "0.05", "0.025", "0.02", "0.01" });
    m_timeIntervalComboBox->setCurrentText("0.2");
    m_timeIntervalComboBox->setFixedWidth(60);
    paramsAndControlLayout->addWidget(m_timeIntervalComboBox);
    paramsAndControlLayout->addSpacing(43);
    m_startPauseResumeButton = new QPushButton(tr("Start Task"));
    m_stopButton = new QPushButton(tr("Stop Task"));
    m_startPauseResumeButton->setFixedWidth(80); 
    m_stopButton->setFixedWidth(80);
    paramsAndControlLayout->addWidget(m_startPauseResumeButton);
    paramsAndControlLayout->addWidget(m_stopButton);
    paramsAndControlLayout->addStretch(); 
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont("Courier New", 9));
    m_logTextEdit->setStyleSheet(textEditStyle);
    mainLayout->addLayout(inputLayout);
    mainLayout->addLayout(outputLayout);
    mainLayout->addLayout(folderOptionsLayout);
    mainLayout->addLayout(paramsAndControlLayout);
    mainLayout->addWidget(m_logTextEdit, 1);
    m_includeSubfoldersCheckBox->setStyleSheet(checkBoxStyle);
    m_reuseSubfoldersCheckBox->setStyleSheet(checkBoxStyle);
    m_gridCheckBox->setStyleSheet(checkBoxStyle);
    m_widthLimitCheckBox->setStyleSheet(checkBoxStyle);
    m_maxWidthSpinBox->setStyleSheet(spinBoxStyle);
    m_heightComboBox->setStyleSheet(comboBoxStyle);
    m_timeIntervalComboBox->setStyleSheet(comboBoxStyle);
    m_formatComboBox->setStyleSheet(comboBoxStyle);
    m_startPauseResumeButton->setStyleSheet(controlButtonStyle);
    m_stopButton->setStyleSheet(controlButtonStyle);
    formatLabel->setStyleSheet("QLabel { color: rgb(217, 217, 217); }");
    heightLabel->setStyleSheet("QLabel { color: rgb(217, 217, 217); }");
    intervalLabel->setStyleSheet("QLabel { color: rgb(217, 217, 217); }");
    m_qualityLabel->setStyleSheet("QLabel { color: rgb(217, 217, 217); }");

    connect(m_inputPathSelectButton, &QPushButton::clicked, this, &BatWidget::onBrowseInputPath);
    connect(m_outputPathSelectButton, &QPushButton::clicked, this, &BatWidget::onBrowseOutputPath);
    connect(m_startPauseResumeButton, &QPushButton::clicked, this, &BatWidget::onStartPauseResumeClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &BatWidget::onStopClicked);
    connect(m_formatComboBox, &QComboBox::currentTextChanged, this, &BatWidget::onFormatChanged);
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualityValueLabel, qOverload<int>(&QLabel::setNum));

    onFormatChanged(m_formatComboBox->currentText());
}


void BatWidget::onFormatChanged(const QString& format) {
    QString identifier = m_formatComboBox->currentData().toString();
    bool showQualityControls = true;
    if (identifier == "PNG") {
        m_qualityLabel->setText(tr("Compression"));
        m_qualitySlider->setRange(0, 9);
        m_qualitySlider->setSingleStep(1);
        m_qualitySlider->setValue(6);
    } else if (identifier == "JPG") {
        m_qualityLabel->setText(tr("Quality"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setSingleStep(5);
        m_qualitySlider->setValue(85);
    } else if (identifier == "WebP") {
        m_qualityLabel->setText(tr("Quality"));
        m_qualitySlider->setRange(1, 101);
        m_qualitySlider->setSingleStep(5);
        m_qualitySlider->setValue(75);
    } else if (identifier == "AVIF") {
        m_qualityLabel->setText(tr("Quality"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setSingleStep(5);
        m_qualitySlider->setValue(75);
    } else if (identifier == "JPEG 2000") {
        m_qualityLabel->setText(tr("Quality"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setSingleStep(5);
        m_qualitySlider->setValue(75);
    } else {
        showQualityControls = false;
    }
    m_qualityLabel->setVisible(showQualityControls);
    m_qualitySlider->setVisible(showQualityControls);
    m_qualityValueLabel->setVisible(showQualityControls);
    if (showQualityControls) { 
        m_qualityValueLabel->setNum(m_qualitySlider->value()); 
    }
}


void BatWidget::onBrowseInputPath() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Source Folder"), m_lastInputPath);
    if (!path.isEmpty()) { 
        m_lastInputPath = path;
        m_inputPathDisplayLabel->setText(QDir::toNativeSeparators(path));
        m_inputPathDisplayLabel->setStyleSheet("QLabel { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 5px; }");
    }
}


void BatWidget::onBrowseOutputPath() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Destination Folder"), m_lastOutputPath);
    if (!path.isEmpty()) { 
        m_lastOutputPath = path;
        m_outputPathDisplayLabel->setText(QDir::toNativeSeparators(path));
        m_outputPathDisplayLabel->setStyleSheet("QLabel { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 5px; }");
    }
}


void BatWidget::onStartPauseResumeClicked(){
    if (m_currentState == BatchState::Idle){
        if (m_lastInputPath.isEmpty() || m_lastOutputPath.isEmpty()) {
            QMessageBox::warning(this, tr("Missing Paths"), tr("Please select valid input and output paths."));
            return;
        }

        m_logTextEdit->clear();
        BatSettings settings;
        settings.inputPath = m_lastInputPath;
        settings.outputPath = m_lastOutputPath;
        settings.includeSubfolders = m_includeSubfoldersCheckBox->isChecked();
        settings.reuseSubfolderStructure = m_reuseSubfoldersCheckBox->isChecked();
        settings.enableGrid = m_gridCheckBox->isChecked();
        settings.enableWidthLimit = m_widthLimitCheckBox->isChecked();
        settings.maxWidth = m_maxWidthSpinBox->value();
        settings.imageHeight = m_heightComboBox->currentText().toInt();
        settings.timeInterval = m_timeIntervalComboBox->currentText().toDouble();
        settings.exportFormat = m_formatComboBox->currentData().toString();
        settings.qualityLevel = m_qualitySlider->value();

        emit startBatchRequested(settings);
    } 
    else if (m_currentState == BatchState::Running) { emit pauseBatchRequested(); }
    else if (m_currentState == BatchState::Paused) { emit resumeBatchRequested(); }
}


void BatWidget::onStopClicked() {
    if (m_currentState == BatchState::Running || m_currentState == BatchState::Paused) { emit stopBatchRequested(); }
}


void BatWidget::appendLog(const QString& message) {
    m_logTextEdit->append(message);
    m_logTextEdit->ensureCursorVisible();
}


void BatWidget::updateProgress(int current, int total) {
}


void BatWidget::onBatchStarted() {
    m_currentState = BatchState::Running;
    m_startPauseResumeButton->setText(tr("Pause Task"));
    setControlsEnabledForBatch(true);
}


void BatWidget::onBatchPaused() {
    m_currentState = BatchState::Paused;
    m_startPauseResumeButton->setText(tr("Resume Task"));
    appendLog(tr("Task paused"));
}


void BatWidget::onBatchResumed() {
    m_currentState = BatchState::Running;
    m_startPauseResumeButton->setText(tr("Task paused"));
    appendLog(tr("Task resumed"));
}


void BatWidget::onBatchStopped() {
    m_currentState = BatchState::Idle;
    m_startPauseResumeButton->setText(tr("Start Processing"));
    setControlsEnabledForBatch(false);
    appendLog(tr("Task terminated"));
}


void BatWidget::onBatchFinished(const QString& summaryReport) {
    m_currentState = BatchState::Idle;
    m_startPauseResumeButton->setText(tr("Start Task"));
    setControlsEnabledForBatch(false);
    appendLog(tr("==================== Task Completed ===================="));
    appendLog(summaryReport);
}


void BatWidget::setControlsEnabledForBatch(bool isRunning) {
    bool controlsDisabled = isRunning;
    m_inputPathSelectButton->setEnabled(!controlsDisabled);
    m_outputPathSelectButton->setEnabled(!controlsDisabled);
    m_includeSubfoldersCheckBox->setEnabled(!controlsDisabled);
    m_reuseSubfoldersCheckBox->setEnabled(!controlsDisabled);
    m_gridCheckBox->setEnabled(!controlsDisabled);
    m_widthLimitCheckBox->setEnabled(!controlsDisabled);
    m_maxWidthSpinBox->setEnabled(!controlsDisabled && m_widthLimitCheckBox->isChecked());
    m_heightComboBox->setEnabled(!controlsDisabled);
    m_timeIntervalComboBox->setEnabled(!controlsDisabled);
    m_formatComboBox->setEnabled(!controlsDisabled);
    m_qualitySlider->setEnabled(!controlsDisabled);
    m_stopButton->setEnabled(isRunning);
    m_startPauseResumeButton->setEnabled(true);
}