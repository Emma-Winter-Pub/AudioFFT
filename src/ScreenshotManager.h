#pragma once

#include <QObject>
#include <QKeySequence>
#include <QWidget>
#include <QString>

class ScreenshotManager : public QObject {
    Q_OBJECT

public:
    explicit ScreenshotManager(QWidget* mainWindow, QObject* parent = nullptr);
    void updateSettings();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class CaptureMode {
        DialogAndOptionalClipboard,
        SilentClipboardOnly
    };
    void triggerScreenshot(CaptureMode mode);
    void performCapture(QWidget* target, CaptureMode mode);
    void drawMouseCursor(QPaintDevice* device, const QPoint& localPos);
    bool isKeyMatch(class QKeyEvent* event, const QKeySequence& sequence);
    QWidget* m_mainWindow;
    QKeySequence m_hotkey1;
    QKeySequence m_hotkey2;
    QKeySequence m_hotkeyQuick;
    bool m_hideCursor;
    bool m_copyToClipboard;
    QString m_lastSavePath;
};