#include "MainWindow.h"
#include "SingleFileWidget.h"
#include "FastStreamingWidget.h"
#include "SpectrogramProcessor.h"
#include "BatWidget.h"
#include "BatProcessor.h"
#include "BatchStreamTypes.h"
#include "BatchStreamProcessor.h"
#include "RibbonButton.h"
#include "BatTypes.h"
#include "AppConfig.h"
#include "AboutDialog.h"
#include "PlayerControlBar.h"
#include "Utils.h"
#include "GlobalSettingsDialog.h"
#include "GlobalPreferences.h"
#include "ScreenshotManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QMenu>
#include <QApplication>
#include <QTextEdit>
#include <QCheckBox>
#include <QCloseEvent>
#include <QThread>
#include <QFileInfo> 
#include <QTimer>
#include <QEvent>
#include <QKeyEvent> 
#include <QFontDatabase>
#include <QSettings> 
#include <QProcess>
#include <QDir>
#include <QSurfaceFormat>

MainWindow::MainWindow(const QString& initialLangCode, QWidget *parent)
    : QMainWindow(parent),
      m_processorThread(new QThread(this)),
      m_processor(new BatProcessor()),
      m_batchStreamThread(new QThread(this)),
      m_batchStreamProcessor(new BatchStreamProcessor()),
      m_langCycleTimer(new QTimer(this)), 
      m_langCycleIndex(0)                 
{
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
    qRegisterMetaType<BatSettings>("BatSettings");
    qRegisterMetaType<FileSnapshot>("FileSnapshot");
    qRegisterMetaType<BatchStreamSettings>("BatchStreamSettings");
    qRegisterMetaType<QSharedPointer<BatchStreamFileSnapshot>>("QSharedPointer<BatchStreamFileSnapshot>");
    qRegisterMetaType<ProcessMode>("ProcessMode");
    setupGlobalStyle();
    setupUI();
    m_screenshotManager = new ScreenshotManager(this, this);
    GlobalPreferences prefs = GlobalPreferences::load();
    applyGlobalSettings(prefs, false); 
    m_currentLangCode = initialLangCode; 
    connect(m_langCycleTimer, &QTimer::timeout, this, &MainWindow::onLanguageTimerTimeout);
    if (m_currentLangCode.isEmpty()) {
        m_langCycleTimer->start(1000);
    }
    retranslateUi();
    QSize layoutMinSize = this->minimumSizeHint();
    int targetW = std::max(AppConfig::DEFAULT_WINDOW_WIDTH, layoutMinSize.width());
    int targetH = std::max(AppConfig::DEFAULT_WINDOW_HEIGHT, layoutMinSize.height());
    resize(targetW, targetH);
    m_processor->moveToThread(m_processorThread);
    connect(m_processorThread, &QThread::finished, m_processor, &QObject::deleteLater);
    m_processorThread->start();
    connect(m_batWidget, &BatWidget::requestScan, m_processor, &BatProcessor::scanDirectory);
    connect(m_processor, &BatProcessor::scanFinished, m_batWidget, &BatWidget::onScanFinished);
    connect(m_batWidget, &BatWidget::requestStartProcessing, m_processor, &BatProcessor::startProcessing);
    connect(m_batWidget, &BatWidget::pauseBatchRequested, m_processor, &BatProcessor::pause);
    connect(m_batWidget, &BatWidget::resumeBatchRequested, m_processor, &BatProcessor::resume);
    connect(m_batWidget, &BatWidget::stopBatchRequested, m_processor, &BatProcessor::stop);
    connect(m_processor, &BatProcessor::logMessage, m_batWidget, &BatWidget::appendLog);
    connect(m_processor, &BatProcessor::progressUpdated, m_batWidget, &BatWidget::updateProgress);
    connect(m_processor, &BatProcessor::batchStarted, m_batWidget, &BatWidget::onBatchStarted);
    connect(m_processor, &BatProcessor::batchPaused, m_batWidget, &BatWidget::onBatchPaused);
    connect(m_processor, &BatProcessor::batchResumed, m_batWidget, &BatWidget::onBatchResumed);
    connect(m_processor, &BatProcessor::batchStopped, m_batWidget, &BatWidget::onBatchStopped);
    connect(m_processor, &BatProcessor::batchFinished, m_batWidget, &BatWidget::onBatchFinished);
    m_batchStreamProcessor->moveToThread(m_batchStreamThread);
    connect(m_batchStreamThread, &QThread::finished, m_batchStreamProcessor, &QObject::deleteLater);
    m_batchStreamThread->start();
    connect(m_batWidget, &BatWidget::requestScanStream, m_batchStreamProcessor, &BatchStreamProcessor::scanDirectory);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::scanFinished, m_batWidget, &BatWidget::onScanFinished);
    connect(m_batWidget, &BatWidget::requestStartProcessingStream, m_batchStreamProcessor, &BatchStreamProcessor::startProcessing);
    connect(m_batWidget, &BatWidget::pauseBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::pause);
    connect(m_batWidget, &BatWidget::resumeBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::resume);
    connect(m_batWidget, &BatWidget::stopBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::stop);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::logMessage, m_batWidget, &BatWidget::appendLog);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::progressUpdated, m_batWidget, &BatWidget::updateProgress);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchStarted, m_batWidget, &BatWidget::onBatchStarted);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchPaused, m_batWidget, &BatWidget::onBatchPaused);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchResumed, m_batWidget, &BatWidget::onBatchResumed);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchStopped, m_batWidget, &BatWidget::onBatchStopped);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchFinished, m_batWidget, &BatWidget::onBatchFinished);
    QTimer::singleShot(1000, this, [this](){
        if(m_batWidget) {
            m_batWidget->adjustSize();
            m_batWidget->ensurePolished();
        }
        QEvent event(QEvent::LayoutRequest);
        QApplication::sendEvent(this, &event);
    });
}

MainWindow::~MainWindow() {
    m_processorThread->quit();
    m_processorThread->wait();
    m_batchStreamThread->quit();
    m_batchStreamThread->wait();
}

void MainWindow::onSwitchLanguage(const QString& langCode) {
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    if (settings.value("language").toString() == langCode && !m_currentLangCode.isEmpty()) {
        return;
    }
    settings.setValue("language", langCode);
    settings.sync();
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QProcess::startDetached(program, arguments);
    QApplication::quit();
}

void MainWindow::onLanguageTimerTimeout() {
    static const QStringList demoLanguages = {
        "简体中文",
        "繁體中文",
        "日本語",
        "한국어",
        "Deutsch",
        "English",
        "Français",
        "Русский"
    };
    m_btnLanguage->setText(demoLanguages[m_langCycleIndex]);
    m_langCycleIndex = (m_langCycleIndex + 1) % demoLanguages.size();
}

void MainWindow::retranslateUi() {
    QString currentPath;
    if (m_currentWorkspaceIndex == 0) currentPath = m_fastStreamingWidget->getCurrentFilePath();
    else if (m_currentWorkspaceIndex == 1) currentPath = m_singleFileWidget->getCurrentFilePath();
    updateWindowTitle(currentPath);
    m_btnGlobalSettings->setText(tr("设置"));
    m_btnAbout->setText(tr("帮助"));
    if (!m_langCycleTimer->isActive()) {
        if (m_currentLangCode == "zh-JT") m_btnLanguage->setText("简体中文");
        else if (m_currentLangCode == "zh-FT") m_btnLanguage->setText("繁體中文");
        else if (m_currentLangCode == "ja") m_btnLanguage->setText("日本語");
        else if (m_currentLangCode == "ko") m_btnLanguage->setText("한국어");
        else if (m_currentLangCode == "de") m_btnLanguage->setText("Deutsch");
        else if (m_currentLangCode == "en") m_btnLanguage->setText("English");
        else if (m_currentLangCode == "fr") m_btnLanguage->setText("Français");
        else if (m_currentLangCode == "ru") m_btnLanguage->setText("Русский");
        else m_btnLanguage->setText("Language");
    }
    QMenu* menuLang = m_btnLanguage->menu();
    if (!menuLang) {
        menuLang = new QMenu(this);
        m_btnLanguage->setMenu(menuLang);
    }
    menuLang->clear();
    menuLang->addAction("简体中文",  [this](){ onSwitchLanguage("zh-JT"); });
    menuLang->addAction("繁體中文",  [this](){ onSwitchLanguage("zh-FT"); });
    menuLang->addAction("日本語",    [this](){ onSwitchLanguage("ja"); });
    menuLang->addAction("한국어",    [this](){ onSwitchLanguage("ko"); });
    menuLang->addAction("Deutsch",  [this](){ onSwitchLanguage("de"); });
    menuLang->addAction("English",  [this](){ onSwitchLanguage("en"); });
    menuLang->addAction("Français", [this](){ onSwitchLanguage("fr"); });
    menuLang->addAction("Русский",  [this](){ onSwitchLanguage("ru"); });
    QMenu* menuWorkspace = m_btnWorkspace->menu();
    if (!menuWorkspace) {
        menuWorkspace = new QMenu(this);
        m_btnWorkspace->setMenu(menuWorkspace);
    }
    menuWorkspace->clear();
    menuWorkspace->addAction(tr("流式"), [=](){ onWorkspaceChanged(0); });
    menuWorkspace->addAction(tr("全量"), [=](){ onWorkspaceChanged(1); });
    menuWorkspace->addAction(tr("批量"), [=](){ onWorkspaceChanged(2); });
    QString wsName;
    if (m_currentWorkspaceIndex == 0) wsName = tr("工作空间：流式");
    else if (m_currentWorkspaceIndex == 1) wsName = tr("工作空间：全量");
    else if (m_currentWorkspaceIndex == 2) wsName = tr("工作空间：批量");
    else wsName = tr("工作空间：全量");
    m_btnWorkspace->setText(wsName);
    if (m_logWindow) m_logWindow->setWindowTitle(tr("日志"));
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupGlobalStyle(){
    QString qss = R"(
        QMainWindow, QWidget#centralWidget { background-color: #3B4453; color: #E0E0E0; font-size: 9pt; }
        QToolButton { background-color: transparent; border: none; border-radius: 2px; padding: 0px -4px; color: #D9D9D9; }
        QToolButton:hover { background-color: #4E5A6E; color: white; }
        QToolButton:pressed { background-color: #2B333E; }
        QToolButton:checked { background-color: #4A90E2; color: white; }
        QToolButton::menu-indicator { image: none; }
        QMenu { background-color: #2F3642; border: 1px solid #111; }
        QMenu::item { padding: 3px 20px; color: #DDD; margin: 1px 0; }
        QMenu::item:selected { background-color: #4A90E2; color: white; }
        QSpinBox { background-color: #222; border: 1px solid #555; color: white; padding: 1px; }
        QCheckBox { color: #DDD; spacing: 4px; }
        QScrollBar:vertical { border: none; background: rgb(34, 41, 51); width: 10px; margin: 0px; }
        QScrollBar::handle:vertical { background: rgb(78, 90, 110); min-height: 20px; border-radius: 5px; }
    )";
    qApp->setStyleSheet(qss);
}

void MainWindow::setupUI() {
    auto centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    auto topBarWidget = new QWidget(this);
    auto topLayout = new QHBoxLayout(topBarWidget);
    topLayout->setContentsMargins(5, 2, 5, 2);
    topLayout->setSpacing(0);
    m_btnWorkspace = new RibbonButton("", this); 
    m_btnWorkspace->setStyleSheet("color: rgb(217, 217, 217); font-size: 10pt;");
    m_btnWorkspace->setFocusPolicy(Qt::NoFocus); 
    m_btnLanguage = new RibbonButton("", this); 
    m_btnLanguage->setStyleSheet("color: #AAA; font-size: 9pt;");
    m_btnLanguage->setFocusPolicy(Qt::NoFocus);
    m_btnAbout = new QPushButton(this); 
    m_btnAbout->setFlat(true);
    m_btnAbout->setStyleSheet("QPushButton { color: #AAA; text-decoration: underline; border: none; } QPushButton:hover { color: white; }");
    connect(m_btnAbout, &QPushButton::clicked, this, &MainWindow::showAboutDialog);
    m_btnAbout->setFocusPolicy(Qt::NoFocus);
    topLayout->addWidget(m_btnWorkspace);
    topLayout->addSpacing(20);
    m_playerControlBar = new PlayerControlBar(this);
    topLayout->addWidget(m_playerControlBar);
    connect(m_playerControlBar, &PlayerControlBar::timeChanged, this, [this](double seconds){
        QWidget* current = m_stackedWidget->currentWidget();
        if (current == m_singleFileWidget) {
            m_singleFileWidget->setPlayheadPosition(seconds);
        } else if (current == m_fastStreamingWidget) {
            m_fastStreamingWidget->setPlayheadPosition(seconds);
        }
    });
    connect(m_playerControlBar, &PlayerControlBar::stateChanged, this, [this](PlayerController::State state){
        bool visible = (state != PlayerController::Stopped);
        QWidget* current = m_stackedWidget->currentWidget();
        if (current == m_singleFileWidget) {
            m_singleFileWidget->setPlayheadVisible(visible);
        } else if (current == m_fastStreamingWidget) {
            m_fastStreamingWidget->setPlayheadVisible(visible);
        }
    });
    topLayout->addStretch();
    m_btnGlobalSettings = new RibbonButton(tr("设置"), this);
    m_btnGlobalSettings->setStyleSheet("color: #AAA; font-size: 9pt;");
    m_btnGlobalSettings->setFocusPolicy(Qt::NoFocus);
    connect(m_btnGlobalSettings, &RibbonButton::clicked, this, &MainWindow::onGlobalSettingsClicked);
    topLayout->addWidget(m_btnGlobalSettings);
    topLayout->addSpacing(10);
    topLayout->addWidget(m_btnLanguage);
    topLayout->addSpacing(10);
    topLayout->addWidget(m_btnAbout);
    topLayout->addSpacing(5);
    m_stackedWidget = new QStackedWidget(this);
    m_fastStreamingWidget = new FastStreamingWidget(this); 
    m_singleFileWidget = new SingleFileWidget(this);       
    m_batWidget = new BatWidget(this);                     
    m_stackedWidget->addWidget(m_fastStreamingWidget); 
    m_stackedWidget->addWidget(m_singleFileWidget);    
    m_stackedWidget->addWidget(m_batWidget);           
    mainLayout->addWidget(topBarWidget);
    mainLayout->addWidget(m_stackedWidget, 1);
    setCentralWidget(centralWidget);
    m_logWindow = new QWidget(nullptr, Qt::Window | Qt::Tool);
    m_logWindow->resize(AppConfig::LOG_AREA_DEFAULT_WIDTH, AppConfig::LOG_AREA_DEFAULT_HEIGHT);
    m_logWindow->setStyleSheet(qApp->styleSheet());
    auto logLayout = new QVBoxLayout(m_logWindow);
    logLayout->setContentsMargins(0,0,0,0);
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    QFont logFont = QApplication::font(); 
    logFont.setPointSize(8);
    m_logTextEdit->setFont(logFont);
    m_logTextEdit->setStyleSheet("background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: none;");
    logLayout->addWidget(m_logTextEdit);
    m_logWindow->installEventFilter(this);
    connect(m_singleFileWidget, &SingleFileWidget::logMessageGenerated, this, &MainWindow::appendLogMessage);
    connect(m_singleFileWidget->getShowLogCheckBox(), &QCheckBox::toggled, this, &MainWindow::toggleLogWindow);
    connect(m_singleFileWidget, &SingleFileWidget::filePathChanged, this, &MainWindow::updateWindowTitle);
    connect(m_singleFileWidget, &SingleFileWidget::playerProviderReady, this, &MainWindow::onPlayerProviderReady);
    connect(m_singleFileWidget, &SingleFileWidget::seekRequested, m_playerControlBar, &PlayerControlBar::seek);
    connect(m_fastStreamingWidget, &FastStreamingWidget::logMessageGenerated, this, &MainWindow::appendLogMessage);
    connect(m_fastStreamingWidget->getShowLogCheckBox(), &QCheckBox::toggled, this, &MainWindow::toggleLogWindow);
    connect(m_fastStreamingWidget, &FastStreamingWidget::filePathChanged, this, &MainWindow::updateWindowTitle);
    connect(m_fastStreamingWidget, &FastStreamingWidget::playerProviderReady, this, &MainWindow::onPlayerProviderReady);
    connect(m_fastStreamingWidget, &FastStreamingWidget::seekRequested, m_playerControlBar, &PlayerControlBar::seek);
    m_logWindow->ensurePolished(); 
    m_logTextEdit->ensurePolished();
    m_currentWorkspaceIndex = -1; 
    onWorkspaceChanged(1); 
}

void MainWindow::onWorkspaceChanged(int index){
    int oldIndex = m_currentWorkspaceIndex;
    m_currentWorkspaceIndex = index;
    if (index >= m_stackedWidget->count()) index = 2;
    m_stackedWidget->setCurrentIndex(index);
    if (m_playerControlBar) {
        m_playerControlBar->switchWorkspace(oldIndex, index);
    }
    bool shouldShowLog = false;
    QString currentFilePath;
    
    if (index == 0) {
        currentFilePath = m_fastStreamingWidget->getCurrentFilePath();
        shouldShowLog = m_fastStreamingWidget->getShowLogCheckBox()->isChecked();
    } 
    else if (index == 1) {
        currentFilePath = m_singleFileWidget->getCurrentFilePath();
        shouldShowLog = m_singleFileWidget->getShowLogCheckBox()->isChecked();
    } 
    else if (index == 2) {
        shouldShowLog = false;
    }
    if (m_btnGlobalSettings) {
        if (index == 2) {
            m_btnGlobalSettings->hide();
        } else {
            m_btnGlobalSettings->show();
        }
    }
    retranslateUi();
    if (shouldShowLog) {
        if (!m_logWindow->isVisible()) m_logWindow->show();
    } else {
        if (m_logWindow->isVisible()) m_logWindow->hide();
    }
}

void MainWindow::onPlayerProviderReady(QSharedPointer<IPlayerProvider> provider, double duration) {
    int sourceIndex = -1;
    if (sender() == m_fastStreamingWidget) sourceIndex = 0;
    else if (sender() == m_singleFileWidget) sourceIndex = 1;
    if (sourceIndex != -1 && m_playerControlBar) {
        bool isActive = (sourceIndex == m_currentWorkspaceIndex);
        m_playerControlBar->setProvider(sourceIndex, provider, duration, isActive);
    }
}

void MainWindow::updateWindowTitle(const QString &title) {
    if (title.isEmpty()) setWindowTitle("AudioFFT");
    else {
        QFileInfo fileInfo(title);
        setWindowTitle("AudioFFT - " + fileInfo.fileName());
    }
}

void MainWindow::showAboutDialog() {
    AboutDialog dialog(this);
    dialog.exec();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_logWindow && event->type() == QEvent::Close) {
        toggleLogWindow(false);
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::toggleLogWindow(bool checked) {
    if (checked) m_logWindow->show();
    else m_logWindow->hide();
    QCheckBox* boxSingle = m_singleFileWidget->getShowLogCheckBox();
    if (boxSingle->isChecked() != checked) {
        boxSingle->setChecked(checked); 
    }
    QCheckBox* boxStream = m_fastStreamingWidget->getShowLogCheckBox();
    if (boxStream->isChecked() != checked) {
        boxStream->setChecked(checked);
    }
}

void MainWindow::appendLogMessage(const QString& message) {
    if (message.isEmpty()) m_logTextEdit->clear();
    else m_logTextEdit->append(message);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_logWindow) m_logWindow->close();
    QMainWindow::closeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        if (m_playerControlBar && m_playerControlBar->isVisible()) {
            QMetaObject::invokeMethod(m_playerControlBar, "onPlayClicked");
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onGlobalSettingsClicked() {
    GlobalSettingsDialog dlg(this);
    connect(&dlg, &GlobalSettingsDialog::settingsChanged, this, [this](const GlobalPreferences& prefs){
        applyGlobalSettings(prefs, true);
    });
    dlg.exec();
}

void MainWindow::applyGlobalSettings(const GlobalPreferences& prefs, bool isRuntime) {
    CrosshairStyle style;
    style.lineLength = prefs.crosshairLength;
    style.lineWidth = prefs.crosshairWidth;
    style.color = prefs.crosshairColor;
    PlayheadStyle phStyle;
    phStyle.visible = prefs.playheadVisible;
    phStyle.lineWidth = prefs.playheadLineWidth;
    phStyle.lineColor = prefs.playheadColor;
    phStyle.handleColor = prefs.playheadHandleColor;
    if (m_singleFileWidget) {
        m_singleFileWidget->updateCrosshairStyle(style, prefs.enableCrosshair);
        m_singleFileWidget->setIndicatorVisibility(prefs.showCoordFreq, prefs.showCoordTime, prefs.showCoordDb);
        m_singleFileWidget->updateSpectrumProfileStyle(
            prefs.showSpectrumProfile, 
            prefs.spectrumProfileColor, 
            prefs.spectrumProfileLineWidth,
            prefs.spectrumProfileFilled,
            prefs.spectrumProfileFillAlpha,
            prefs.spectrumProfileType
        );
        m_singleFileWidget->updatePlayheadStyle(phStyle);
        m_singleFileWidget->setProfileFrameRate(prefs.profileFrameRate);
        m_singleFileWidget->updateProbeConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        if (!isRuntime) {
            m_singleFileWidget->applyGlobalPreferences(prefs, true);
        }
    }
    if (m_fastStreamingWidget) {
        m_fastStreamingWidget->updateCrosshairStyle(style, prefs.enableCrosshair);
        m_fastStreamingWidget->setIndicatorVisibility(prefs.showCoordFreq, prefs.showCoordTime, prefs.showCoordDb);
        m_fastStreamingWidget->updateSpectrumProfileStyle(
            prefs.showSpectrumProfile, 
            prefs.spectrumProfileColor, 
            prefs.spectrumProfileLineWidth,
            prefs.spectrumProfileFilled,
            prefs.spectrumProfileFillAlpha,
            prefs.spectrumProfileType
        );
        m_fastStreamingWidget->updatePlayheadStyle(phStyle);
        m_fastStreamingWidget->setProfileFrameRate(prefs.profileFrameRate);
        m_fastStreamingWidget->updateProbeConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        if (!isRuntime) {
            m_fastStreamingWidget->applyGlobalPreferences(prefs, true);
        }
    }
    if (m_playerControlBar) {
        m_playerControlBar->setPlayerFrameRate(prefs.playerFrameRate);
    }
    if (m_screenshotManager) {
        m_screenshotManager->updateSettings();
    }
}