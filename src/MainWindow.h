#pragma once

#include "CrosshairConfig.h"
#include "GlobalPreferences.h"

#include <QMainWindow>
#include <QSharedPointer>
#include <QTimer>

class SingleFileWidget;
class BatWidget;
class FastStreamingWidget;
class BatProcessor;
class BatchStreamProcessor;
class RibbonButton;
class QStackedWidget;
class QTextEdit;
class QThread;
class QCheckBox;
class IPlayerProvider;
class QHBoxLayout;
class PlayerControlBar;
class QPushButton;
class ScreenshotManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& initialLangCode = "", QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void appendLogMessage(const QString& message);
    void toggleLogWindow(bool checked);
    void updateWindowTitle(const QString& filePath);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void showAboutDialog();
    void onWorkspaceChanged(int index);
    void onPlayerProviderReady(QSharedPointer<IPlayerProvider> provider, double duration);
    void onSwitchLanguage(const QString& langCode);
    void onLanguageTimerTimeout();
    void onGlobalSettingsClicked();
    void applyGlobalSettings(const GlobalPreferences& prefs, bool isRuntime = false);

private:
    void setupUI();
    void setupGlobalStyle();
    void retranslateUi();
    RibbonButton* m_btnWorkspace;
    RibbonButton* m_btnLanguage;
    QPushButton* m_btnAbout;
    QStackedWidget* m_stackedWidget;
    FastStreamingWidget* m_fastStreamingWidget; 
    SingleFileWidget *m_singleFileWidget;       
    BatWidget* m_batWidget;
    QWidget* m_logWindow;
    QTextEdit* m_logTextEdit;
    QThread* m_processorThread;
    BatProcessor* m_processor; 
    QThread* m_batchStreamThread;
    BatchStreamProcessor* m_batchStreamProcessor;
    PlayerControlBar* m_playerControlBar = nullptr;
    int m_currentWorkspaceIndex = -1; 
    QTimer* m_langCycleTimer = nullptr;
    int m_langCycleIndex = 0;
    QString m_currentLangCode;
    RibbonButton* m_btnGlobalSettings;
    ScreenshotManager* m_screenshotManager = nullptr;
};