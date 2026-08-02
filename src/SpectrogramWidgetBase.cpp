#include "SpectrogramWidgetBase.h"
#include "RibbonButton.h"
#include "GlobalPreferences.h"
#include "AppConfig.h"
#include "ColorPaletteFactory.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QApplication>
#include <QPainter>
#include <cmath>

namespace {
    class PaletteMenuButton : public QPushButton {
    public:
        QString textLabel;
        QPixmap colorBar;
        PaletteMenuButton(const QString& t, const QPixmap& pm, QWidget* parent = nullptr) 
            : QPushButton(parent), textLabel(t), colorBar(pm) {
            setMinimumHeight(26); 
        }
        QSize sizeHint() const override {
            QSize base = QPushButton::sizeHint();
            QFontMetrics fm(font());
            int textW = fm.horizontalAdvance(textLabel);
            return QSize(textW + 140, std::max(base.height(), 26));
        }
    protected:
        void paintEvent(QPaintEvent* event) override {
            QPushButton::paintEvent(event); 
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, false);
            QRect textR = rect().adjusted(15, 0, -120, 0); 
            p.setPen(palette().color(QPalette::ButtonText)); 
            p.drawText(textR, Qt::AlignLeft | Qt::AlignVCenter, textLabel);
            QRect iconR(rect().right() - 115, rect().top() + (rect().height() - 16) / 2, 100, 16);
            p.drawPixmap(iconR.topLeft(), colorBar);
        }
    };
}

SpectrogramWidgetBase::SpectrogramWidgetBase(QWidget *parent):QWidget(parent) {}

SpectrogramWidgetBase::~SpectrogramWidgetBase() {}

void SpectrogramWidgetBase::setupBaseUI() {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(2, 2, 2, 2);
    toolbarLayout->setSpacing(1);
    m_checkLog = new QCheckBox(this); 
    m_checkLog->hide(); 
    QString gridMenuStyle = R"(
        QWidget { background-color: #2F3642; }
        QPushButton { border: 1px solid transparent; color: #D9D9D9; background-color: transparent; padding: 4px 8px; font-size: 11px; text-align: left; min-width: 40px; }
        QPushButton:hover { background-color: #4A90E2; color: white; border-radius: 2px; }
    )";
    QString heightMenuStyle = R"(
        QWidget { background-color: #2F3642; }
        QPushButton { border: 1px solid transparent; color: #D9D9D9; background-color: transparent; padding: 4px 8px; font-size: 11px; min-width: 30px; }
        QPushButton:hover { background-color: #4A90E2; color: white; border-radius: 2px; }
    )";
    QString menuWidgetStyle = R"(
        QWidget { background-color: #2F3642; } 
        QLabel { color: rgb(217, 217, 217); font-size: 9pt; }
        QComboBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 3px; }
        QComboBox QAbstractItemView { background-color: rgb(78, 90, 110); selection-background-color: #5A687A; color: rgb(217, 217, 217); border: 1px solid #2B333E; }
        QSpinBox { background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: rgb(217, 217, 217); padding-left: 1px; }
        QCheckBox { color: rgb(217, 217, 217); spacing: 2px; }
    )";

    // --- 2. 基础切换按钮 ---
    m_btnLog = createToggleButton("");
    m_btnGrid = createToggleButton("");
    m_btnComponents = createToggleButton("");
    m_btnZoom = createToggleButton("");
    m_btnLog->setFocusPolicy(Qt::NoFocus);
    m_btnGrid->setFocusPolicy(Qt::NoFocus);
    m_btnComponents->setFocusPolicy(Qt::NoFocus);
    m_btnZoom->setFocusPolicy(Qt::NoFocus);
    m_btnGrid->setChecked(false);
    m_btnComponents->setChecked(true);
    connect(m_btnLog, &QToolButton::toggled, m_checkLog, &QCheckBox::setChecked);
    connect(m_checkLog, &QCheckBox::toggled, m_btnLog, &QToolButton::setChecked);
    connect(m_btnGrid, &QToolButton::toggled, this, &SpectrogramWidgetBase::gridVisibilityChanged);
    connect(m_btnComponents, &QToolButton::toggled, this, &SpectrogramWidgetBase::componentsVisibilityChanged);
    connect(m_btnZoom, &QToolButton::toggled, this, &SpectrogramWidgetBase::zoomToggleChanged);
    toolbarLayout->addWidget(m_btnLog); 
    toolbarLayout->addSpacing(1); 
    toolbarLayout->addWidget(m_btnGrid); 
    toolbarLayout->addSpacing(1); 
    toolbarLayout->addWidget(m_btnComponents);
    toolbarLayout->addSpacing(1); 
    toolbarLayout->addWidget(m_btnZoom); 
    
    // --- 3. 限宽设置 ---
    toolbarLayout->addSpacing(1);
    m_btnWidth = new RibbonButton("", this);
    m_btnWidth->setFocusPolicy(Qt::NoFocus);
    m_btnWidth->setCheckable(true);
    m_btnWidth->setChecked(m_widthLimitEnabled);
    QMenu* menuWidth = new QMenu(this);
    QWidgetAction* actionWidth = new QWidgetAction(menuWidth);
    QWidget* widgetWidth = new QWidget(); 
    widgetWidth->setStyleSheet(menuWidgetStyle);
    QHBoxLayout* lWidth = new QHBoxLayout(widgetWidth); 
    lWidth->setContentsMargins(4,4,4,4);
    m_checkWidth = new QCheckBox(); 
    m_checkWidth->setChecked(m_widthLimitEnabled);
    m_spinWidth = new QSpinBox(); 
    m_spinWidth->setRange(500, 10000); 
    m_spinWidth->setValue(m_widthLimitValue); 
    m_spinWidth->setEnabled(m_widthLimitEnabled);
    m_spinWidth->setFixedWidth(100);
    lWidth->addWidget(m_checkWidth); 
    lWidth->addWidget(m_spinWidth);
    actionWidth->setDefaultWidget(widgetWidth); 
    menuWidth->addAction(actionWidth);
    m_btnWidth->setMenu(menuWidth);
    connect(m_checkWidth, &QCheckBox::toggled, this, &SpectrogramWidgetBase::onWidthToggled);
    connect(m_spinWidth, &QSpinBox::valueChanged, this, &SpectrogramWidgetBase::onWidthValueChanged);
    toolbarLayout->addWidget(m_btnWidth);

    // --- 4. 分隔线 ---
    toolbarLayout->addWidget(createVLine());

    // --- 5. 之后的按钮 (音轨、声道等) ---
    m_btnTrack = new RibbonButton("", this); 
    m_btnTrack->setFocusPolicy(Qt::NoFocus); 
    m_btnTrack->setMenu(new QMenu(this)); 
    toolbarLayout->addWidget(m_btnTrack);
    m_btnChannel = new RibbonButton("", this); 
    m_btnChannel->setFocusPolicy(Qt::NoFocus); 
    m_btnChannel->setMenu(new QMenu(this)); 
    toolbarLayout->addWidget(m_btnChannel);

    // --- 高度按钮 ---
    m_btnHeight = new RibbonButton("", this);
    m_btnHeight->setFocusPolicy(Qt::NoFocus);
    QMenu* menuHeight = new QMenu(this);
    QWidgetAction* actionHeightGrid = new QWidgetAction(menuHeight);
    QWidget* widgetHeightGrid = new QWidget(); 
    widgetHeightGrid->setStyleSheet(heightMenuStyle);
    QGridLayout* gridHeight = new QGridLayout(widgetHeightGrid); 
    gridHeight->setContentsMargins(4, 4, 4, 4); 
    gridHeight->setSpacing(1);
    const std::vector<int> specialHeights = { 16385, 8193, 4097, 2049, 1025, 513, 257, 129, 65, 33 };
    for (int i = 0; i < (int)specialHeights.size(); ++i) {
        QPushButton* btn = new QPushButton(QString::number(specialHeights[i])); 
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [=](){ onHeightActionTriggered(specialHeights[i]); menuHeight->close(); }); 
        gridHeight->addWidget(btn, i, 0); 
    }
    int h_count = 0;
    const int H_ROWS = 10;
    for (int h = 8000; h >= 100; h -= 100) {
        QPushButton* btn = new QPushButton(QString::number(h)); 
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [=](){ onHeightActionTriggered(h); menuHeight->close(); });
        int row = h_count % H_ROWS; int col = (h_count / H_ROWS) + 1; 
        gridHeight->addWidget(btn, row, col); 
        h_count++;
    }
    actionHeightGrid->setDefaultWidget(widgetHeightGrid); 
    menuHeight->addAction(actionHeightGrid);
    m_btnHeight->setMenu(menuHeight); 
    toolbarLayout->addWidget(m_btnHeight);

    // --- 精度按钮 ---
    m_btnInterval = new RibbonButton("", this);
    m_btnInterval->setFocusPolicy(Qt::NoFocus);
    QMenu* menuInterval = new QMenu(this);
    QAction* actAuto = menuInterval->addAction(tr("自动"));
    connect(actAuto, &QAction::triggered, this, &SpectrogramWidgetBase::onAutoIntervalTriggered);
    menuInterval->addSeparator();
    const QList<double> intervals = { 1.0, 0.5, 0.25, 0.2, 0.1, 0.05, 0.025, 0.02, 0.01, 0.005, 0.0025, 0.002, 0.001 };
    for(double val : intervals) {
        QAction* act = menuInterval->addAction(QString::number(val));
        connect(act, &QAction::triggered, this, [=](){ 
            onIntervalActionTriggered(val); 
        });
    }
    m_btnInterval->setMenu(menuInterval); 
    toolbarLayout->addWidget(m_btnInterval);

    // --- 窗口函数按钮 ---
    m_btnWindow = new RibbonButton("", this);
    m_btnWindow->setFocusPolicy(Qt::NoFocus);
    QMenu* menuWindow = new QMenu(this);
    QWidgetAction* actionWindowGrid = new QWidgetAction(menuWindow);
    QWidget* widgetWindowGrid = new QWidget(); 
    widgetWindowGrid->setStyleSheet(gridMenuStyle);
    QGridLayout* gridWindow = new QGridLayout(widgetWindowGrid);
    gridWindow->setContentsMargins(4, 4, 4, 4); 
    gridWindow->setSpacing(1);
    struct WinItem { FFTWindowType t; double p; };
    std::vector<WinItem> winItems = {
        {FFTWindowType::Rectangular, 0.0},
        {FFTWindowType::Triangular, 0.0},
        {FFTWindowType::Hann, 0.0},
        {FFTWindowType::Hamming, 0.0},
        {FFTWindowType::Blackman, 0.0},
        {FFTWindowType::BlackmanHarris, 0.0},
        {FFTWindowType::FlatTop, 0.0},
        {FFTWindowType::Sine, 0.0},
        {FFTWindowType::Cauchy, 0.0},
        {FFTWindowType::Parzen, 0.0},
        {FFTWindowType::Poisson, 0.0},
        {FFTWindowType::Bohman, 0.0},
        {FFTWindowType::Nuttall, 0.0},
        {FFTWindowType::Lanczos, 0.0},
        {FFTWindowType::Welch, 0.0},
        {FFTWindowType::DolphChebyshev, 0.0},
        {FFTWindowType::BartlettHann, 0.0},
        {FFTWindowType::Minimum4Term, 0.0},
        {FFTWindowType::Minimum7Term, 0.0},
        {FFTWindowType::Gaussian, 0.0},
        {FFTWindowType::Kaiser, 0.0},
        {FFTWindowType::Tukey, 0.0}
    };
    const int W_ROWS = 12;
    for(int i=0; i < (int)winItems.size(); ++i) {
        FFTWindowType t = winItems[i].t; 
        QString name = FFTWindowFunctions::getName(t, winItems[i].p);
        QPushButton* btn = new QPushButton(name); 
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [=](){
            onWindowActionTriggered(t);
            menuWindow->close();
        });
        int row = i % W_ROWS; int col = i / W_ROWS; 
        gridWindow->addWidget(btn, row, col);
    }
    actionWindowGrid->setDefaultWidget(widgetWindowGrid); 
    menuWindow->addAction(actionWindowGrid);
    m_btnWindow->setMenu(menuWindow); 
    toolbarLayout->addWidget(m_btnWindow);

    // --- 映射曲线 ---
    m_btnCurve = new RibbonButton(tr("映射:F01"), this);
    m_btnCurve->setFocusPolicy(Qt::NoFocus);
    QMenu* menuCurve = new QMenu(this);
    QWidgetAction* actionCurveList = new QWidgetAction(menuCurve);
    QScrollArea* scrollArea = new QScrollArea();
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(0, 4, 0, 4);
    scrollLayout->setSpacing(0);
    QList<CurveInfo> curves = MappingCurves::getAllCurves();
    for (int i = 0; i < curves.size(); ++i) {
        const CurveInfo& info = curves[i];
        QPushButton* btn = new QPushButton(info.displayText);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { text-align: left; padding: 5px 20px; border: none; background: transparent; color: #D9D9D9; }"
            "QPushButton:hover { background-color: #4A90E2; color: white; }"
        );
        connect(btn, &QPushButton::clicked, this, [=](){ 
            onCurveActionTriggered(info.type); 
            menuCurve->close();
        });
        scrollLayout->addWidget(btn);
        if (info.hasSeparator && i < curves.size() - 1) {
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Plain);
            line->setFixedHeight(1);
            line->setStyleSheet("background-color: #454D5E; margin: 3px 6px;");
            scrollLayout->addWidget(line);
        }
    }
    scrollWidget->setStyleSheet("background-color: #2F3642;");
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(R"(
        QScrollArea { border: none; background-color: #2F3642; }
        QScrollBar:vertical { border: none; background: #222222; width: 10px; margin: 0px; }
        QScrollBar::handle:vertical { background: #555555; min-height: 20px; border-radius: 5px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");
    actionCurveList->setDefaultWidget(scrollArea);
    menuCurve->addAction(actionCurveList);
    m_btnCurve->setMenu(menuCurve); 
    toolbarLayout->addWidget(m_btnCurve);

    // --- 配色按钮 ---
    m_btnPalette = new RibbonButton("", this);
    m_btnPalette->setFocusPolicy(Qt::NoFocus);
    QMenu* menuPalette = new QMenu(this);
    QWidgetAction* actionInvert = new QWidgetAction(menuPalette);
    m_checkPaletteInvert = new QCheckBox(tr("颜色反向"), menuPalette);
    m_checkPaletteInvert->setStyleSheet("color: #D9D9D9; margin: 4px 12px;");
    m_checkPaletteInvert->setChecked(m_paletteInverted);
    connect(m_checkPaletteInvert, &QCheckBox::toggled, this, [=](bool checked){
        m_paletteInverted = checked;
        emit paletteSettingsChanged(m_currentPaletteId, m_paletteInverted, m_paletteNegative);
    });
    actionInvert->setDefaultWidget(m_checkPaletteInvert);
    menuPalette->addAction(actionInvert);
    QWidgetAction* actionNegative = new QWidgetAction(menuPalette);
    m_checkPaletteNegative = new QCheckBox(tr("颜色反相"), menuPalette);
    m_checkPaletteNegative->setStyleSheet("color: #D9D9D9; margin: 4px 12px;");
    m_checkPaletteNegative->setChecked(m_paletteNegative);
    connect(m_checkPaletteNegative, &QCheckBox::toggled, this, [=](bool checked){
        m_paletteNegative = checked;
        emit paletteSettingsChanged(m_currentPaletteId, m_paletteInverted, m_paletteNegative);
    });
    actionNegative->setDefaultWidget(m_checkPaletteNegative);
    menuPalette->addAction(actionNegative);
    menuPalette->addSeparator();
    auto createPaletteIcon = [](const QString& id) -> QIcon {
        QPixmap pixmap(100, 16);
        QPainter p(&pixmap);
        const auto& colors = ColorPaletteFactory::instance().getPalette(id, false);
        for(int x = 0; x < 100; ++x) {
            int idx = x * 255 / 99;
            p.setPen(QColor(colors[idx]));
            p.drawLine(x, 0, x, 16);
        }
        return QIcon(pixmap);
    };
    QWidgetAction* actionPaletteList = new QWidgetAction(menuPalette);
    QScrollArea* scrollAreaPal = new QScrollArea();
    QWidget* scrollWidgetPal = new QWidget();
    QVBoxLayout* scrollLayoutPal = new QVBoxLayout(scrollWidgetPal);
    scrollLayoutPal->setContentsMargins(0, 4, 0, 4);
    scrollLayoutPal->setSpacing(0);
    auto palettes = ColorPaletteFactory::instance().getAvailablePalettes();
    for (const auto& p : palettes) {
        PaletteMenuButton* btn = new PaletteMenuButton(
            QString("%1    %2").arg(p.first).arg(p.second),
            createPaletteIcon(p.first).pixmap(QSize(100, 16))
        );
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { text-align: left; padding: 5px 15px; border: none; background: transparent; color: #D9D9D9; }"
            "QPushButton:hover { background-color: #4A90E2; color: white; }"
        );
        connect(btn, &QPushButton::clicked, this, [=](){ 
            onPaletteActionTriggered(p.first); 
            menuPalette->close(); 
        });
        scrollLayoutPal->addWidget(btn);
    }
    scrollWidgetPal->setStyleSheet("background-color: #2F3642;");
    scrollAreaPal->setWidget(scrollWidgetPal);
    scrollAreaPal->setWidgetResizable(true);
    scrollAreaPal->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollAreaPal->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollAreaPal->setStyleSheet("QScrollArea { border: none; background-color: #2F3642; }"
                                 "QScrollBar:vertical { border: none; background: #222222; width: 10px; }"
                                 "QScrollBar::handle:vertical { background: #555555; min-height: 20px; border-radius: 5px; }");
    actionPaletteList->setDefaultWidget(scrollAreaPal);
    menuPalette->addAction(actionPaletteList);
    m_btnPalette->setMenu(menuPalette); 
    toolbarLayout->addWidget(m_btnPalette);

    // --- dB 设置 ---
    m_btndB = new RibbonButton("", this);
    m_btndB->setFocusPolicy(Qt::NoFocus);
    QMenu* menudB = new QMenu(this);
    QWidgetAction* actiondB = new QWidgetAction(menudB);
    QWidget* widgetdB = new QWidget(); 
    widgetdB->setStyleSheet(menuWidgetStyle);
    QHBoxLayout* ldB = new QHBoxLayout(widgetdB); 
    ldB->setContentsMargins(4,4,4,4);
    m_comboDbMax = new QComboBox(); 
    m_comboDbMax->setFixedWidth(60);
    for(int i=0; i>-300; --i) m_comboDbMax->addItem(QString::number(i));
    m_comboDbMax->setCurrentText(QString::number((int)m_currentMaxDb));
    m_comboDbMin = new QComboBox(); 
    m_comboDbMin->setFixedWidth(60);
    for(int i=-1; i>=-300; --i) m_comboDbMin->addItem(QString::number(i));
    m_comboDbMin->setCurrentText(QString::number((int)m_currentMinDb));
    ldB->addWidget(new QLabel(tr("上限:"))); 
    ldB->addWidget(m_comboDbMax);
    ldB->addSpacing(5);
    ldB->addWidget(new QLabel(tr("下限:"))); 
    ldB->addWidget(m_comboDbMin);
    actiondB->setDefaultWidget(widgetdB); 
    menudB->addAction(actiondB);
    m_btndB->setMenu(menudB);
    connect(m_comboDbMax, &QComboBox::textActivated, this, &SpectrogramWidgetBase::onDbMaxChanged);
    connect(m_comboDbMin, &QComboBox::textActivated, this, &SpectrogramWidgetBase::onDbMinChanged);
    toolbarLayout->addWidget(m_btndB);

    // --- 6. 分隔线 ---
    toolbarLayout->addWidget(createVLine());
    toolbarLayout->addSpacing(3);

    // --- 7. 打开/保存按钮 ---
    QString actionBtnStyle = "QPushButton { background-color: #4E5A6E; border: 1px solid #2B333E; border-radius: 0px; padding: 1px 8px; min-height: 18px; color: #D9D9D9; } QPushButton:hover { background-color: #5A687A; } QPushButton:disabled { background-color: #3A4250; color: #777; }";
    m_btnOpen = new QPushButton();
    m_btnOpen->setStyleSheet(actionBtnStyle);
    m_btnSave = new QPushButton();
    m_btnSave->setStyleSheet(actionBtnStyle); 
    m_btnSave->setEnabled(false);
    m_btnOpen->setFocusPolicy(Qt::NoFocus);
    m_btnSave->setFocusPolicy(Qt::NoFocus);
    connect(m_btnOpen, &QPushButton::clicked, this, &SpectrogramWidgetBase::openFileRequested);
    connect(m_btnSave, &QPushButton::clicked, this, &SpectrogramWidgetBase::saveFileRequested);
    toolbarLayout->addWidget(m_btnOpen); 
    toolbarLayout->addSpacing(4); 
    toolbarLayout->addWidget(m_btnSave); 
    toolbarLayout->addSpacing(3);
    toolbarLayout->addStretch();
    layout->addLayout(toolbarLayout);
    m_viewerLayout = new QVBoxLayout();
    m_viewerLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(m_viewerLayout, 1);
}

void SpectrogramWidgetBase::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateBaseUi();
    }
    QWidget::changeEvent(event);
}

void SpectrogramWidgetBase::retranslateBaseUi() {
    m_btnLog->setText(tr("日志"));
    m_btnGrid->setText(tr("网格"));
    m_btnComponents->setText(tr("组件")); 
    m_btnZoom->setText(tr("缩放频率"));
    m_btnHeight->setText(QString(tr("高度:%1")).arg(m_currentHeight));
    if (m_isAutoPrecision) {
        m_btnInterval->setText(tr("精度:自动"));
    } else {
        m_btnInterval->setText(QString(tr("精度:%1")).arg(m_currentInterval));
    }
    QString winName = getWindowName(m_currentWindowType);
    m_btnWindow->setText(tr("窗口:") + winName);
    QString curveName = getCurveName(m_currentCurveType);
    m_btnCurve->setText(tr("映射:") + curveName);
    m_btnPalette->setText(tr("配色:") + m_currentPaletteId);
    m_btndB->setText(QString(tr("dB:%1~%2")).arg(m_currentMaxDb).arg(m_currentMinDb));
    m_btnWidth->setText(QString(tr("限宽:%1")).arg(m_widthLimitValue));
    m_checkWidth->setText(tr("启用"));
    m_btnOpen->setText(tr("打开"));
    m_btnSave->setText(tr("保存"));
}

void SpectrogramWidgetBase::setBaseControlsEnabled(bool enabled) {
    m_btnOpen->setEnabled(true);
    m_btnTrack->setEnabled(enabled);
    m_btnChannel->setEnabled(enabled);
    m_btnHeight->setEnabled(enabled);
    m_btnInterval->setEnabled(enabled);
    m_btnWindow->setEnabled(enabled);
    m_btnCurve->setEnabled(enabled);
    m_btndB->setEnabled(enabled);
    m_btnPalette->setEnabled(enabled);
    m_btnSave->setEnabled(enabled);
}

QToolButton* SpectrogramWidgetBase::createToggleButton(const QString& text) {
    QToolButton* btn = new QToolButton(this);
    btn->setText(text);
    btn->setCheckable(true);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    btn->setFixedHeight(22);
    return btn;
}

QFrame* SpectrogramWidgetBase::createVLine() {
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Plain);
    line->setLineWidth(1);
    line->setFixedHeight(22);
    line->setStyleSheet("color: #222831; background-color: #222831; margin-left: 2px; margin-right: 2px;");
    return line;
}

QString SpectrogramWidgetBase::getWindowName(FFTWindowType type) {
    QString full = FFTWindowFunctions::getName(type);
    if (full.contains(u'(')) return full.split(u'(')[0];
    return full;
}

QString SpectrogramWidgetBase::getCurveName(CurveType type) {
    QString fullName = MappingCurves::getName(type);
    int spaceIndex = fullName.indexOf(' ');
    if (spaceIndex != -1) return fullName.left(spaceIndex);
    return fullName;
}

int SpectrogramWidgetBase::getRequiredFftSize(int height) const {
    for (int size : AppConfig::FFT_SIZE_OPTIONS) {
        if (height <= (size / 2 + 1)) return size;
    }
    return AppConfig::FFT_SIZE_OPTIONS.back();
}

void SpectrogramWidgetBase::updateAutoPrecisionText() {
    if (m_isAutoPrecision) m_btnInterval->setText(tr("精度:自动"));
}

void SpectrogramWidgetBase::onHeightActionTriggered(int height) {
    if (m_currentHeight == height) return;
    m_currentHeight = height;
    m_btnHeight->setText(QString(tr("高度:%1")).arg(height));
    if (m_isAutoPrecision) updateAutoPrecisionText();
    emit parameterChanged();
}

void SpectrogramWidgetBase::onAutoIntervalTriggered() {
    if (m_isAutoPrecision) return;
    m_isAutoPrecision = true;
    updateAutoPrecisionText();
    emit parameterChanged();
}

void SpectrogramWidgetBase::onIntervalActionTriggered(double interval) {
    if (!m_isAutoPrecision && std::abs(interval - m_currentInterval) < 1e-9) return;
    m_isAutoPrecision = false;
    m_currentInterval = interval;
    m_btnInterval->setText(QString(tr("精度:%1")).arg(interval));
    emit parameterChanged();
}

void SpectrogramWidgetBase::onWindowActionTriggered(FFTWindowType type) {
    if (m_currentWindowType == type) return;
    m_currentWindowType = type;
    m_btnWindow->setText(tr("窗口:") + getWindowName(type));
    emit parameterChanged();
}

void SpectrogramWidgetBase::onCurveActionTriggered(CurveType type) {
    if (m_currentCurveType == type) return;
    m_currentCurveType = type;
    m_btnCurve->setText(tr("映射:") + getCurveName(type));
    emit curveSettingsChanged(type);
    emit parameterChanged();
}

void SpectrogramWidgetBase::onPaletteActionTriggered(const QString& paletteId) {
    if (m_currentPaletteId == paletteId) return;
    m_currentPaletteId = paletteId;
    m_btnPalette->setText(tr("配色:") + m_currentPaletteId);
    emit paletteSettingsChanged(m_currentPaletteId, m_paletteInverted, m_paletteNegative);
}

void SpectrogramWidgetBase::onDbMaxChanged(const QString& text) {
    bool ok; double val = text.toDouble(&ok); if(!ok) return;
    if (std::abs(val - m_currentMaxDb) < 1e-9) return;
    if (val <= m_currentMinDb) {
        m_currentMinDb = val - 1;
        if (m_currentMinDb < -300) { m_currentMinDb = -300; val = -299; }
        m_comboDbMin->blockSignals(true);
        m_comboDbMin->setCurrentText(QString::number((int)m_currentMinDb));
        m_comboDbMin->blockSignals(false);
        m_comboDbMax->blockSignals(true);
        m_comboDbMax->setCurrentText(QString::number((int)val));
        m_comboDbMax->blockSignals(false);
    }
    m_currentMaxDb = val;
    m_btndB->setText(QString(tr("dB:%1~%2")).arg(m_currentMaxDb).arg(m_currentMinDb));
    emit dbSettingsChanged(m_currentMinDb, m_currentMaxDb);
    emit parameterChanged();
}

void SpectrogramWidgetBase::onDbMinChanged(const QString& text) {
    bool ok; double val = text.toDouble(&ok); if(!ok) return;
    if (std::abs(val - m_currentMinDb) < 1e-9) return;
    if (val >= m_currentMaxDb) {
        m_currentMaxDb = val + 1;
        if (m_currentMaxDb > 0) { m_currentMaxDb = 0; val = -1; }
        m_comboDbMax->blockSignals(true); m_comboDbMax->setCurrentText(QString::number((int)m_currentMaxDb)); m_comboDbMax->blockSignals(false);
        m_comboDbMin->blockSignals(true); m_comboDbMin->setCurrentText(QString::number((int)val)); m_comboDbMin->blockSignals(false);
    }
    m_currentMinDb = val;
    m_btndB->setText(QString(tr("dB:%1~%2")).arg(m_currentMaxDb).arg(m_currentMinDb));
    emit dbSettingsChanged(m_currentMinDb, m_currentMaxDb);
    emit parameterChanged();
}

void SpectrogramWidgetBase::onWidthToggled(bool checked) {
    m_widthLimitEnabled = checked;
    m_spinWidth->setEnabled(checked);
    m_btnWidth->setChecked(checked);
}

void SpectrogramWidgetBase::onWidthValueChanged(int value) {
    m_widthLimitValue = value;
    m_btnWidth->setText(QString(tr("限宽:%1")).arg(value));
}

void SpectrogramWidgetBase::updateCrosshairStyle(const CrosshairStyle& style, bool enabled) {
    Q_UNUSED(style);
    Q_UNUSED(enabled);
}

void SpectrogramWidgetBase::updatePlayheadStyle(const PlayheadStyle& style) {
    Q_UNUSED(style);
}

void SpectrogramWidgetBase::setProfileFrameRate(int fps) {
    Q_UNUSED(fps);
}

void SpectrogramWidgetBase::updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) {
    Q_UNUSED(spectrumSrc);
    Q_UNUSED(probeSrc);
    Q_UNUSED(precision);
}

void SpectrogramWidgetBase::applyGlobalPreferences(const GlobalPreferences& prefs, bool silent){
    bool oldBlocked = this->signalsBlocked();
    this->blockSignals(true);
    if (m_btnLog->isChecked() != prefs.showLog) m_btnLog->setChecked(prefs.showLog);
    if (m_btnGrid->isChecked() != prefs.showGrid) {
        m_btnGrid->setChecked(prefs.showGrid);
        emit gridVisibilityChanged(prefs.showGrid);
    }
    if (m_btnComponents->isChecked() != prefs.showComponents) {
        m_btnComponents->setChecked(prefs.showComponents);
        emit componentsVisibilityChanged(prefs.showComponents);
    }
    if (m_btnZoom->isChecked() != prefs.enableZoom) {
        m_btnZoom->setChecked(prefs.enableZoom);
        emit zoomToggleChanged(prefs.enableZoom);
    }
    m_widthLimitEnabled = prefs.enableWidthLimit;
    m_widthLimitValue = prefs.maxWidth;
    m_btnWidth->setChecked(m_widthLimitEnabled);
    m_checkWidth->setChecked(m_widthLimitEnabled);
    m_spinWidth->setEnabled(m_widthLimitEnabled);
    m_spinWidth->setValue(m_widthLimitValue);
    m_btnWidth->setText(QString(tr("限宽:%1")).arg(m_widthLimitValue));
    bool needReprocess = false;
    if (m_currentHeight != prefs.height) {
        m_currentHeight = prefs.height;
        m_btnHeight->setText(QString(tr("高度:%1")).arg(m_currentHeight));
        needReprocess = true;
    }
    bool newAuto = (prefs.timeInterval <= 0.000000001);
    if (m_isAutoPrecision != newAuto || std::abs(m_currentInterval - prefs.timeInterval) > 1e-9) {
        m_isAutoPrecision = newAuto;
        m_currentInterval = prefs.timeInterval;
        if (m_isAutoPrecision) {
            m_btnInterval->setText(tr("精度:自动"));
        } else {
            m_btnInterval->setText(QString(tr("精度:%1")).arg(m_currentInterval));
        }
        needReprocess = true;
    }
    if (m_currentWindowType != prefs.windowType) {
        m_currentWindowType = prefs.windowType;
        m_btnWindow->setText(tr("窗口:") + getWindowName(m_currentWindowType));
        needReprocess = true;
    }
    if (m_currentCurveType != prefs.curveType) {
        m_currentCurveType = prefs.curveType;
        m_btnCurve->setText(tr("映射:") + getCurveName(m_currentCurveType));
        emit curveSettingsChanged(m_currentCurveType);
        needReprocess = true;
    }
    if (m_currentPaletteId != prefs.paletteId || m_paletteInverted != prefs.paletteInverted || m_paletteNegative != prefs.paletteNegative) {
        auto palettes = ColorPaletteFactory::instance().getAvailablePalettes();
        bool exists = false;
        for (const auto& p : palettes) {
            if (p.first == prefs.paletteId) { exists = true; break; }
        }
        m_currentPaletteId = exists ? prefs.paletteId : "0000";
        m_paletteInverted = prefs.paletteInverted;
        if (m_checkPaletteInvert && m_checkPaletteInvert->isChecked() != m_paletteInverted) {
            m_checkPaletteInvert->blockSignals(true);
            m_checkPaletteInvert->setChecked(m_paletteInverted);
            m_checkPaletteInvert->blockSignals(false);
        }
        m_paletteNegative = prefs.paletteNegative;
        if (m_checkPaletteNegative && m_checkPaletteNegative->isChecked() != m_paletteNegative) {
            m_checkPaletteNegative->blockSignals(true);
            m_checkPaletteNegative->setChecked(m_paletteNegative);
            m_checkPaletteNegative->blockSignals(false);
        }
        m_btnPalette->setText(tr("配色:") + m_currentPaletteId);
        emit paletteSettingsChanged(m_currentPaletteId, m_paletteInverted, m_paletteNegative);
    }
    if (std::abs(m_currentMinDb - prefs.minDb) > 1e-9 || std::abs(m_currentMaxDb - prefs.maxDb) > 1e-9) {
        m_currentMinDb = prefs.minDb;
        m_currentMaxDb = prefs.maxDb;
        m_comboDbMax->setCurrentText(QString::number((int)m_currentMaxDb));
        m_comboDbMin->setCurrentText(QString::number((int)m_currentMinDb));
        m_btndB->setText(QString(tr("dB:%1~%2")).arg(m_currentMaxDb).arg(m_currentMinDb));
        emit dbSettingsChanged(m_currentMinDb, m_currentMaxDb);
        needReprocess = true; 
    }
    this->blockSignals(oldBlocked);
    if (needReprocess && !silent) {
        emit parameterChanged();
    }
}

void SpectrogramWidgetBase::updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type, SpectrumProfileDirection direction) {
    Q_UNUSED(visible);
    Q_UNUSED(color);
    Q_UNUSED(lineWidth);
    Q_UNUSED(filled);
    Q_UNUSED(alpha);
    Q_UNUSED(type);
    Q_UNUSED(direction);
}

void SpectrogramWidgetBase::setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) {
    Q_UNUSED(showFreq);
    Q_UNUSED(showTime);
    Q_UNUSED(showDb);
}