#include "MainWindow.h"
#include "SingleFileWidget.h"
#include "BatWidget.h"
#include "BatProcessor.h"
#include "AppConfig.h"
#include "AboutDialog.h"

#include <QLabel>
#include <QEvent>
#include <QThread>
#include <QWidget>
#include <QScreen>
#include <QComboBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QPushButton>
#include <QStackedWidget>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_processorThread(new QThread(this)),
      m_processor(new BatProcessor())
{
    m_currentCurveType = CurveType::Function0;

    setupUI();
    resize(AppConfig::DEFAULT_WINDOW_WIDTH, AppConfig::DEFAULT_WINDOW_HEIGHT);
    this->setObjectName("mainWindow");
    this->setStyleSheet("QMainWindow#mainWindow { background-color: rgb(59, 68, 83); }");
    setMinimumSize(AppConfig::MIN_WINDOW_WIDTH, AppConfig::MIN_WINDOW_HEIGHT); 
    setWindowTitle("AudioFFT");

    m_processor->moveToThread(m_processorThread);

    connect(m_batWidget, &BatWidget::startBatchRequested, this, &MainWindow::onStartBatch);
    
    qRegisterMetaType<BatSettings>("BatSettings");
    connect(this, &MainWindow::startBatch, m_processor, &BatProcessor::processBatch);
    
    connect(m_batWidget, &BatWidget::pauseBatchRequested,  m_processor, &BatProcessor::pause);
    connect(m_batWidget, &BatWidget::resumeBatchRequested, m_processor, &BatProcessor::resume);
    connect(m_batWidget, &BatWidget::stopBatchRequested,   m_processor, &BatProcessor::stop);
    connect(m_processor, &BatProcessor::logMessage,        m_batWidget, &BatWidget::appendLog);
    connect(m_processor, &BatProcessor::progressUpdated,   m_batWidget, &BatWidget::updateProgress);
    connect(m_processor, &BatProcessor::batchStarted,      m_batWidget, &BatWidget::onBatchStarted);
    connect(m_processor, &BatProcessor::batchPaused,       m_batWidget, &BatWidget::onBatchPaused);
    connect(m_processor, &BatProcessor::batchResumed,      m_batWidget, &BatWidget::onBatchResumed);
    connect(m_processor, &BatProcessor::batchStopped,      m_batWidget, &BatWidget::onBatchStopped);
    connect(m_processor, &BatProcessor::batchFinished,     m_batWidget, &BatWidget::onBatchFinished);

    connect(m_processorThread, &QThread::finished, m_processor, &QObject::deleteLater);
    
    m_processorThread->start();
}


MainWindow::~MainWindow() {
    m_processorThread->quit();
    m_processorThread->wait();
}


void MainWindow::setupUI() {
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 2, 0, 0);
    mainLayout->setSpacing(3);

    auto topBarWidget = new QWidget(this);
    auto topBarLayout = new QHBoxLayout(topBarWidget);
    topBarLayout->setContentsMargins(5, 0, 5, 0);
    topBarLayout->setSpacing(5); 

    auto modeLabel = new QLabel(tr("Mode Selection"), this);
    modeLabel->setStyleSheet("color: rgb(217, 217, 217);");
    m_modeComboBox = new QComboBox(this);
    m_modeComboBox->addItem(tr("Single"));
    m_modeComboBox->addItem(tr("Batch"));
    m_modeComboBox->setFixedWidth(85);
    
    const QString comboBoxStyle = 
        "QComboBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 3px; }"
        "QComboBox QAbstractItemView { background-color: rgb(78, 90, 110); selection-background-color: #5A687A; color: rgb(217, 217, 217); border: 1px solid #2B333E; }";
    m_modeComboBox->setStyleSheet(comboBoxStyle);

    topBarLayout->addWidget(modeLabel);
    topBarLayout->addWidget(m_modeComboBox);
    topBarLayout->addSpacing(10);

    auto curveLabel = new QLabel(tr(" Mapping Function"), this);
    curveLabel->setStyleSheet("color: rgb(217, 217, 217);");
    m_curveTypeComboBox = new QComboBox(this);
    m_curveTypeComboBox->setFixedWidth(100);
    m_curveTypeComboBox->setStyleSheet(comboBoxStyle);

    m_curveTypeComboBox->addItem(tr("01 Linear"), QVariant::fromValue(CurveType::Function0));
    m_curveTypeComboBox->addItem(tr("02 Log1"), QVariant::fromValue(CurveType::Function1));
    m_curveTypeComboBox->addItem(tr("03 Log2"), QVariant::fromValue(CurveType::Function2));
    m_curveTypeComboBox->addItem(tr("04 Sin1"), QVariant::fromValue(CurveType::Function3));
    m_curveTypeComboBox->addItem(tr("05 Sin2"), QVariant::fromValue(CurveType::Function4));
    m_curveTypeComboBox->addItem(tr("06 Quad1"), QVariant::fromValue(CurveType::Function5));
    m_curveTypeComboBox->addItem(tr("07 Quad2"), QVariant::fromValue(CurveType::Function6));
    m_curveTypeComboBox->addItem(tr("08 Cubic1"), QVariant::fromValue(CurveType::Function7));
    m_curveTypeComboBox->addItem(tr("09 Cubic2"), QVariant::fromValue(CurveType::Function8));
    m_curveTypeComboBox->addItem(tr("10 Tan1"), QVariant::fromValue(CurveType::Function9));
    m_curveTypeComboBox->addItem(tr("11 Tan2"), QVariant::fromValue(CurveType::Function10));
    m_curveTypeComboBox->addItem(tr("12 CubicF1"), QVariant::fromValue(CurveType::Function11));
    m_curveTypeComboBox->addItem(tr("13 CubicF2"), QVariant::fromValue(CurveType::Function12));

    topBarLayout->addWidget(curveLabel);
    topBarLayout->addWidget(m_curveTypeComboBox);

    topBarLayout->addStretch(); 
    
    auto aboutButton = new QPushButton(tr("Help"), this);
    aboutButton->setFlat(true);
    aboutButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: none; color: #CCCCCC; padding: 0px; }"
        "QPushButton:hover { color: white; text-decoration: underline; }"
    );
    topBarLayout->addWidget(aboutButton);

    m_stackedWidget = new QStackedWidget(this);
    m_singleFileWidget = new SingleFileWidget();
    m_batWidget = new BatWidget();
    m_stackedWidget->addWidget(m_singleFileWidget);
    m_stackedWidget->addWidget(m_batWidget);
    
    mainLayout->addWidget(topBarWidget);
    mainLayout->addWidget(m_stackedWidget, 1);
    
    setCentralWidget(centralWidget);

    m_logWindow = new QWidget(nullptr, Qt::Window | Qt::Tool);
    m_logWindow->setWindowTitle(tr("Log"));
    m_logWindow->setStyleSheet(
        "QWidget { background-color: rgb(59, 68, 83); }" 
        "QScrollBar:vertical { border: none; background: rgb(34, 41, 51); width: 10px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: rgb(78, 90, 110); min-height: 20px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: rgb(85, 98, 118); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        "QScrollBar:horizontal { border: none; background: rgb(34, 41, 51); height: 10px; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: rgb(78, 90, 110); min-width: 20px; border-radius: 5px; }"
        "QScrollBar::handle:horizontal:hover { background: rgb(85, 98, 118); }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
    );
    auto logLayout = new QVBoxLayout(m_logWindow);
    logLayout->setContentsMargins(5,5,5,5);
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont("Courier New", 9));
    m_logTextEdit->setStyleSheet(
        "QTextEdit { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: 1px solid #2B333E; }"
    );
    logLayout->addWidget(m_logTextEdit);
    m_logWindow->resize(AppConfig::LOG_AREA_DEFAULT_WIDTH, AppConfig::LOG_AREA_DEFAULT_HEIGHT);
    m_logWindow->installEventFilter(this);
    m_showLogCheckBox = m_singleFileWidget->getShowLogCheckBox();
    
    connect(m_modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAboutDialog);
    connect(m_singleFileWidget, &SingleFileWidget::logMessageGenerated, this, &MainWindow::appendLogMessage);
    connect(m_showLogCheckBox, &QCheckBox::toggled, this, &MainWindow::toggleLogWindow);
    connect(m_singleFileWidget, &SingleFileWidget::filePathChanged, this, &MainWindow::updateWindowTitle);
    connect(m_curveTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCurveTypeChanged);
}


void MainWindow::onStartBatch(const BatSettings& settings){
    BatSettings settingsWithCurve = settings;
    settingsWithCurve.curveType = m_currentCurveType;
    emit startBatch(settingsWithCurve);
}


void MainWindow::onCurveTypeChanged(int index){
    if (index < 0) return;
    m_currentCurveType = m_curveTypeComboBox->itemData(index).value<CurveType>();
    m_singleFileWidget->setCurveType(m_currentCurveType);
}


void MainWindow::onModeChanged(int index){
    if (index == 1) {
        if (m_showLogCheckBox->isChecked()) {
            m_logWindow->hide();
            m_showLogCheckBox->blockSignals(true);
            m_showLogCheckBox->setChecked(false);
            m_showLogCheckBox->blockSignals(false);}}
    
    m_stackedWidget->setCurrentIndex(index);
    
    if (index == 0) {
        updateWindowTitle(m_singleFileWidget->getCurrentFilePath());}
    else {
        setWindowTitle("AudioFFT");}
}


void MainWindow::showAboutDialog(){
    AboutDialog dialog(this);
    dialog.exec();
}


void MainWindow::updateWindowTitle(const QString& filePath){
    if (m_modeComboBox->currentIndex() == 0) {
        if (filePath.isEmpty()) {
            setWindowTitle("AudioFFT");}
       else {
            QFileInfo fileInfo(filePath);
            setWindowTitle(QString("AudioFFT - %1").arg(fileInfo.fileName()));}}
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_logWindow && event->type() == QEvent::Close) {
        m_showLogCheckBox->blockSignals(true);
        m_showLogCheckBox->setChecked(false);
        m_showLogCheckBox->blockSignals(false);}
    return QMainWindow::eventFilter(watched, event);
}


void MainWindow::toggleLogWindow(bool checked) {
    if(checked) {m_logWindow->show();}
    else {m_logWindow->hide();}
}


void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_logWindow) {
        m_logWindow->close();}
    QMainWindow::closeEvent(event);
}


void MainWindow::appendLogMessage(const QString& message) {
    if (message.isEmpty()) {m_logTextEdit->clear();}
    else {m_logTextEdit->append(message);}
}