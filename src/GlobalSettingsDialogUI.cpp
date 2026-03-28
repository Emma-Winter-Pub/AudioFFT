#include "GlobalSettingsDialogUI.h"
#include "AppConfig.h"
#include "ColorPalette.h"
#include "MappingCurves.h"
#include "WindowFunctions.h"
#include "GlobalPreferences.h" 

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QFrame>
#include <QSlider>
#include <QTabWidget>
#include <QDialog>
#include <QKeySequenceEdit>

GlobalSettingsDialogUI::GlobalSettingsDialogUI() {}
GlobalSettingsDialogUI::~GlobalSettingsDialogUI() {}

void GlobalSettingsDialogUI::setupUi(QDialog* parent) {
    parent->setStyleSheet(R"(
        QDialog { background-color: #3B4453; color: #E0E0E0; }
        QTabWidget::pane { border: 1px solid #2B333E; background: #3B4453; }
        QTabBar::tab { background: #2F3642; color: #E0E0E0; padding: 6px 12px; border: 1px solid #2B333E; border-bottom: none; }
        QTabBar::tab:selected { background: #4E5A6E; color: white; }
        QLabel { font-size: 12px; color: #E0E0E0; }
        QCheckBox { font-size: 12px; color: #E0E0E0; spacing: 4px; }
        QComboBox, QSpinBox { font-size: 12px; height: 20px; background-color: rgb(78, 90, 110); border: 1px solid #2B333E; color: #E0E0E0; padding-left: 2px; }
        QComboBox QAbstractItemView { background-color: rgb(78, 90, 110); selection-background-color: #5A687A; color: #E0E0E0; border: 1px solid #2B333E; }
        QPushButton { font-size: 12px; min-width: 0px; min-height: 18px; padding: 2px 10px; background-color: rgb(78, 90, 110); color: #E0E0E0; border: 1px solid #1C222B; }
        QPushButton:hover { background-color: #556171; }
        QPushButton:pressed { background-color: #303645; }
    )");
    auto mainLayout = new QVBoxLayout(parent);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    QTabWidget* tabWidget = new QTabWidget(parent);
    auto createTab = [&](QGridLayout*& layoutOut) -> QWidget* {
        QWidget* widget = new QWidget(parent);
        layoutOut = new QGridLayout(widget);
        layoutOut->setContentsMargins(15, 15, 15, 15);
        layoutOut->setVerticalSpacing(8);
        layoutOut->setHorizontalSpacing(10);
        layoutOut->setColumnStretch(1, 1);
        return widget;
    };
    auto addRow = [](QGridLayout* layout, int& row, QString labelText, QWidget* widget) {
        QLabel* label = new QLabel(labelText);
        layout->addWidget(label, row, 0, Qt::AlignRight | Qt::AlignVCenter);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(widget, row, 1, Qt::AlignVCenter);
        row++;
    };
    auto addBottomNote = [](QGridLayout* layout, int& row) {
        QLabel* note = new QLabel(QObject::tr("标记 * 的选项将在下一次启动 AudioFFT 时生效"));
        note->setStyleSheet("color: #888888; margin-top: 10px;");
        note->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        layout->addWidget(note, row++, 0, 1, 2);
    };
    QList<QColor> presets = { 
        Qt::white, Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, 
        QColor(255, 128, 0), QColor(128, 0, 255), Qt::gray 
    };

    //常规
    QGridLayout* layoutTab1;
    QWidget* tab1 = createTab(layoutTab1);
    int row1 = 0;
    m_cbLog = new QCheckBox(QObject::tr("启用"), parent); 
    addRow(layoutTab1, row1, QObject::tr("* 日志："), m_cbLog);
    m_cbGrid = new QCheckBox(QObject::tr("启用"), parent); 
    addRow(layoutTab1, row1, QObject::tr("* 网格："), m_cbGrid);
    m_cbComponents = new QCheckBox(QObject::tr("启用"), parent); 
    addRow(layoutTab1, row1, QObject::tr("* 组件："), m_cbComponents);
    m_cbZoom = new QCheckBox(QObject::tr("启用"), parent); 
    addRow(layoutTab1, row1, QObject::tr("* 缩放频率："), m_cbZoom);
    QWidget* widthContainer = new QWidget(parent);
    QHBoxLayout* widthLayout = new QHBoxLayout(widthContainer);
    widthLayout->setContentsMargins(0, 0, 0, 0);
    widthLayout->setSpacing(5);
    m_cbWidthLimit = new QCheckBox(QObject::tr("启用      限制最大宽度"), parent);
    m_spinWidthVal = new QSpinBox(parent);
    m_spinWidthVal->setRange(500, 10000);
    m_spinWidthVal->setFixedWidth(60);
    widthLayout->addWidget(m_cbWidthLimit);
    widthLayout->addWidget(m_spinWidthVal);
    widthLayout->addWidget(new QLabel(QObject::tr("像素"), parent));
    widthLayout->addStretch();
    QObject::connect(m_cbWidthLimit, &QCheckBox::toggled, m_spinWidthVal, &QWidget::setEnabled);
    addRow(layoutTab1, row1, QObject::tr("* 限宽："), widthContainer);
    layoutTab1->setRowStretch(row1, 1); row1++;
    addBottomNote(layoutTab1, row1);
    tabWidget->addTab(tab1, QObject::tr("常规"));

    //性能
    QGridLayout* layoutTab2;
    QWidget* tab2 = createTab(layoutTab2);
    int row2 = 0;
    m_cbGpuAccel = new QCheckBox(QObject::tr("启用 (仅适用于频率分布图)"), parent);
    m_cbGpuAccel->setToolTip(QObject::tr("使用 OpenGL 渲染频率分布图"));
    addRow(layoutTab2, row2, QObject::tr("* 硬件加速："), m_cbGpuAccel);
    m_cbCacheFft = new QCheckBox(QObject::tr("启用 (仅适用于全量模式)"), parent);
    m_cbCacheFft->setToolTip(QObject::tr("保留FFT原始计算结果，提高精度，但会增加内存占用"));
    addRow(layoutTab2, row2, QObject::tr("缓存FFT："), m_cbCacheFft);
    QWidget* playerFpsContainer = new QWidget(parent);
    QHBoxLayout* playerFpsLayout = new QHBoxLayout(playerFpsContainer);
    playerFpsLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderPlayerFps = new QSlider(Qt::Horizontal, parent);
    m_sliderPlayerFps->setRange(15, 300);
    m_lblPlayerFpsVal = new QLabel("60 Hz", parent); 
    m_lblPlayerFpsVal->setFixedWidth(50);
    m_lblPlayerFpsVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_sliderPlayerFps, &QSlider::valueChanged, [this](int val){ m_lblPlayerFpsVal->setText(QString("%1 Hz").arg(val)); });
    playerFpsLayout->addWidget(m_sliderPlayerFps);
    playerFpsLayout->addWidget(m_lblPlayerFpsVal);
    addRow(layoutTab2, row2, QObject::tr("指针帧率："), playerFpsContainer);
    QWidget* profileFpsContainer = new QWidget(parent);
    QHBoxLayout* profileFpsLayout = new QHBoxLayout(profileFpsContainer);
    profileFpsLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderProfileFps = new QSlider(Qt::Horizontal, parent);
    m_sliderProfileFps->setRange(15, 300);
    m_lblProfileFpsVal = new QLabel("30 Hz", parent);
    m_lblProfileFpsVal->setFixedWidth(50);
    m_lblProfileFpsVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_sliderProfileFps, &QSlider::valueChanged, [this](int val){ m_lblProfileFpsVal->setText(QString("%1 Hz").arg(val)); });
    profileFpsLayout->addWidget(m_sliderProfileFps);
    profileFpsLayout->addWidget(m_lblProfileFpsVal);
    addRow(layoutTab2, row2, QObject::tr("曲线帧率："), profileFpsContainer);
    layoutTab2->setRowStretch(row2, 1); row2++;
    addBottomNote(layoutTab2, row2);
    tabWidget->addTab(tab2, QObject::tr("性能"));

    //频谱图
    QGridLayout* layoutTab3;
    QWidget* tab3 = createTab(layoutTab3);
    int row3 = 0;
    m_cmbHeight = new QComboBox(parent);
    const std::vector<int> specialHeights = { 16385, 8193, 4097, 2049, 1025, 513, 257, 129, 65, 33 };
    for (int h : specialHeights) m_cmbHeight->addItem(QString::number(h), h);
    m_cmbHeight->insertSeparator(specialHeights.size());
    for (int h = 8000; h >= 100; h -= 100) m_cmbHeight->addItem(QString::number(h), h);
    addRow(layoutTab3, row3, QObject::tr("* 高度："), m_cmbHeight);
    m_cmbPrecision = new QComboBox(parent);
    m_cmbPrecision->addItem(QObject::tr("自动"), 0.0);
    const QStringList intervals = { "1", "0.5", "0.25", "0.2", "0.1", "0.05", "0.025", "0.02", "0.01", "0.005", "0.0025", "0.002", "0.001" };
    for (const auto& interval : intervals) {
        m_cmbPrecision->addItem(interval, interval.toDouble());
    }
    addRow(layoutTab3, row3, QObject::tr("* 精度："), m_cmbPrecision);
    m_cmbWindow = new QComboBox(parent);
    auto addWin = [&](WindowType t) {
        m_cmbWindow->addItem(WindowFunctions::getName(t), QVariant::fromValue(t));
    };
    addWin(WindowType::Rectangular); addWin(WindowType::Triangular); addWin(WindowType::Hann);
    addWin(WindowType::Hamming); addWin(WindowType::Blackman); addWin(WindowType::BlackmanHarris);
    addWin(WindowType::BlackmanNuttall); addWin(WindowType::Nuttall); addWin(WindowType::FlatTop);
    addWin(WindowType::Minimum4Term); addWin(WindowType::Minimum7Term); addWin(WindowType::Gaussian);
    addWin(WindowType::Kaiser); addWin(WindowType::Tukey); addWin(WindowType::Lanczos);
    addRow(layoutTab3, row3, QObject::tr("* 窗口："), m_cmbWindow);
    m_cmbMapping = new QComboBox(parent);
    const QList<CurveInfo> curves = MappingCurves::getAllCurves();
    for (const auto& info : curves) {
        m_cmbMapping->addItem(info.displayText, QVariant::fromValue(info.type));
        if (info.hasSeparator) m_cmbMapping->insertSeparator(m_cmbMapping->count());
    }
    addRow(layoutTab3, row3, QObject::tr("* 映射："), m_cmbMapping);
    m_cmbPalette = new QComboBox(parent);
    QMap<PaletteType, QString> palettes = ColorPalette::getPaletteNames();
    for (auto it = palettes.begin(); it != palettes.end(); ++it) {
        m_cmbPalette->addItem(it.value(), QVariant::fromValue(it.key()));
    }
    addRow(layoutTab3, row3, QObject::tr("* 配色："), m_cmbPalette);
    QWidget* dbContainer = new QWidget(parent);
    QHBoxLayout* dbLayout = new QHBoxLayout(dbContainer);
    dbLayout->setContentsMargins(0, 0, 0, 0);
    dbLayout->setSpacing(2);
    QLabel* lblMax = new QLabel(QObject::tr("上限"), parent);
    m_cmbDbMax = new QComboBox(parent);
    for (int i = 0; i >= -299; i--) m_cmbDbMax->addItem(QString::number(i));
    m_cmbDbMax->setFixedWidth(60);
    QLabel* lblMin = new QLabel(QObject::tr(" 下限"), parent);
    m_cmbDbMin = new QComboBox(parent);
    for (int i = -1; i >= -300; i--) m_cmbDbMin->addItem(QString::number(i));
    m_cmbDbMin->setFixedWidth(60);
    dbLayout->addWidget(lblMax); dbLayout->addWidget(m_cmbDbMax);
    dbLayout->addSpacing(6);
    dbLayout->addWidget(lblMin); dbLayout->addWidget(m_cmbDbMin);
    dbLayout->addStretch();
    addRow(layoutTab3, row3, QObject::tr("* dB范围："), dbContainer);
    layoutTab3->setRowStretch(row3, 1); row3++;
    addBottomNote(layoutTab3, row3);
    tabWidget->addTab(tab3, QObject::tr("频谱图"));

    //频率分布
    QGridLayout* layoutTab4;
    QWidget* tab4 = createTab(layoutTab4);
    int row4 = 0;
    m_cbProfile = new QCheckBox(QObject::tr("显示"), parent);
    addRow(layoutTab4, row4, QObject::tr("开关："), m_cbProfile);
    m_cmbSpecSource = new QComboBox(parent);
    m_cmbSpecSource->addItem(QObject::tr("频谱图"), QVariant::fromValue(DataSourceType::ImagePixel));
    m_cmbSpecSource->addItem(QObject::tr("FFT缓存"), QVariant::fromValue(DataSourceType::FftRawData));
    addRow(layoutTab4, row4, QObject::tr("* 数据源："), m_cmbSpecSource);
    m_cmbProfileType = new QComboBox(parent);
    m_cmbProfileType->addItem(QObject::tr("折线图"), QVariant::fromValue(SpectrumProfileType::Line));
    m_cmbProfileType->addItem(QObject::tr("柱状图"), QVariant::fromValue(SpectrumProfileType::Bar));
    addRow(layoutTab4, row4, QObject::tr("类型："), m_cmbProfileType);
    QWidget* profWidthContainer = new QWidget(parent);
    QHBoxLayout* profWidthLayout = new QHBoxLayout(profWidthContainer);
    profWidthLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderProfileWidth = new QSlider(Qt::Horizontal, parent);
    m_sliderProfileWidth->setRange(1, 5);
    m_lblProfileWidthVal = new QLabel("1", parent); 
    m_lblProfileWidthVal->setFixedWidth(40); 
    m_lblProfileWidthVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_sliderProfileWidth, &QSlider::valueChanged, m_lblProfileWidthVal, qOverload<int>(&QLabel::setNum));
    profWidthLayout->addWidget(m_sliderProfileWidth); 
    profWidthLayout->addWidget(m_lblProfileWidthVal);
    addRow(layoutTab4, row4, QObject::tr("线宽："), profWidthContainer);
    QWidget* profColorContainer = new QWidget(parent);
    QHBoxLayout* profColorLayout = new QHBoxLayout(profColorContainer);
    profColorLayout->setContentsMargins(0, 0, 0, 3);
    m_btnProfileColorPreview = new QPushButton(parent);
    m_btnProfileColorPreview->setFixedSize(75, 20);
    profColorLayout->addWidget(m_btnProfileColorPreview); 
    profColorLayout->addStretch();
    addRow(layoutTab4, row4, QObject::tr("颜色："), profColorContainer);
    QWidget* profPresetContainer = new QWidget(parent);
    QGridLayout* profPresetLayout = new QGridLayout(profPresetContainer);
    profPresetLayout->setContentsMargins(0, 0, 0, 0);
    profPresetLayout->setSpacing(2);
    for (int i = 0; i < presets.size(); ++i) {
        auto btn = new QPushButton(parent);
        btn->setFixedSize(12, 18);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString("background-color: %1; border: none;").arg(presets[i].name()));
        btn->setProperty("color", presets[i]);
        btn->setProperty("isPresetBtn", true); 
        btn->setProperty("presetTarget", "profile");
        profPresetLayout->addWidget(btn, 0, i);
    }
    profPresetLayout->setColumnStretch(10, 1);
    addRow(layoutTab4, row4, QObject::tr("预设："), profPresetContainer);
    m_cbProfileFill = new QCheckBox(QObject::tr("启用"), parent);
    addRow(layoutTab4, row4, QObject::tr("填充："), m_cbProfileFill);
    QWidget* alphaContainer = new QWidget(parent);
    QHBoxLayout* alphaLayout = new QHBoxLayout(alphaContainer);
    alphaLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderProfileAlpha = new QSlider(Qt::Horizontal, parent);
    m_sliderProfileAlpha->setRange(0, 255);
    m_lblProfileAlphaVal = new QLabel("127", parent); 
    m_lblProfileAlphaVal->setFixedWidth(30); 
    m_lblProfileAlphaVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_cbProfileFill, &QCheckBox::toggled, m_sliderProfileAlpha, &QWidget::setEnabled);
    QObject::connect(m_sliderProfileAlpha, &QSlider::valueChanged, m_lblProfileAlphaVal, qOverload<int>(&QLabel::setNum));
    alphaLayout->addWidget(m_sliderProfileAlpha); 
    alphaLayout->addWidget(m_lblProfileAlphaVal);
    addRow(layoutTab4, row4, QObject::tr("不透明度："), alphaContainer);
    layoutTab4->setRowStretch(row4, 1); row4++;
    addBottomNote(layoutTab4, row4);
    tabWidget->addTab(tab4, QObject::tr("频率分布"));

    //十字光标
    QGridLayout* layoutTab5;
    QWidget* tab5 = createTab(layoutTab5);
    int row5 = 0;
    m_cbCrosshair = new QCheckBox(QObject::tr("启用"), parent);
    addRow(layoutTab5, row5, QObject::tr("开关："), m_cbCrosshair);
    QWidget* lenContainer = new QWidget(parent);
    QHBoxLayout* lenLayout = new QHBoxLayout(lenContainer);
    lenLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderCrossLen = new QSlider(Qt::Horizontal, parent);
    m_sliderCrossLen->setRange(100, 10000); 
    m_sliderCrossLen->setSingleStep(50); 
    m_sliderCrossLen->setPageStep(200);
    m_lblCrossLenVal = new QLabel("1000", parent); 
    m_lblCrossLenVal->setFixedWidth(40); 
    m_lblCrossLenVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lenLayout->addWidget(m_sliderCrossLen); 
    lenLayout->addWidget(m_lblCrossLenVal);
    addRow(layoutTab5, row5, QObject::tr("线长："), lenContainer);
    QWidget* widthContainer2 = new QWidget(parent);
    QHBoxLayout* widthLayout2 = new QHBoxLayout(widthContainer2);
    widthLayout2->setContentsMargins(0, 0, 10, 0);
    m_sliderCrossWidth = new QSlider(Qt::Horizontal, parent);
    m_sliderCrossWidth->setRange(1, 15);
    m_lblCrossWidthVal = new QLabel("1", parent); 
    m_lblCrossWidthVal->setFixedWidth(40); 
    m_lblCrossWidthVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_sliderCrossWidth, &QSlider::valueChanged, m_lblCrossWidthVal, qOverload<int>(&QLabel::setNum));
    widthLayout2->addWidget(m_sliderCrossWidth); 
    widthLayout2->addWidget(m_lblCrossWidthVal);
    addRow(layoutTab5, row5, QObject::tr("线宽："), widthContainer2);
    QWidget* colorContainer = new QWidget(parent);
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->setContentsMargins(0, 0, 0, 3);
    m_btnColorPreview = new QPushButton(parent);
    m_btnColorPreview->setFixedSize(75, 20);
    colorLayout->addWidget(m_btnColorPreview); 
    colorLayout->addStretch();
    addRow(layoutTab5, row5, QObject::tr("颜色："), colorContainer);
    QWidget* presetContainer = new QWidget(parent);
    QGridLayout* presetLayout = new QGridLayout(presetContainer);
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(2);
    for (int i = 0; i < presets.size(); ++i) {
        auto btn = new QPushButton(parent);
        btn->setFixedSize(12, 18);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString("background-color: %1; border: none;").arg(presets[i].name()));
        btn->setProperty("color", presets[i]);
        btn->setProperty("isPresetBtn", true); 
        btn->setProperty("presetTarget", "crosshair");
        presetLayout->addWidget(btn, 0, i);
    }
    presetLayout->setColumnStretch(10, 1);
    addRow(layoutTab5, row5, QObject::tr("预设："), presetContainer);
    layoutTab5->setRowStretch(row5, 1);
    tabWidget->addTab(tab5, QObject::tr("十字光标"));

    //探针
    QGridLayout* layoutTab6;
    QWidget* tab6 = createTab(layoutTab6);
    int row6 = 0;
    m_cbCoordFreq = new QCheckBox(QObject::tr("显示"), parent); 
    addRow(layoutTab6, row6, QObject::tr("频率："), m_cbCoordFreq);
    m_cbCoordTime = new QCheckBox(QObject::tr("显示"), parent); 
    addRow(layoutTab6, row6, QObject::tr("时间："), m_cbCoordTime);
    m_cbCoordDb = new QCheckBox(QObject::tr("显示"), parent); 
    addRow(layoutTab6, row6, QObject::tr("dB："), m_cbCoordDb);
    m_cmbProbeSource = new QComboBox(parent);
    m_cmbProbeSource->addItem(QObject::tr("频谱图"), QVariant::fromValue(DataSourceType::ImagePixel));
    m_cmbProbeSource->addItem(QObject::tr("FFT缓存"), QVariant::fromValue(DataSourceType::FftRawData));
    addRow(layoutTab6, row6, QObject::tr("* 数据源："), m_cmbProbeSource);
    QWidget* precContainer = new QWidget(parent);
    QHBoxLayout* precLayout = new QHBoxLayout(precContainer);
    precLayout->setContentsMargins(0, 0, 0, 0);
    precLayout->setSpacing(10); 
    m_sliderProbePrecision = new QSlider(Qt::Horizontal, parent);
    m_sliderProbePrecision->setRange(0, 2); 
    m_sliderProbePrecision->setPageStep(1); 
    m_sliderProbePrecision->setFixedWidth(120);
    m_lblProbePrecisionVal = new QLabel(parent); 
    m_lblProbePrecisionVal->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    precLayout->addWidget(m_sliderProbePrecision); 
    precLayout->addWidget(m_lblProbePrecisionVal);
    precLayout->addStretch();
    addRow(layoutTab6, row6, QObject::tr("精度："), precContainer);
    layoutTab6->setRowStretch(row6, 1); row6++;
    addBottomNote(layoutTab6, row6);
    tabWidget->addTab(tab6, QObject::tr("探针"));

    //播放器
    QGridLayout* layoutTab7;
    QWidget* tab7 = createTab(layoutTab7);
    int row7 = 0;
    m_cbPlayheadVisible = new QCheckBox(QObject::tr("显示"), parent);
    addRow(layoutTab7, row7, QObject::tr("指针："), m_cbPlayheadVisible);
    QWidget* phWidthContainer = new QWidget(parent);
    QHBoxLayout* phWidthLayout = new QHBoxLayout(phWidthContainer);
    phWidthLayout->setContentsMargins(0, 0, 10, 0);
    m_sliderPlayheadWidth = new QSlider(Qt::Horizontal, parent);
    m_sliderPlayheadWidth->setRange(1, 10);
    m_lblPlayheadWidthVal = new QLabel("1", parent);
    m_lblPlayheadWidthVal->setFixedWidth(40);
    m_lblPlayheadWidthVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QObject::connect(m_sliderPlayheadWidth, &QSlider::valueChanged, m_lblPlayheadWidthVal, qOverload<int>(&QLabel::setNum));
    phWidthLayout->addWidget(m_sliderPlayheadWidth);
    phWidthLayout->addWidget(m_lblPlayheadWidthVal);
    addRow(layoutTab7, row7, QObject::tr("线宽："), phWidthContainer);
    QWidget* handleColorContainer = new QWidget(parent);
    QHBoxLayout* handleColorLayout = new QHBoxLayout(handleColorContainer);
    handleColorLayout->setContentsMargins(0, 0, 0, 3);
    m_btnPlayheadHandleColor = new QPushButton(parent);
    m_btnPlayheadHandleColor->setFixedSize(75, 20);
    handleColorLayout->addWidget(m_btnPlayheadHandleColor);
    handleColorLayout->addStretch();
    addRow(layoutTab7, row7, QObject::tr("手柄颜色："), handleColorContainer);
    QWidget* handlePresetContainer = new QWidget(parent);
    QGridLayout* handlePresetLayout = new QGridLayout(handlePresetContainer);
    handlePresetLayout->setContentsMargins(0, 0, 0, 0);
    handlePresetLayout->setSpacing(2);
    for (int i = 0; i < presets.size(); ++i) {
        auto btn = new QPushButton(parent);
        btn->setFixedSize(12, 18);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString("background-color: %1; border: none;").arg(presets[i].name()));
        btn->setProperty("color", presets[i]);
        btn->setProperty("isPresetBtn", true);
        btn->setProperty("presetTarget", "playheadHandle");
        handlePresetLayout->addWidget(btn, 0, i);
    }
    handlePresetLayout->setColumnStretch(10, 1);
    addRow(layoutTab7, row7, QObject::tr("预设："), handlePresetContainer);
    QWidget* phColorContainer = new QWidget(parent);
    QHBoxLayout* phColorLayout = new QHBoxLayout(phColorContainer);
    phColorLayout->setContentsMargins(0, 0, 0, 3);
    m_btnPlayheadColor = new QPushButton(parent);
    m_btnPlayheadColor->setFixedSize(75, 20);
    phColorLayout->addWidget(m_btnPlayheadColor);
    phColorLayout->addStretch();
    addRow(layoutTab7, row7, QObject::tr("指针颜色："), phColorContainer);
    QWidget* phPresetContainer = new QWidget(parent);
    QGridLayout* phPresetLayout = new QGridLayout(phPresetContainer);
    phPresetLayout->setContentsMargins(0, 0, 0, 0);
    phPresetLayout->setSpacing(2);
    for (int i = 0; i < presets.size(); ++i) {
        auto btn = new QPushButton(parent);
        btn->setFixedSize(12, 18);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString("background-color: %1; border: none;").arg(presets[i].name()));
        btn->setProperty("color", presets[i]);
        btn->setProperty("isPresetBtn", true);
        btn->setProperty("presetTarget", "playheadColor");
        phPresetLayout->addWidget(btn, 0, i);
    }
    phPresetLayout->setColumnStretch(10, 1);
    addRow(layoutTab7, row7, QObject::tr("预设："), phPresetContainer);
    layoutTab7->setRowStretch(row7, 1);
    tabWidget->addTab(tab7, QObject::tr("播放器"));
    mainLayout->addWidget(tabWidget);

    //截图
    QGridLayout* layoutTab8;
    QWidget* tab8 = createTab(layoutTab8);
    int row8 = 0;
    m_cbHideCursor = new QCheckBox(QObject::tr("启用"), parent);
    addRow(layoutTab8, row8, QObject::tr("隐藏鼠标指针："), m_cbHideCursor);
    m_cbClipboard = new QCheckBox(QObject::tr("启用 (仅适用于 快捷键1 和 快捷键2)"), parent);
    addRow(layoutTab8, row8, QObject::tr("复制到剪贴板："), m_cbClipboard);
    layoutTab8->addItem(new QSpacerItem(20, 15), row8++, 0);
    QLabel* groupLabel1 = new QLabel(QObject::tr("截图并保存为图片"), parent);
    layoutTab8->addWidget(groupLabel1, row8++, 0, 1, 2);
    m_keyEditScreenshot1 = new QKeySequenceEdit(parent);
    m_keyEditScreenshot1->setStyleSheet("background-color: rgb(78, 90, 110); color: #E0E0E0; border: 1px solid #2B333E;");
    addRow(layoutTab8, row8, QObject::tr("快捷键1："), m_keyEditScreenshot1);
    m_keyEditScreenshot2 = new QKeySequenceEdit(parent);
    m_keyEditScreenshot2->setStyleSheet("background-color: rgb(78, 90, 110); color: #E0E0E0; border: 1px solid #2B333E;");
    addRow(layoutTab8, row8, QObject::tr("快捷键2："), m_keyEditScreenshot2);
    layoutTab8->addItem(new QSpacerItem(20, 15), row8++, 0);
    QLabel* groupLabel2 = new QLabel(QObject::tr("仅截图到剪贴板"), parent);
    layoutTab8->addWidget(groupLabel2, row8++, 0, 1, 2);
    m_keyEditQuickCopy = new QKeySequenceEdit(parent);
    m_keyEditQuickCopy->setStyleSheet("background-color: rgb(78, 90, 110); color: #E0E0E0; border: 1px solid #2B333E;");
    addRow(layoutTab8, row8, QObject::tr("快捷键3："), m_keyEditQuickCopy);
    QLabel* hintLabel = new QLabel(QObject::tr("点击快捷键，然后按下键盘按键，即可修改默认键值"), parent);
    hintLabel->setStyleSheet("color: #888888; margin-top: 10px;");
    layoutTab8->addWidget(hintLabel, row8++, 0, 1, 2);
    layoutTab8->setRowStretch(row8, 1);
    tabWidget->addTab(tab8, QObject::tr("截图"));

    // 底部按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->setContentsMargins(0, 5, 0, 0);
    m_btnRestore = new QPushButton(QObject::tr("恢复默认"), parent);
    m_btnOk = new QPushButton(QObject::tr("确定"), parent);
    m_btnCancel = new QPushButton(QObject::tr("取消"), parent);
    btnLayout->addWidget(m_btnRestore);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOk);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);
}