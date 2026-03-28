#pragma once

#include <QObject>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;
class QPushButton;
class QDialog;
class QKeySequenceEdit;

class GlobalSettingsDialogUI {
public:
    GlobalSettingsDialogUI();
    ~GlobalSettingsDialogUI();
    void setupUi(QDialog* parent);
    QCheckBox*   m_cbLog = nullptr;
    QCheckBox*   m_cbGrid = nullptr;
    QCheckBox*   m_cbComponents = nullptr;
    QCheckBox*   m_cbZoom = nullptr;
    QCheckBox*   m_cbWidthLimit = nullptr;
    QSpinBox*    m_spinWidthVal = nullptr;
    QCheckBox*   m_cbGpuAccel = nullptr;
    QCheckBox*   m_cbCacheFft = nullptr;
    QSlider*     m_sliderPlayerFps = nullptr;
    QLabel*      m_lblPlayerFpsVal = nullptr;
    QSlider*     m_sliderProfileFps = nullptr;
    QLabel*      m_lblProfileFpsVal = nullptr;
    QComboBox*   m_cmbHeight = nullptr;
    QComboBox*   m_cmbPrecision = nullptr;
    QComboBox*   m_cmbWindow = nullptr;
    QComboBox*   m_cmbMapping = nullptr;
    QComboBox*   m_cmbPalette = nullptr;
    QComboBox*   m_cmbDbMax = nullptr;
    QComboBox*   m_cmbDbMin = nullptr;
    QCheckBox*   m_cbProfile = nullptr;
    QComboBox*   m_cmbProfileType = nullptr;
    QSlider*     m_sliderProfileWidth = nullptr;
    QLabel*      m_lblProfileWidthVal = nullptr;
    QPushButton* m_btnProfileColorPreview = nullptr;
    QCheckBox*   m_cbProfileFill = nullptr;
    QSlider*     m_sliderProfileAlpha = nullptr;
    QLabel*      m_lblProfileAlphaVal = nullptr;
    QCheckBox*   m_cbCrosshair = nullptr;
    QSlider*     m_sliderCrossLen = nullptr;
    QLabel*      m_lblCrossLenVal = nullptr;
    QSlider*     m_sliderCrossWidth = nullptr;
    QLabel*      m_lblCrossWidthVal = nullptr;
    QPushButton* m_btnColorPreview = nullptr;
    QCheckBox*   m_cbCoordFreq = nullptr;
    QCheckBox*   m_cbCoordTime = nullptr;
    QCheckBox*   m_cbCoordDb = nullptr;
    QComboBox*   m_cmbSpecSource = nullptr;
    QComboBox*   m_cmbProbeSource = nullptr;
    QSlider*     m_sliderProbePrecision = nullptr;
    QLabel*      m_lblProbePrecisionVal = nullptr;
    QCheckBox*   m_cbPlayheadVisible = nullptr;
    QSlider*     m_sliderPlayheadWidth = nullptr;
    QLabel*      m_lblPlayheadWidthVal = nullptr;
    QPushButton* m_btnPlayheadHandleColor = nullptr;
    QPushButton* m_btnPlayheadColor = nullptr;
    QPushButton* m_btnRestore = nullptr;
    QPushButton* m_btnOk = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QKeySequenceEdit* m_keyEditScreenshot1 = nullptr;
    QKeySequenceEdit* m_keyEditScreenshot2 = nullptr;
    QKeySequenceEdit* m_keyEditQuickCopy   = nullptr;
    QCheckBox*        m_cbHideCursor = nullptr;
    QCheckBox*        m_cbClipboard = nullptr;
};