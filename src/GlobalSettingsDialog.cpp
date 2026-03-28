#include "GlobalSettingsDialog.h"
#include "GlobalSettingsDialogUI.h"
#include "AppConfig.h"

#include <QColorDialog>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QKeySequenceEdit>

GlobalSettingsDialog::GlobalSettingsDialog(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<GlobalSettingsDialogUI>())
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("设置"));
    m_currentPrefs = GlobalPreferences::load();
    m_tempCrosshairColor = m_currentPrefs.crosshairColor;
    m_tempProfileColor = m_currentPrefs.spectrumProfileColor;
    m_tempPlayheadColor = m_currentPrefs.playheadColor;
    m_tempPlayheadHandleColor = m_currentPrefs.playheadHandleColor;
    ui->setupUi(this); 
    setupConnections(); 
    updateUiFromPrefs();
    setMinimumWidth(550);
    adjustSize();
}

GlobalSettingsDialog::~GlobalSettingsDialog() {}

void GlobalSettingsDialog::setupConnections() {
    connect(ui->m_cmbDbMax, &QComboBox::textActivated, this, &GlobalSettingsDialog::onDbMaxChanged);
    connect(ui->m_cmbDbMin, &QComboBox::textActivated, this, &GlobalSettingsDialog::onDbMinChanged);
    connect(ui->m_cbCacheFft, &QCheckBox::toggled, this, &GlobalSettingsDialog::onCacheFftToggled);
    connect(ui->m_sliderCrossLen, &QSlider::valueChanged, this, &GlobalSettingsDialog::onLengthSliderChanged);
    connect(ui->m_btnColorPreview, &QPushButton::clicked, this, &GlobalSettingsDialog::onColorPreviewClicked);
    connect(ui->m_cmbProfileType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlobalSettingsDialog::onProfileTypeChanged);
    connect(ui->m_btnProfileColorPreview, &QPushButton::clicked, this, &GlobalSettingsDialog::onProfileColorPreviewClicked);
    connect(ui->m_cmbProbeSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlobalSettingsDialog::onProbeSourceChanged);
    connect(ui->m_sliderProbePrecision, &QSlider::valueChanged, this, &GlobalSettingsDialog::onProbePrecisionChanged);
    connect(ui->m_btnPlayheadColor, &QPushButton::clicked, this, &GlobalSettingsDialog::onPlayheadColorPreviewClicked);
    connect(ui->m_btnPlayheadHandleColor, &QPushButton::clicked, this, &GlobalSettingsDialog::onPlayheadHandleColorPreviewClicked);
    connect(ui->m_btnRestore, &QPushButton::clicked, this, &GlobalSettingsDialog::onRestoreDefaults);
    connect(ui->m_btnOk, &QPushButton::clicked, this, &GlobalSettingsDialog::onOk);
    connect(ui->m_btnCancel, &QPushButton::clicked, this, &GlobalSettingsDialog::onCancel);
    QList<QPushButton*> btns = this->findChildren<QPushButton*>();
    for(QPushButton* btn : btns) {
        if(btn->property("isPresetBtn").toBool()) {
            connect(btn, &QPushButton::clicked, this, &GlobalSettingsDialog::onPresetColorClicked);
        }
    }
}

void GlobalSettingsDialog::updateUiFromPrefs() {
    bool oldState = blockSignals(true);
    ui->m_cbLog->setChecked(m_currentPrefs.showLog);
    ui->m_cbGrid->setChecked(m_currentPrefs.showGrid);
    ui->m_cbComponents->setChecked(m_currentPrefs.showComponents);
    ui->m_cbZoom->setChecked(m_currentPrefs.enableZoom);
    ui->m_cbWidthLimit->setChecked(m_currentPrefs.enableWidthLimit);
    ui->m_spinWidthVal->setValue(m_currentPrefs.maxWidth);
    ui->m_spinWidthVal->setEnabled(m_currentPrefs.enableWidthLimit);
    int idx = ui->m_cmbHeight->findData(m_currentPrefs.height);
    if (idx >= 0) ui->m_cmbHeight->setCurrentIndex(idx);
    idx = ui->m_cmbPrecision->findData(m_currentPrefs.timeInterval);
    if (idx >= 0) ui->m_cmbPrecision->setCurrentIndex(idx);
    idx = ui->m_cmbWindow->findData(QVariant::fromValue(m_currentPrefs.windowType));
    if (idx >= 0) ui->m_cmbWindow->setCurrentIndex(idx);
    idx = ui->m_cmbMapping->findData(QVariant::fromValue(m_currentPrefs.curveType));
    if (idx >= 0) ui->m_cmbMapping->setCurrentIndex(idx);
    idx = ui->m_cmbPalette->findData(QVariant::fromValue(m_currentPrefs.paletteType));
    if (idx >= 0) ui->m_cmbPalette->setCurrentIndex(idx);
    ui->m_cmbDbMax->setCurrentText(QString::number((int)m_currentPrefs.maxDb));
    ui->m_cmbDbMin->setCurrentText(QString::number((int)m_currentPrefs.minDb));
    ui->m_cbCrosshair->setChecked(m_currentPrefs.enableCrosshair);
    ui->m_sliderCrossLen->setValue(m_currentPrefs.crosshairLength);
    ui->m_lblCrossLenVal->setNum(m_currentPrefs.crosshairLength);
    ui->m_sliderCrossWidth->setValue(m_currentPrefs.crosshairWidth);
    ui->m_lblCrossWidthVal->setNum(m_currentPrefs.crosshairWidth);
    m_tempCrosshairColor = m_currentPrefs.crosshairColor;
    updateColorButton(ui->m_btnColorPreview, m_tempCrosshairColor);
    ui->m_cbProfile->setChecked(m_currentPrefs.showSpectrumProfile);
    int typeIdx = ui->m_cmbProfileType->findData(QVariant::fromValue(m_currentPrefs.spectrumProfileType));
    if (typeIdx >= 0) ui->m_cmbProfileType->setCurrentIndex(typeIdx);
    onProfileTypeChanged(ui->m_cmbProfileType->currentIndex());
    ui->m_sliderProfileWidth->setValue(m_currentPrefs.spectrumProfileLineWidth);
    ui->m_lblProfileWidthVal->setNum(m_currentPrefs.spectrumProfileLineWidth);
    m_tempProfileColor = m_currentPrefs.spectrumProfileColor;
    updateColorButton(ui->m_btnProfileColorPreview, m_tempProfileColor);
    ui->m_cbProfileFill->setChecked(m_currentPrefs.spectrumProfileFilled);
    ui->m_sliderProfileAlpha->setValue(m_currentPrefs.spectrumProfileFillAlpha);
    ui->m_lblProfileAlphaVal->setNum(m_currentPrefs.spectrumProfileFillAlpha);
    ui->m_sliderProfileAlpha->setEnabled(m_currentPrefs.spectrumProfileFilled);
    ui->m_cbCoordFreq->setChecked(m_currentPrefs.showCoordFreq);
    ui->m_cbCoordTime->setChecked(m_currentPrefs.showCoordTime);
    ui->m_cbCoordDb->setChecked(m_currentPrefs.showCoordDb);
    ui->m_cbCacheFft->setChecked(m_currentPrefs.cacheFftData);
    ui->m_cbGpuAccel->setChecked(m_currentPrefs.enableGpuAcceleration);
    ui->m_sliderPlayerFps->setValue(m_currentPrefs.playerFrameRate);
    ui->m_lblPlayerFpsVal->setText(QString("%1 Hz").arg(m_currentPrefs.playerFrameRate));
    ui->m_sliderProfileFps->setValue(m_currentPrefs.profileFrameRate);
    ui->m_lblProfileFpsVal->setText(QString("%1 Hz").arg(m_currentPrefs.profileFrameRate));
    int specSrcIdx = ui->m_cmbSpecSource->findData(QVariant::fromValue(m_currentPrefs.spectrumSource));
    if (specSrcIdx >= 0) ui->m_cmbSpecSource->setCurrentIndex(specSrcIdx);
    int probeSrcIdx = ui->m_cmbProbeSource->findData(QVariant::fromValue(m_currentPrefs.probeSource));
    if (probeSrcIdx >= 0) ui->m_cmbProbeSource->setCurrentIndex(probeSrcIdx);
    int prec = m_currentPrefs.probeDbPrecision;
    int sliderIndex = 0;
    if (prec <= 0) sliderIndex = 0;
    else if (prec <= 7) sliderIndex = 1;
    else sliderIndex = 2;
    ui->m_sliderProbePrecision->setValue(sliderIndex);
    onProbePrecisionChanged(sliderIndex);
    ui->m_cbPlayheadVisible->setChecked(m_currentPrefs.playheadVisible);
    ui->m_sliderPlayheadWidth->setValue(m_currentPrefs.playheadLineWidth);
    ui->m_lblPlayheadWidthVal->setNum(m_currentPrefs.playheadLineWidth);
    m_tempPlayheadColor = m_currentPrefs.playheadColor;
    updateColorButton(ui->m_btnPlayheadColor, m_tempPlayheadColor);
    m_tempPlayheadHandleColor = m_currentPrefs.playheadHandleColor;
    updateColorButton(ui->m_btnPlayheadHandleColor, m_tempPlayheadHandleColor);
    ui->m_keyEditScreenshot1->setKeySequence(m_currentPrefs.screenshotHotkey1);
    ui->m_keyEditScreenshot2->setKeySequence(m_currentPrefs.screenshotHotkey2);
    ui->m_keyEditQuickCopy->setKeySequence(m_currentPrefs.quickCopyHotkey);
    ui->m_cbHideCursor->setChecked(m_currentPrefs.hideMouseCursor);
    ui->m_cbClipboard->setChecked(m_currentPrefs.copyToClipboard);
    onCacheFftToggled(m_currentPrefs.cacheFftData);
    onProbeSourceChanged(ui->m_cmbProbeSource->currentIndex());
    blockSignals(oldState);
}

void GlobalSettingsDialog::collectPrefsFromUi() {
    m_currentPrefs.showLog = ui->m_cbLog->isChecked();
    m_currentPrefs.showGrid = ui->m_cbGrid->isChecked();
    m_currentPrefs.showComponents = ui->m_cbComponents->isChecked();
    m_currentPrefs.enableZoom = ui->m_cbZoom->isChecked();
    m_currentPrefs.enableWidthLimit = ui->m_cbWidthLimit->isChecked();
    m_currentPrefs.maxWidth = ui->m_spinWidthVal->value();
    m_currentPrefs.height = ui->m_cmbHeight->currentData().toInt();
    m_currentPrefs.timeInterval = ui->m_cmbPrecision->currentData().toDouble();
    m_currentPrefs.windowType = ui->m_cmbWindow->currentData().value<WindowType>();
    m_currentPrefs.curveType = ui->m_cmbMapping->currentData().value<CurveType>();
    m_currentPrefs.paletteType = ui->m_cmbPalette->currentData().value<PaletteType>();
    m_currentPrefs.maxDb = ui->m_cmbDbMax->currentText().toDouble();
    m_currentPrefs.minDb = ui->m_cmbDbMin->currentText().toDouble();
    m_currentPrefs.enableCrosshair = ui->m_cbCrosshair->isChecked();
    m_currentPrefs.crosshairLength = ui->m_sliderCrossLen->value();
    m_currentPrefs.crosshairWidth = ui->m_sliderCrossWidth->value();
    m_currentPrefs.crosshairColor = m_tempCrosshairColor;
    m_currentPrefs.showSpectrumProfile = ui->m_cbProfile->isChecked();
    m_currentPrefs.spectrumProfileType = ui->m_cmbProfileType->currentData().value<SpectrumProfileType>();
    m_currentPrefs.spectrumProfileLineWidth = ui->m_sliderProfileWidth->value();
    m_currentPrefs.spectrumProfileColor = m_tempProfileColor;
    m_currentPrefs.spectrumProfileFilled = ui->m_cbProfileFill->isChecked();
    m_currentPrefs.spectrumProfileFillAlpha = ui->m_sliderProfileAlpha->value();
    m_currentPrefs.showCoordFreq = ui->m_cbCoordFreq->isChecked();
    m_currentPrefs.showCoordTime = ui->m_cbCoordTime->isChecked();
    m_currentPrefs.showCoordDb   = ui->m_cbCoordDb->isChecked();
    m_currentPrefs.cacheFftData = ui->m_cbCacheFft->isChecked();
    m_currentPrefs.enableGpuAcceleration = ui->m_cbGpuAccel->isChecked();
    m_currentPrefs.playerFrameRate = ui->m_sliderPlayerFps->value();
    m_currentPrefs.profileFrameRate = ui->m_sliderProfileFps->value();
    m_currentPrefs.spectrumSource = ui->m_cmbSpecSource->currentData().value<DataSourceType>();
    m_currentPrefs.probeSource = ui->m_cmbProbeSource->currentData().value<DataSourceType>();
    int idx = ui->m_sliderProbePrecision->value();
    if (idx == 0) m_currentPrefs.probeDbPrecision = 0;
    else if (idx == 1) m_currentPrefs.probeDbPrecision = 7;
    else m_currentPrefs.probeDbPrecision = 15;
    m_currentPrefs.playheadVisible = ui->m_cbPlayheadVisible->isChecked();
    m_currentPrefs.playheadLineWidth = ui->m_sliderPlayheadWidth->value();
    m_currentPrefs.playheadColor = m_tempPlayheadColor;
    m_currentPrefs.playheadHandleColor = m_tempPlayheadHandleColor;
    m_currentPrefs.screenshotHotkey1 = ui->m_keyEditScreenshot1->keySequence();
    m_currentPrefs.screenshotHotkey2 = ui->m_keyEditScreenshot2->keySequence();
    m_currentPrefs.quickCopyHotkey   = ui->m_keyEditQuickCopy->keySequence();
    m_currentPrefs.hideMouseCursor = ui->m_cbHideCursor->isChecked();
    m_currentPrefs.copyToClipboard = ui->m_cbClipboard->isChecked();
}

void GlobalSettingsDialog::onCacheFftToggled(bool checked) {
    if (!checked) {
        int imgIdxSpec = ui->m_cmbSpecSource->findData(QVariant::fromValue(DataSourceType::ImagePixel));
        if (imgIdxSpec >= 0) ui->m_cmbSpecSource->setCurrentIndex(imgIdxSpec);
        int imgIdxProbe = ui->m_cmbProbeSource->findData(QVariant::fromValue(DataSourceType::ImagePixel));
        if (imgIdxProbe >= 0) ui->m_cmbProbeSource->setCurrentIndex(imgIdxProbe);
        ui->m_cmbSpecSource->setEnabled(false);
        ui->m_cmbProbeSource->setEnabled(false);
    } else {
        ui->m_cmbSpecSource->setEnabled(true);
        ui->m_cmbProbeSource->setEnabled(true);
    }
}

void GlobalSettingsDialog::onProbeSourceChanged(int index) {
    auto src = ui->m_cmbProbeSource->itemData(index).value<DataSourceType>();
    if (src == DataSourceType::ImagePixel) {
        ui->m_sliderProbePrecision->setEnabled(false);
        ui->m_sliderProbePrecision->setValue(0);
    } else {
        ui->m_sliderProbePrecision->setEnabled(true);
        if (ui->m_sliderProbePrecision->value() == 0) {
            ui->m_sliderProbePrecision->setValue(2);
        }
    }
}

void GlobalSettingsDialog::onProbePrecisionChanged(int val) {
    if (val == 0) ui->m_lblProbePrecisionVal->setText(tr("整数"));
    else if (val == 1) ui->m_lblProbePrecisionVal->setText(tr("7 位有效数字"));
    else ui->m_lblProbePrecisionVal->setText(tr("15 位有效数字"));
}

void GlobalSettingsDialog::updateColorButton(QPushButton* btn, const QColor& c) {
    QString textColor = (c.lightness() > 128) ? "black" : "white";
    btn->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid #888; border-radius: 2px; min-width: 0px;"
    ).arg(c.name()).arg(textColor));
    btn->setText(c.name().toUpper());
}

void GlobalSettingsDialog::onLengthSliderChanged(int val) {
    int step = 50;
    int snapped = (val + step / 2) / step * step;
    if (snapped < 50) snapped = 50;
    if (snapped != val) {
        ui->m_sliderCrossLen->setValue(snapped);
    }
    ui->m_lblCrossLenVal->setNum(snapped);
}

void GlobalSettingsDialog::onDbMaxChanged(const QString& text) {
    bool ok; double newMax = text.toDouble(&ok); if(!ok) return;
    double currentMin = ui->m_cmbDbMin->currentText().toDouble();
    if (newMax <= currentMin) {
        double newMin = newMax - 1;
        if (newMin < -300) { newMin = -300; newMax = -299;
            ui->m_cmbDbMax->blockSignals(true); ui->m_cmbDbMax->setCurrentText(QString::number((int)newMax)); ui->m_cmbDbMax->blockSignals(false);
        }
        ui->m_cmbDbMin->blockSignals(true); ui->m_cmbDbMin->setCurrentText(QString::number((int)newMin)); ui->m_cmbDbMin->blockSignals(false);
    }
}

void GlobalSettingsDialog::onDbMinChanged(const QString& text) {
    bool ok; double newMin = text.toDouble(&ok); if(!ok) return;
    double currentMax = ui->m_cmbDbMax->currentText().toDouble();
    if (newMin >= currentMax) {
        double newMax = newMin + 1;
        if (newMax > 0) { newMax = 0; newMin = -1;
            ui->m_cmbDbMin->blockSignals(true); ui->m_cmbDbMin->setCurrentText(QString::number((int)newMin)); ui->m_cmbDbMin->blockSignals(false);
        }
        ui->m_cmbDbMax->blockSignals(true); ui->m_cmbDbMax->setCurrentText(QString::number((int)newMax)); ui->m_cmbDbMax->blockSignals(false);
    }
}

void GlobalSettingsDialog::onProfileTypeChanged(int index) {
    auto type = ui->m_cmbProfileType->itemData(index).value<SpectrumProfileType>();
    if (type == SpectrumProfileType::Bar) {
        ui->m_sliderProfileWidth->setEnabled(false);
        ui->m_cbProfileFill->setChecked(true);
        ui->m_cbProfileFill->setEnabled(false);
        ui->m_sliderProfileAlpha->setEnabled(true);
    } else {
        ui->m_sliderProfileWidth->setEnabled(true);
        ui->m_cbProfileFill->setEnabled(true);
        ui->m_sliderProfileAlpha->setEnabled(ui->m_cbProfileFill->isChecked());
    }
}

void GlobalSettingsDialog::onColorPreviewClicked() {
    QColor c = QColorDialog::getColor(m_tempCrosshairColor, this, tr("选择十字光标颜色"));
    if (c.isValid()) {
        m_tempCrosshairColor = c;
        updateColorButton(ui->m_btnColorPreview, c);
    }
}

void GlobalSettingsDialog::onProfileColorPreviewClicked() {
    QColor c = QColorDialog::getColor(m_tempProfileColor, this, tr("选择频率分布图颜色"));
    if (c.isValid()) {
        m_tempProfileColor = c;
        updateColorButton(ui->m_btnProfileColorPreview, c);
    }
}

void GlobalSettingsDialog::onPlayheadColorPreviewClicked() {
    QColor c = QColorDialog::getColor(m_tempPlayheadColor, this, tr("选择播放指针颜色"));
    if (c.isValid()) {
        m_tempPlayheadColor = c;
        updateColorButton(ui->m_btnPlayheadColor, c);
    }
}

void GlobalSettingsDialog::onPlayheadHandleColorPreviewClicked() {
    QColor c = QColorDialog::getColor(m_tempPlayheadHandleColor, this, tr("选择播放手柄颜色"));
    if (c.isValid()) {
        m_tempPlayheadHandleColor = c;
        updateColorButton(ui->m_btnPlayheadHandleColor, c);
    }
}

void GlobalSettingsDialog::onPresetColorClicked() {
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QColor c = btn->property("color").value<QColor>();
        QString target = btn->property("presetTarget").toString();
        if (target == "crosshair") {
            m_tempCrosshairColor = c;
            updateColorButton(ui->m_btnColorPreview, c);
        } else if (target == "profile") {
            m_tempProfileColor = c;
            updateColorButton(ui->m_btnProfileColorPreview, c);
        } else if (target == "playheadColor") {
            m_tempPlayheadColor = c;
            updateColorButton(ui->m_btnPlayheadColor, c);
        } else if (target == "playheadHandle") {
            m_tempPlayheadHandleColor = c;
            updateColorButton(ui->m_btnPlayheadHandleColor, c);
        }
    }
}

void GlobalSettingsDialog::onRestoreDefaults() {
    m_currentPrefs.resetToDefaults();
    m_tempCrosshairColor = m_currentPrefs.crosshairColor;
    m_tempProfileColor = m_currentPrefs.spectrumProfileColor;
    m_tempPlayheadColor = m_currentPrefs.playheadColor;
    m_tempPlayheadHandleColor = m_currentPrefs.playheadHandleColor;
    updateUiFromPrefs();
}

void GlobalSettingsDialog::onOk() {
    collectPrefsFromUi();
    GlobalPreferences::save(m_currentPrefs);
    emit settingsChanged(m_currentPrefs);
    accept();
}

void GlobalSettingsDialog::onCancel() {
    reject();
}