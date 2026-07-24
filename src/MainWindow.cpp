#include "MainWindow.h"
#include "AppConfig.h"
#include "AboutDialog.h"
#include "RibbonButton.h"
#include "LogListView.h"
#include "LogListModel.h"
#include "PlayerControlBar.h"
#include "GlobalSettingsDialog.h"
#include "GlobalPreferences.h"
#include "ScreenshotManager.h"
#include "ColorPaletteFactory.h"
#include "ColorUserPaletteLoader.h"
#include "FullLoadWidget.h"
#include "FullLoadUtils.h"
#include "FullLoadSpectrogramProcessor.h"
#include "StreamingWidget.h"
#include "BatchFullLoadTypes.h"
#include "BatchFullLoadWidget.h"
#include "BatchFullLoadProcessor.h"
#include "BatchStreamTypes.h"
#include "BatchStreamProcessor.h"

#include <QDir>
#include <QUrl>
#include <QMenu>
#include <QTimer>
#include <QEvent>
#include <QThread>
#include <QProcess>
#include <QTextEdit>
#include <QCheckBox>
#include <QFileInfo>
#include <QKeyEvent>
#include <QSettings>
#include <QCloseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QLocalSocket>
#include <QFontDatabase>
#include <QStackedWidget>
#include <QSurfaceFormat>

MainWindow::MainWindow(const QString& initialLangCode, const QString& externalFile, QWidget *parent)
    : QMainWindow(parent),
      m_processorThread(new QThread(this)),
      m_processor(new BatchFullLoadProcessor()),
      m_batchStreamThread(new QThread(this)),
      m_batchStreamProcessor(new BatchStreamProcessor()),
      m_langCycleTimer(new QTimer(this)), 
      m_langCycleIndex(0),
      m_externalFileToLoad(externalFile)
{
    qRegisterMetaType<BatchSettings>("BatchSettings");
    qRegisterMetaType<FileSnapshot>("FileSnapshot");
    qRegisterMetaType<BatchStreamSettings>("BatchStreamSettings");
    qRegisterMetaType<QSharedPointer<BatchStreamFileSnapshot>>("QSharedPointer<BatchStreamFileSnapshot>");
    qRegisterMetaType<ProcessMode>("ProcessMode");
    ColorPaletteFactory::instance().initialize();
    ColorUserPaletteLoader::loadUserPalettes([](const QString& msg){
        qDebug() << msg;
    });
    setupGlobalStyle();
    setupUI();
    m_screenshotManager = new ScreenshotManager(this, this);
    GlobalPreferences prefs = GlobalPreferences::load();
    applyGlobalSettings(prefs, false);
    m_currentLangCode = initialLangCode;
    if (!prefs.allowMultipleInstances) {
        QLocalServer::removeServer("AudioFFT_SingleInstance_Server");
        m_localServer = new QLocalServer(this);
        connect(m_localServer, &QLocalServer::newConnection, this, &MainWindow::onNewLocalConnection);
        m_localServer->listen("AudioFFT_SingleInstance_Server");
    }
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
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::requestScan, m_processor, &BatchFullLoadProcessor::scanDirectory);
    connect(m_processor, &BatchFullLoadProcessor::scanFinished, m_batchFullLoadWidget, &BatchFullLoadWidget::onScanFinished);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::requestStartProcessing, m_processor, &BatchFullLoadProcessor::startProcessing);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::pauseBatchRequested, m_processor, &BatchFullLoadProcessor::pause, Qt::DirectConnection);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::resumeBatchRequested, m_processor, &BatchFullLoadProcessor::resume, Qt::DirectConnection);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::stopBatchRequested, m_processor, &BatchFullLoadProcessor::stop, Qt::DirectConnection);
    connect(m_processor, &BatchFullLoadProcessor::logMessage, m_batchFullLoadWidget, &BatchFullLoadWidget::appendLog);
    connect(m_processor, &BatchFullLoadProcessor::progressUpdated, m_batchFullLoadWidget, &BatchFullLoadWidget::updateProgress);
    connect(m_processor, &BatchFullLoadProcessor::batchStarted, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchStarted);
    connect(m_processor, &BatchFullLoadProcessor::batchPaused, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchPaused);
    connect(m_processor, &BatchFullLoadProcessor::batchResumed, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchResumed);
    connect(m_processor, &BatchFullLoadProcessor::batchStopped, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchStopped);
    connect(m_processor, &BatchFullLoadProcessor::batchFinished, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchFinished);
    m_batchStreamProcessor->moveToThread(m_batchStreamThread);
    connect(m_batchStreamThread, &QThread::finished, m_batchStreamProcessor, &QObject::deleteLater);
    m_batchStreamThread->start();
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::requestScanStream, m_batchStreamProcessor, &BatchStreamProcessor::scanDirectory);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::scanFinished, m_batchFullLoadWidget, &BatchFullLoadWidget::onScanFinished);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::requestStartProcessingStream, m_batchStreamProcessor, &BatchStreamProcessor::startProcessing);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::pauseBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::pause, Qt::DirectConnection);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::resumeBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::resume, Qt::DirectConnection);
    connect(m_batchFullLoadWidget, &BatchFullLoadWidget::stopBatchStreamRequested, m_batchStreamProcessor, &BatchStreamProcessor::stop, Qt::DirectConnection);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::logMessage, m_batchFullLoadWidget, &BatchFullLoadWidget::appendLog);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::progressUpdated, m_batchFullLoadWidget, &BatchFullLoadWidget::updateProgress);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchStarted, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchStarted);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchPaused, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchPaused);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchResumed, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchResumed);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchStopped, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchStopped);
    connect(m_batchStreamProcessor, &BatchStreamProcessor::batchFinished, m_batchFullLoadWidget, &BatchFullLoadWidget::onBatchFinished);
    QTimer::singleShot(1000, this, [this](){
        if(m_batchFullLoadWidget) {
            m_batchFullLoadWidget->adjustSize();
            m_batchFullLoadWidget->ensurePolished();
        }
        QEvent event(QEvent::LayoutRequest);
        QApplication::sendEvent(this, &event);
        if (!m_externalFileToLoad.isEmpty()) {
            processExternalFile(m_externalFileToLoad);
        }
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
    if (m_localServer) {
        m_localServer->close();
        delete m_localServer;
        m_localServer = nullptr;
    }
    QLocalServer::removeServer("AudioFFT_SingleInstance_Server");
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    if (!arguments.isEmpty()) {
        arguments.removeFirst();
    }
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
    if (m_currentWorkspaceIndex == 0) currentPath = m_StreamingWidget->getCurrentFilePath();
    else if (m_currentWorkspaceIndex == 1) currentPath = m_fullLoadWidget->getCurrentFilePath();
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
        else m_btnLanguage->setText("语言");
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
        if (current == m_fullLoadWidget) {
            m_fullLoadWidget->setPlayheadPosition(seconds);
        } else if (current == m_StreamingWidget) {
            m_StreamingWidget->setPlayheadPosition(seconds);
        }
    });
    connect(m_playerControlBar, &PlayerControlBar::stateChanged, this, [this](PlayerController::State state){
        bool visible = (state != PlayerController::Stopped);
        QWidget* current = m_stackedWidget->currentWidget();
        if (current == m_fullLoadWidget) {
            m_fullLoadWidget->setPlayheadVisible(visible);
            if (state == PlayerController::Playing) {
                m_fullLoadWidget->tryAutoExpand(m_isTriggeredByExternalOpen);
                m_isTriggeredByExternalOpen = false;
            } else {
                m_fullLoadWidget->abortAutoExpand();
            }
        } else if (current == m_StreamingWidget) {
            m_StreamingWidget->setPlayheadVisible(visible);
            if (state == PlayerController::Playing) {
                m_StreamingWidget->tryAutoExpand(m_isTriggeredByExternalOpen);
                m_isTriggeredByExternalOpen = false;
            } else {
                m_StreamingWidget->abortAutoExpand();
            }
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
    m_StreamingWidget = new StreamingWidget(this); 
    m_fullLoadWidget = new FullLoadWidget(this);       
    m_batchFullLoadWidget = new BatchFullLoadWidget(this);                     
    m_stackedWidget->addWidget(m_StreamingWidget); 
    m_stackedWidget->addWidget(m_fullLoadWidget);    
    m_stackedWidget->addWidget(m_batchFullLoadWidget);           
    mainLayout->addWidget(topBarWidget);
    mainLayout->addWidget(m_stackedWidget, 1);
    setCentralWidget(centralWidget);

#ifdef Q_OS_WIN
    m_logWindow = new QWidget(nullptr, Qt::Window | Qt::Tool);
#else
    m_logWindow = new QWidget(nullptr, Qt::Window);
#endif

    m_logWindow->resize(AppConfig::LOG_AREA_DEFAULT_WIDTH, AppConfig::LOG_AREA_DEFAULT_HEIGHT);
    m_logWindow->setStyleSheet(qApp->styleSheet());
    auto logLayout = new QVBoxLayout(m_logWindow);
    logLayout->setContentsMargins(0,0,0,0);
    m_logView = new LogListView(m_logWindow);
    m_logModel = new LogListModel(this);
    m_logView->setModel(m_logModel);
    QFont logFont = QApplication::font(); 
    logFont.setPointSize(8);
    m_logView->setFont(logFont);
    m_logView->setStyleSheet(
        "QListView { background-color: rgb(78, 90, 110); color: rgb(217, 217, 217); border: none; outline: none; }"
        "QListView::item { padding: 1px 2px; }"
        "QListView::item:selected { background-color: #4A90E2; color: white; }"
    );
    logLayout->addWidget(m_logView);
    m_logWindow->installEventFilter(this);
    connect(m_fullLoadWidget, &FullLoadWidget::logMessageGenerated, this, &MainWindow::appendLogMessage);
    connect(m_fullLoadWidget->getShowLogCheckBox(), &QCheckBox::toggled, this, &MainWindow::toggleLogWindow);
    connect(m_fullLoadWidget, &FullLoadWidget::filePathChanged, this, &MainWindow::updateWindowTitle);
    connect(m_fullLoadWidget, &FullLoadWidget::playerProviderReady, this, &MainWindow::onPlayerProviderReady);
    connect(m_fullLoadWidget, &FullLoadWidget::seekRequested, m_playerControlBar, &PlayerControlBar::seek);
    connect(m_StreamingWidget, &StreamingWidget::logMessageGenerated, this, &MainWindow::appendLogMessage);
    connect(m_StreamingWidget->getShowLogCheckBox(), &QCheckBox::toggled, this, &MainWindow::toggleLogWindow);
    connect(m_StreamingWidget, &StreamingWidget::filePathChanged, this, &MainWindow::updateWindowTitle);
    connect(m_StreamingWidget, &StreamingWidget::playerProviderReady, this, &MainWindow::onPlayerProviderReady);
    connect(m_StreamingWidget, &StreamingWidget::seekRequested, m_playerControlBar, &PlayerControlBar::seek);
    connect(m_fullLoadWidget, &FullLoadWidget::mediaInfoChanged, m_playerControlBar, &PlayerControlBar::updateMediaInfo);
    connect(m_StreamingWidget, &StreamingWidget::mediaInfoChanged, m_playerControlBar, &PlayerControlBar::updateMediaInfo);
    m_logWindow->ensurePolished(); 
    m_logView->ensurePolished();
    m_currentWorkspaceIndex = -1; 
    GlobalPreferences prefs = GlobalPreferences::load();
    int startWs = prefs.defaultWorkspace;
    if (startWs < 0 || startWs > 1) startWs = 1;
    onWorkspaceChanged(startWs);
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
        currentFilePath = m_StreamingWidget->getCurrentFilePath();
        shouldShowLog = m_StreamingWidget->getShowLogCheckBox()->isChecked();
    } 
    else if (index == 1) {
        currentFilePath = m_fullLoadWidget->getCurrentFilePath();
        shouldShowLog = m_fullLoadWidget->getShowLogCheckBox()->isChecked();
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

void MainWindow::onPlayerProviderReady(QSharedPointer<XPlayerProvider> provider, double duration) {
    int sourceIndex = -1;
    if (sender() == m_StreamingWidget) sourceIndex = 0;
    else if (sender() == m_fullLoadWidget) sourceIndex = 1;
    if (sourceIndex != -1 && m_playerControlBar) {
        bool isActive = (sourceIndex == m_currentWorkspaceIndex);
        m_playerControlBar->setProvider(sourceIndex, provider, duration, isActive);
        if (m_pendingAutoPlay) {
            m_pendingAutoPlay = false;
            if (isActive && provider) {
                m_playerControlBar->play();
            }
        }
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
    QCheckBox* boxSingle = m_fullLoadWidget->getShowLogCheckBox();
    if (boxSingle->isChecked() != checked) {
        boxSingle->setChecked(checked); 
    }
    QCheckBox* boxStream = m_StreamingWidget->getShowLogCheckBox();
    if (boxStream->isChecked() != checked) {
        boxStream->setChecked(checked);
    }
}

void MainWindow::appendLogMessage(const QString& message) {
    if (message.isEmpty()) m_logModel->clear();
    else m_logModel->appendLog(message);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_logWindow) m_logWindow->close();
    QMainWindow::closeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (m_currentWorkspaceIndex == 2) {
        event->accept();
        return;
    }
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
    if (m_fullLoadWidget) {
        m_fullLoadWidget->updateCrosshairStyle(style, prefs.enableCrosshair);
        m_fullLoadWidget->setIndicatorVisibility(prefs.showCoordFreq, prefs.showCoordTime, prefs.showCoordDb);
        m_fullLoadWidget->updateSpectrumProfileStyle(
            prefs.showSpectrumProfile, 
            prefs.spectrumProfileColor, 
            prefs.spectrumProfileLineWidth,
            prefs.spectrumProfileFilled,
            prefs.spectrumProfileFillAlpha,
            prefs.spectrumProfileType,
            prefs.spectrumProfileDirection
        );
        m_fullLoadWidget->updatePlayheadStyle(phStyle);
        m_fullLoadWidget->setProfileFrameRate(prefs.profileFrameRate);
        m_fullLoadWidget->updateProbeConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        m_fullLoadWidget->setAutoExpandOnPlay(prefs.autoExpandOnPlay);
        if (!isRuntime) {
            m_fullLoadWidget->applyGlobalPreferences(prefs, true);
        }
    }
    if (m_StreamingWidget) {
        m_StreamingWidget->updateCrosshairStyle(style, prefs.enableCrosshair);
        m_StreamingWidget->setIndicatorVisibility(prefs.showCoordFreq, prefs.showCoordTime, prefs.showCoordDb);
        m_StreamingWidget->updateSpectrumProfileStyle(
            prefs.showSpectrumProfile, 
            prefs.spectrumProfileColor, 
            prefs.spectrumProfileLineWidth,
            prefs.spectrumProfileFilled,
            prefs.spectrumProfileFillAlpha,
            prefs.spectrumProfileType,
            prefs.spectrumProfileDirection
        );
        m_StreamingWidget->updatePlayheadStyle(phStyle);
        m_StreamingWidget->setProfileFrameRate(prefs.profileFrameRate);
        m_StreamingWidget->updateProbeConfig(prefs.spectrumSource, prefs.probeSource, prefs.probeDbPrecision);
        m_StreamingWidget->setAutoExpandOnPlay(prefs.autoExpandOnPlay);
        if (!isRuntime) {
            m_StreamingWidget->applyGlobalPreferences(prefs, true);
        }
    }
    if (m_playerControlBar) {
        m_playerControlBar->setPlayerFrameRate(prefs.playerFrameRate);
    }
    if (m_screenshotManager) {
        m_screenshotManager->updateSettings();
    }
}

void MainWindow::onNewLocalConnection() {
    if (!m_localServer) return;
    QLocalSocket* socket = m_localServer->nextPendingConnection();
    if (socket) {
        if (socket->waitForReadyRead(1000)) {
            QString filePath = QString::fromUtf8(socket->readAll());
            if (!filePath.isEmpty()) {
                setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
                activateWindow();
                raise();
                processExternalFile(filePath);
            }
        }
        socket->disconnectFromServer();
        socket->deleteLater();
    }
}

void MainWindow::processExternalFile(const QString& rawFilePath) {
    QString filePath = rawFilePath.trimmed();
    if (filePath.isEmpty()) return;
    QString resolvedPath = filePath;
    QUrl url(filePath);
    if (url.isValid() && url.isLocalFile()) {
        resolvedPath = url.toLocalFile();
    }
    if (m_currentWorkspaceIndex == 2) {
        if (m_batchFullLoadWidget && m_batchFullLoadWidget->isBusy()) {
            return;
        } else {
            GlobalPreferences prefs = GlobalPreferences::load();
            int defaultWs = prefs.defaultWorkspace;
            if (defaultWs < 0 || defaultWs > 1) defaultWs = 1; 
            onWorkspaceChanged(defaultWs);
        }
    }
    GlobalPreferences prefs = GlobalPreferences::load();
    if (m_currentWorkspaceIndex == 0 && m_StreamingWidget) {
        m_StreamingWidget->loadFile(resolvedPath);
    } else if (m_currentWorkspaceIndex == 1 && m_fullLoadWidget) {
        m_fullLoadWidget->loadFile(resolvedPath);
    }
    m_pendingAutoPlay = prefs.autoPlayOnExternalOpen;
    m_isTriggeredByExternalOpen = true;
}