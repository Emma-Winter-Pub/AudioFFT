#pragma once

#include "BatTypes.h"

#include <QDialog>
class QCheckBox;
class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;

class BatWidgetSettings : public QDialog {
    Q_OBJECT

public:
    explicit BatWidgetSettings(const BatSettings& currentSettings, QWidget *parent = nullptr);
    BatSettings getSettings() const;

private slots:
    void onDbMaxChanged(const QString& text);
    void onDbMinChanged(const QString& text);
    void onFormatChanged(int index);

private:
    void setupUi();
    void loadSettings(const BatSettings& settings);
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