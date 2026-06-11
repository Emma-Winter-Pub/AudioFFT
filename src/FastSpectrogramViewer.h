#pragma once

#include "SpectrogramViewerBase.h"
#include "FastConfig.h"

#include <QImage>

class FastSpectrogramViewer : public SpectrogramViewerBase {
    Q_OBJECT

public:
    explicit FastSpectrogramViewer(QWidget *parent = nullptr);
    void initCanvas(int estimatedWidth, int height);
    void appendChunk(const QImage& chunk);
    void reset();
    void setFinished();
    void setPaletteType(PaletteType type) override;
    const QImage& getFullSpectrogram() const { return m_canvas; }

protected:
    int getContentWidth() const override;
    int getContentHeight() const override;
    int getReferenceWidth() const override;
    const QImage* getCurrentImage() const override { return &m_canvas; }
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    int getColorIndex(const QPoint& viewPos) const override;

private:
    void updateColorTable();
    QImage m_canvas;
    int m_currentWriteX = 0;
    bool m_isProcessing = false;
};