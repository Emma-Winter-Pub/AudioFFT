#pragma once

#include "BatchFullLoadTypes.h"

#include <QDialog>
class QCheckBox;
class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;

class BatchFullLoadWidgetSettings : public QDialog {
    Q_OBJECT

public:
    explicit BatchFullLoadWidgetSettings(const BatchSettings& currentSettings, QWidget *parent = nullptr);
    BatchSettings getSettings() const;

private slots:
    void onDbMaxChanged(const QString& text);
    void onDbMinChanged(const QString& text);
    void onFormatChanged(int index);

private:
    void setupUi();
    void loadSettings(const BatchSettings& settings);
    QComboBox* m_cmbMode;
    QComboBox* m_cmbThreads;
    QCheckBox* m_cbInput;
    QCheckBox* m_cbOutput;
    QCheckBox* m_cbWhitelist;
    QCheckBox* m_cbExcludeVideo;
    QCheckBox* m_cbCategorizeByCodec;
    QPushButton* m_btnWhitelistConfig;
    QStringList m_currentWhitelist;
    QComboBox* m_cmbHeight;
    QComboBox* m_cmbPrecision;
    QComboBox* m_cmbWindow;
    QComboBox* m_cmbMapping;
    QComboBox* m_cmbColor;
    QCheckBox* m_cbColorInvert;
    QCheckBox* m_cbColorNegative;
    QComboBox* m_cmbDbMax;
    QComboBox* m_cmbDbMin;
    QCheckBox* m_cbGrid;
    QCheckBox* m_cbComponents;
    QCheckBox* m_cbWidth;
    QSpinBox*  m_spinWidth;
    QComboBox* m_cmbFormat;
    QLabel*    m_lblQualityTitle;
    QWidget*   m_qualityContainer;
    QSlider*   m_sliderQuality;
    QLabel*    m_lblQualityVal;
};