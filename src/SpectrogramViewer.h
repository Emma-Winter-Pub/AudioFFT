#pragma once

#include "SpectrogramViewerBase.h"

#include <QImage>

class SpectrogramViewer : public SpectrogramViewerBase {
    Q_OBJECT

public:
    explicit SpectrogramViewer(QWidget *parent = nullptr);
    void setSpectrogramImage(const QImage &image, bool resetView = true);
    void setLoadingState(bool loading);
    const QImage& getFullSpectrogram() const { return m_spectrogramImage; }

protected:
    int getContentWidth() const override;
    int getContentHeight() const override;
    const QImage* getCurrentImage() const override { return &m_spectrogramImage; }
    void paintEvent(QPaintEvent *event) override;
    int getColorIndex(const QPoint& viewPos) const override;

private:
    QImage m_spectrogramImage;
    bool m_isLoading = false;
};