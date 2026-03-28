#pragma once

#include "AppConfig.h"
#include "ColorPalette.h"
#include "MappingCurves.h"
#include "WindowFunctions.h"
#include "CrosshairOverlay.h"
#include "GlobalPreferences.h"

#include <QWidget>
#include <QEvent>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;
class QToolButton;
class QFrame;
class QVBoxLayout;
class RibbonButton;

class SpectrogramWidgetBase : public QWidget {
    Q_OBJECT

public:
    explicit SpectrogramWidgetBase(QWidget *parent = nullptr);
    virtual ~SpectrogramWidgetBase();
    QCheckBox* getShowLogCheckBox() const { return m_checkLog; }
    void setBaseControlsEnabled(bool enabled);
    virtual void updateCrosshairStyle(const CrosshairStyle& style, bool enabled);
    virtual void updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type);
    virtual void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb);
    virtual void updatePlayheadStyle(const PlayheadStyle& style);
    virtual void setProfileFrameRate(int fps);
    virtual void updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision);
    void applyGlobalPreferences(const GlobalPreferences& prefs, bool silent = false);

protected:
    void setupBaseUI(); 
    virtual void retranslateBaseUi();
    virtual int getRequiredFftSize(int height) const;
    void updateAutoPrecisionText(); 
    QVBoxLayout* m_viewerLayout = nullptr;
    QToolButton* m_btnLog = nullptr;
    QToolButton* m_btnGrid = nullptr;
    QToolButton* m_btnComponents = nullptr;
    QToolButton* m_btnZoom = nullptr;
    QCheckBox*   m_checkLog = nullptr;
    RibbonButton* m_btnTrack = nullptr;
    RibbonButton* m_btnChannel = nullptr;
    RibbonButton* m_btnHeight = nullptr;
    RibbonButton* m_btnInterval = nullptr;
    RibbonButton* m_btnWindow = nullptr;
    RibbonButton* m_btnCurve = nullptr;
    RibbonButton* m_btnPalette = nullptr;
    RibbonButton* m_btndB = nullptr;
    RibbonButton* m_btnWidth = nullptr;
    QComboBox* m_comboDbMax = nullptr;
    QComboBox* m_comboDbMin = nullptr;
    QCheckBox* m_checkWidth = nullptr;
    QSpinBox*  m_spinWidth = nullptr;
    QPushButton* m_btnOpen = nullptr;
    QPushButton* m_btnSave = nullptr;
    int m_currentHeight = AppConfig::DEFAULT_SPECTROGRAM_HEIGHT;
    double m_currentInterval = 0.0; // Auto = 0.0
    bool m_isAutoPrecision = true;
    CurveType m_currentCurveType = CurveType::XX;
    PaletteType m_currentPaletteType = PaletteType::S01;
    WindowType m_currentWindowType = WindowType::Hann;
    double m_currentMinDb = AppConfig::DEFAULT_MIN_DB;
    double m_currentMaxDb = AppConfig::DEFAULT_MAX_DB;
    bool m_widthLimitEnabled = false;
    int m_widthLimitValue = 2000;
    QToolButton* createToggleButton(const QString& text);
    QFrame* createVLine();
    QString getWindowName(WindowType type);
    QString getCurveName(CurveType type);
    QString getPaletteName(PaletteType type);

protected:
    void changeEvent(QEvent *event) override;

signals:
    void parameterChanged();
    void gridVisibilityChanged(bool visible);
    void zoomToggleChanged(bool enabled);
    void componentsVisibilityChanged(bool visible);
    void curveSettingsChanged(CurveType type);
    void paletteSettingsChanged(PaletteType type);
    void dbSettingsChanged(double minDb, double maxDb);
    void openFileRequested();
    void saveFileRequested();

private slots:
    void onHeightActionTriggered(int height);
    void onIntervalActionTriggered(double interval);
    void onWindowActionTriggered(WindowType type);
    void onCurveActionTriggered(CurveType type);
    void onPaletteActionTriggered(PaletteType type);
    void onDbMaxChanged(const QString& text);
    void onDbMinChanged(const QString& text);
    void onWidthToggled(bool checked);
    void onWidthValueChanged(int value);
    void onAutoIntervalTriggered();
};