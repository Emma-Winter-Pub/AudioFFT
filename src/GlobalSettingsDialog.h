#pragma once

#include "GlobalPreferences.h"

#include <QDialog>
#include <memory>

class GlobalSettingsDialogUI; 

class GlobalSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GlobalSettingsDialog(QWidget *parent = nullptr);
    ~GlobalSettingsDialog() override;

signals:
    void settingsChanged(const GlobalPreferences& prefs);

private slots:
    void onDbMaxChanged(const QString& text);
    void onDbMinChanged(const QString& text);
    void onColorPreviewClicked();
    void onProfileColorPreviewClicked();
    void onPlayheadColorPreviewClicked();
    void onPlayheadHandleColorPreviewClicked();
    void onPresetColorClicked(); 
    void onRestoreDefaults();
    void onOk();
    void onCancel();
    void onLengthSliderChanged(int val);
    void onProfileTypeChanged(int index);
    void onCacheFftToggled(bool checked);
    void onProbeSourceChanged(int index);
    void onProbePrecisionChanged(int val);

private:
    void setupConnections();
    void updateUiFromPrefs();
    void collectPrefsFromUi();
    void updateColorButton(class QPushButton* btn, const QColor& c);
    std::unique_ptr<GlobalSettingsDialogUI> ui;
    GlobalPreferences m_currentPrefs;
    QColor m_tempCrosshairColor;
    QColor m_tempProfileColor;
    QColor m_tempPlayheadColor;
    QColor m_tempPlayheadHandleColor;
};