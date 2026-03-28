#include "ScreenshotManager.h"
#include "GlobalPreferences.h"
#include "ScreenshotSaveDialog.h"
#include "ImageEncoderFactory.h"
#include "IImageEncoder.h"

#include <QApplication>
#include <QKeyEvent>
#include <QScreen>
#include <QWindow>
#include <QPixmap>
#include <QImage>
#include <QClipboard>
#include <QPainter>
#include <QMessageBox>
#include <QDir>
#include <QPolygon>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
#ifdef Q_OS_WIN
    QImage qt_fromWinHICON(HICON icon) {
        if (!icon) return QImage();
        ICONINFO ii;
        if (!GetIconInfo(icon, &ii)) return QImage();
        BITMAP bm;
        GetObject(ii.hbmMask, sizeof(bm), &bm);
        int w = bm.bmWidth;
        int h = bm.bmHeight;
        if (!ii.hbmColor) h /= 2;
        HDC screenDC = GetDC(NULL);
        HDC memDC = CreateCompatibleDC(screenDC);
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h; // Top-down
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HBITMAP hBitmap = CreateDIBSection(screenDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
        HGDIOBJ oldObj = SelectObject(memDC, hBitmap);
        memset(bits, 0, w * h * 4);
        DrawIconEx(memDC, 0, 0, icon, w, h, 0, NULL, DI_NORMAL);
        QImage result(static_cast<const uchar*>(bits), w, h, QImage::Format_ARGB32);
        result = result.copy(); 
        SelectObject(memDC, oldObj);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return result;
    }
#endif
}

ScreenshotManager::ScreenshotManager(QWidget* mainWindow, QObject* parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    qApp->installEventFilter(this);
    updateSettings();
}

void ScreenshotManager::updateSettings() {
    GlobalPreferences prefs = GlobalPreferences::load();
    m_hotkey1 = prefs.screenshotHotkey1;
    m_hotkey2 = prefs.screenshotHotkey2;
    m_hotkeyQuick = prefs.quickCopyHotkey;
    m_hideCursor = prefs.hideMouseCursor;
    m_copyToClipboard = prefs.copyToClipboard;
}

bool ScreenshotManager::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (!m_hotkey1.isEmpty() && isKeyMatch(keyEvent, m_hotkey1)) {
            triggerScreenshot(CaptureMode::DialogAndOptionalClipboard);
            return true;
        }
        if (!m_hotkey2.isEmpty() && isKeyMatch(keyEvent, m_hotkey2)) {
            triggerScreenshot(CaptureMode::DialogAndOptionalClipboard);
            return true;
        }
        if (!m_hotkeyQuick.isEmpty() && isKeyMatch(keyEvent, m_hotkeyQuick)) {
            triggerScreenshot(CaptureMode::SilentClipboardOnly);
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

bool ScreenshotManager::isKeyMatch(QKeyEvent* event, const QKeySequence& sequence) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QKeyCombination combo = event->keyCombination();
    return sequence.matches(combo) == QKeySequence::ExactMatch;
#else
    int keyInt = event->modifiers() | event->key();
    return sequence.matches(QKeySequence(keyInt)) == QKeySequence::ExactMatch;
#endif
}

void ScreenshotManager::triggerScreenshot(CaptureMode mode) {
    QWidget* target = QApplication::activeWindow();
    if (!target) {
        target = m_mainWindow;
    }
    if (target) {
        QMetaObject::invokeMethod(this, [this, target, mode](){
            performCapture(target, mode);
        }, Qt::QueuedConnection);
    }
}

void ScreenshotManager::performCapture(QWidget* target, CaptureMode mode) {
    if (!target) return;
    QScreen* screen = target->screen();
    if (!screen && target->windowHandle()) screen = target->windowHandle()->screen();
    if (!screen) screen = QApplication::primaryScreen();
    if (!screen) return;
    qreal dpr = screen->devicePixelRatio();
    QPoint logicalPos = target->mapToGlobal(QPoint(0, 0));
    int x = logicalPos.x();
    int y = logicalPos.y();
    int w = target->width();
    int h = target->height();
    QPixmap pixmap = screen->grabWindow(0, x, y, w, h);
    QImage image = pixmap.toImage();
    if (!m_hideCursor) {
#ifdef Q_OS_WIN
        CURSORINFO ci = {0};
        ci.cbSize = sizeof(CURSORINFO);
        if (GetCursorInfo(&ci) && (ci.flags & CURSOR_SHOWING)) {
            HICON hIcon = CopyIcon(ci.hCursor);
            if (hIcon) {
                QImage cursorImg = qt_fromWinHICON(hIcon);
                if (!cursorImg.isNull()) {
                    ICONINFO ii = {0};
                    if (GetIconInfo(hIcon, &ii)) {
                        int logicalCursorX = qRound(ci.ptScreenPos.x / dpr);
                        int logicalCursorY = qRound(ci.ptScreenPos.y / dpr);
                        int logicalHotX = qRound(ii.xHotspot / dpr);
                        int logicalHotY = qRound(ii.yHotspot / dpr);
                        int drawX = logicalCursorX - x - logicalHotX;
                        int drawY = logicalCursorY - y - logicalHotY;
                        cursorImg.setDevicePixelRatio(dpr);
                        QPainter p(&image);
                        p.drawImage(drawX, drawY, cursorImg);
                        if (ii.hbmColor) DeleteObject(ii.hbmColor);
                        if (ii.hbmMask) DeleteObject(ii.hbmMask);
                    }
                }
                DestroyIcon(hIcon);
            }
        }
#else
        QPoint globalMousePos = QCursor::pos();
        QPoint localMousePos = target->mapFromGlobal(globalMousePos);
        if (target->rect().contains(localMousePos)) {
            drawMouseCursor(&image, localMousePos);
        }
#endif
    }
    if (mode == CaptureMode::SilentClipboardOnly) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setImage(image);
        return; 
    }
    if (m_copyToClipboard) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setImage(image);
    }
    ScreenshotSaveDialog dialog(m_lastSavePath, true, true, true, target);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString outPath = dialog.getSelectedFilePath();
    m_lastSavePath = QFileInfo(outPath).path();
    QString fmtIdentifier = dialog.getSelectedFormatIdentifier();
    int quality = dialog.qualityLevel();
    auto encoder = ImageEncoderFactory::createEncoder(fmtIdentifier);
    if (encoder) {
        if (encoder->encodeAndSave(image, outPath, quality)) {
            QMessageBox::information(target, tr("成功"), tr("截图已保存至：\n%1").arg(outPath));
        } else {
            QMessageBox::critical(target, tr("失败"), tr("无法保存截图文件。"));
        }
    } else {
        QMessageBox::critical(target, tr("错误"), tr("找不到对应的编码器。"));
    }
}

void ScreenshotManager::drawMouseCursor(QPaintDevice* device, const QPoint& pos) {
    QPainter p(device);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(pos);
    QPolygon polygon;
    polygon << QPoint(0, 0) << QPoint(0, 16) << QPoint(4, 13) 
            << QPoint(7, 19) << QPoint(9, 18) << QPoint(6, 12) << QPoint(11, 12);
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::white);
    p.drawPolygon(polygon);
}