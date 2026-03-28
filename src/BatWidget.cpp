#include "BatWidget.h"
#include "BatWidgetSettings.h"
#include "BatUtils.h"
#include "AppConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QSet>

BatWidget::BatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_currentSettings = BatSettings(); 
    setupUi();
    retranslateUi();
    setControlsEnabledForBatch(false); 
}

BatWidget::~BatWidget(){}

void BatWidget::setPaletteType(PaletteType type){ m_currentSettings.paletteType = type; }

void BatWidget::setupUi(){
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2); 
    QString btnStyle =
        "QPushButton { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: 1px solid #1C222B; padding: 3px 0px; }"
        "QPushButton:hover { background-color: rgb(55, 65, 81); }"
        "QPushButton:pressed { background-color: rgb(30, 36, 45); }"
        "QPushButton:disabled { background-color: rgb(50, 58, 70); color: #888888; }";
    QString pathStyle =
        "QLineEdit { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding: 2px; }";
    auto layoutIn = new QHBoxLayout();
    layoutIn->setSpacing(2);
    m_btnInput = new QPushButton(this);
    m_btnInput->setStyleSheet(btnStyle);
    m_editInput = new QLineEdit(this);
    m_editInput->setReadOnly(true);
    m_editInput->setStyleSheet(pathStyle);
    layoutIn->addWidget(m_btnInput);
    layoutIn->addWidget(m_editInput);
    layout->addLayout(layoutIn);
    auto layoutOut = new QHBoxLayout();
    layoutOut->setSpacing(2);
    m_btnOutput = new QPushButton(this);
    m_btnOutput->setStyleSheet(btnStyle);
    m_editOutput = new QLineEdit(this);
    m_editOutput->setReadOnly(true);
    m_editOutput->setStyleSheet(pathStyle);
    layoutOut->addWidget(m_btnOutput);
    layoutOut->addWidget(m_editOutput);
    layout->addLayout(layoutOut);
    layout->addSpacing(2);
    auto layoutBtns = new QHBoxLayout();
    layoutBtns->setSpacing(5);
    m_btnSettings = new QPushButton(this);
    m_btnSettings->setStyleSheet(btnStyle);
    m_btnStart = new QPushButton(this);
    m_btnStart->setStyleSheet(btnStyle);
    m_btnStop = new QPushButton(this);
    m_btnStop->setStyleSheet(btnStyle);
    layoutBtns->addWidget(m_btnSettings);
    layoutBtns->addStretch();
    layoutBtns->addWidget(m_btnStart);
    layoutBtns->addWidget(m_btnStop);
    layout->addLayout(layoutBtns);
    layout->addSpacing(2);
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    QFont logFont = QApplication::font(); 
    logFont.setPointSize(8);
    m_logEdit->setFont(logFont);
    m_logEdit->setStyleSheet("QTextEdit { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: 1px solid #2B333E; }");
    m_logEdit->setText(tr("[提示] 批量处理已准备就绪 (点击设置切换模式)"));
    layout->addWidget(m_logEdit);
    connect(m_btnInput, &QPushButton::clicked, this, &BatWidget::onBrowseInputPath);
    connect(m_btnOutput, &QPushButton::clicked, this, &BatWidget::onBrowseOutputPath);
    connect(m_btnSettings, &QPushButton::clicked, this, &BatWidget::onSettingsClicked);
    connect(m_btnStart, &QPushButton::clicked, this, &BatWidget::onStartClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &BatWidget::onStopClicked);
}

void BatWidget::retranslateUi() {
    m_btnInput->setText(tr("输入路径"));
    m_editInput->setPlaceholderText(tr("选择包含音频文件的源文件夹"));
    m_btnOutput->setText(tr("输出路径"));
    m_editOutput->setPlaceholderText(tr("选择保存频谱图的目标文件夹"));
    m_btnSettings->setText(tr("设置"));
    m_btnStop->setText(tr("终止任务"));
    if (m_currentState == BatchState::Running) {
        m_btnStart->setText(tr("暂停任务"));
    } else if (m_currentState == BatchState::Paused) {
        m_btnStart->setText(tr("继续任务"));
    } else {
        m_btnStart->setText(tr("开始任务"));
    }
    updateButtonSizes();
}

void BatWidget::updateButtonSizes(){
    auto getHintWidth = [](QPushButton* btn) -> int {
        btn->setMinimumWidth(0);
        btn->setMaximumWidth(QWIDGETSIZE_MAX);
        btn->adjustSize();
        return btn->sizeHint().width() + 10; 
    };
    int w1 = getHintWidth(m_btnInput);
    int w2 = getHintWidth(m_btnOutput);
    int w3 = getHintWidth(m_btnSettings);
    int maxGroup1 = std::max({w1, w2, w3, 80});
    m_btnInput->setFixedWidth(maxGroup1);
    m_btnOutput->setFixedWidth(maxGroup1);
    m_btnSettings->setFixedWidth(maxGroup1);
    int w4 = getHintWidth(m_btnStart);
    int w5 = getHintWidth(m_btnStop);
    int maxGroup2 = std::max({w4, w5, 80});
    m_btnStart->setFixedWidth(maxGroup2);
    m_btnStop->setFixedWidth(maxGroup2);
}

void BatWidget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void BatWidget::onBrowseInputPath(){
    QString path = QFileDialog::getExistingDirectory(this, tr("选择源文件夹"), m_currentSettings.inputPath);
    if (!path.isEmpty()) { 
        m_currentSettings.inputPath = path;
        m_editInput->setText(QDir::toNativeSeparators(path));
    }
}

void BatWidget::onBrowseOutputPath(){
    QString path = QFileDialog::getExistingDirectory(this, tr("选择目标文件夹"), m_currentSettings.outputPath);
    if (!path.isEmpty()) { 
        m_currentSettings.outputPath = path;
        m_editOutput->setText(QDir::toNativeSeparators(path));
    }
}

void BatWidget::onSettingsClicked(){
    BatWidgetSettings dlg(m_currentSettings, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString savedInput = m_currentSettings.inputPath;
        QString savedOutput = m_currentSettings.outputPath;
        m_currentSettings = dlg.getSettings();
        m_currentSettings.inputPath = savedInput;
        m_currentSettings.outputPath = savedOutput;
        QString modeStr = (m_currentSettings.mode == BatchMode::FullLoad) ? tr("全量") : tr("流式");
        appendLog(QString(tr("%1 设置参数已更新 [模式: %2] [线程: %3]"))
            .arg(BatUtils::getCurrentTimestamp())
            .arg(modeStr)
            .arg(m_currentSettings.threadCount));
    }
}

BatchStreamSettings BatWidget::convertToStreamSettings(const BatSettings& s) const{
    BatchStreamSettings bs;
    bs.threadCount = s.threadCount;
    bs.inputPath = s.inputPath;
    bs.outputPath = s.outputPath;
    bs.useMultiThreading = (s.threadCount > 1);
    bs.includeSubfolders = s.includeSubfolders;
    bs.reuseSubfolderStructure = s.reuseSubfolderStructure;
    bs.imageHeight = s.imageHeight;
    bs.timeInterval = s.timeInterval;
    bs.windowType = s.windowType;
    bs.curveType = s.curveType;
    bs.paletteType = s.paletteType;
    bs.minDb = s.minDb;
    bs.maxDb = s.maxDb;
    bs.enableGrid = s.enableGrid;
    bs.enableComponents = s.enableComponents;
    bs.enableWidthLimit = s.enableWidthLimit;
    bs.maxWidth = s.maxWidth;
    bs.exportFormat = s.exportFormat;
    bs.qualityLevel = s.qualityLevel;
    return bs;
}

void BatWidget::onStartClicked(){
    if (m_currentState == BatchState::Idle) {
        if (m_currentSettings.inputPath.isEmpty() || m_currentSettings.outputPath.isEmpty()) {
            QMessageBox msgBox(QMessageBox::Warning, tr("路径缺失"), tr("请先选择有效的输入和输出路径。"), QMessageBox::NoButton, this);
            msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
            msgBox.exec();
            return;
        }
        m_logEdit->clear();
        if (m_currentSettings.mode == BatchMode::FullLoad) {
            emit requestScan(m_currentSettings);
        } else {
            emit requestScanStream(convertToStreamSettings(m_currentSettings));
        }
    } 
    else if (m_currentState == BatchState::Running) {
        if (m_currentSettings.mode == BatchMode::FullLoad) emit pauseBatchRequested();
        else emit pauseBatchStreamRequested();
    }
    else if (m_currentState == BatchState::Paused) {
        if (m_currentSettings.mode == BatchMode::FullLoad) emit resumeBatchRequested();
        else emit resumeBatchStreamRequested();
    }
}

void BatWidget::onScanFinished(QSharedPointer<FileSnapshot> snapshotB){
    m_currentRunningSnapshot = snapshotB;
    if (!m_hasFinishedOnce || m_currentSettings.inputPath != m_lastFinishedSettings.inputPath) {
        if (m_currentSettings.mode == BatchMode::FullLoad) emit requestStartProcessing();
        else emit requestStartProcessingStream();
        return;
    }
    QSharedPointer<FileSnapshot> snapshotA = m_lastFinishedSnapshot;
    QSet<QString> keysA;
    if (snapshotA) {
        const auto& k = snapshotA->keys();
        for (const auto& key : k) keysA.insert(key);
    }
    QSet<QString> keysB;
    if (snapshotB) {
        const auto& k = snapshotB->keys();
        for (const auto& key : k) keysB.insert(key);
    }
    QSet<QString> setAdded = keysB - keysA;
    QSet<QString> setRemoved = keysA - keysB;
    if (!keysB.isEmpty() && setAdded.size() == keysB.size()) {
        if (m_currentSettings.mode == BatchMode::FullLoad) emit requestStartProcessing();
        else emit requestStartProcessingStream();
        return;
    }
    QString msg;
    bool shouldPrompt = false;
    if (setAdded.isEmpty() && setRemoved.isEmpty()) {
        msg = tr("检测到输入路径的音频文件与上次完全一致。\n\n是否重新处理？");
        shouldPrompt = true;
    } 
    else {
        msg = tr("检测到输入路径的音频文件清单发生了变化。\n\n");
        if (!setAdded.isEmpty()) msg += QString(tr("增加 %1 个新文件\n")).arg(setAdded.size());
        if (!setRemoved.isEmpty()) msg += QString(tr("减少 %1 个旧文件\n")).arg(setRemoved.size());
        msg += tr("\n\n是否继续处理？");
        shouldPrompt = true;
    }
    if (shouldPrompt) {
        QMessageBox msgBox(QMessageBox::Question, tr("重复任务确认"), msg, QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setButtonText(QMessageBox::Yes, tr("确定"));
        msgBox.setButtonText(QMessageBox::No, tr("取消"));
        msgBox.setDefaultButton(QMessageBox::No);
        int reply = msgBox.exec();
        if (reply == QMessageBox::No) {
            appendLog(BatUtils::getCurrentTimestamp() + tr(" 任务已经取消"));
            onBatchStopped(); 
            return;
        }
    }
    if (m_currentSettings.mode == BatchMode::FullLoad) emit requestStartProcessing();
    else emit requestStartProcessingStream();
}

void BatWidget::onStopClicked(){
    if (m_currentState == BatchState::Running || m_currentState == BatchState::Paused) {
        if (m_currentSettings.mode == BatchMode::FullLoad) emit stopBatchRequested();
        else emit stopBatchStreamRequested();
    }
}

void BatWidget::appendLog(const QString& message){
    m_logEdit->append(message);
    m_logEdit->ensureCursorVisible();
}

void BatWidget::updateProgress(int current, int total){
    Q_UNUSED(current);
    Q_UNUSED(total);
}

void BatWidget::setControlsEnabledForBatch(bool isRunning){
    bool disabled = isRunning;
    m_btnInput->setEnabled(!disabled);
    m_btnOutput->setEnabled(!disabled);
    m_btnSettings->setEnabled(!disabled);
    m_btnStop->setEnabled(isRunning);
    m_btnStart->setEnabled(true);
}

void BatWidget::onBatchStarted(){
    m_currentState = BatchState::Running;
    m_btnStart->setText(tr("暂停任务"));
    setControlsEnabledForBatch(true);
}

void BatWidget::onBatchPaused(){
    m_currentState = BatchState::Paused;
    m_btnStart->setText(tr("继续任务"));
    appendLog(tr("任务已暂停"));
}

void BatWidget::onBatchResumed(){
    m_currentState = BatchState::Running;
    m_btnStart->setText(tr("暂停任务"));
    appendLog(tr("任务已恢复"));
}

void BatWidget::onBatchStopped(){
    m_currentState = BatchState::Idle;
    m_btnStart->setText(tr("开始任务"));
    setControlsEnabledForBatch(false);
    appendLog(tr("任务已终止"));
}

void BatWidget::onBatchFinished(const QString& summaryReport){
    m_currentState = BatchState::Idle;
    m_btnStart->setText(tr("开始任务"));
    setControlsEnabledForBatch(false);
    appendLog(tr("==================== 任务完成 ===================="));
    appendLog(summaryReport);
    m_lastFinishedSettings = m_currentSettings;
    m_lastFinishedSnapshot = m_currentRunningSnapshot;
    m_hasFinishedOnce = true;
}