#include "BatWidgetSettings.h"
#include "AppConfig.h"
#include "ColorPalette.h"
#include "MappingCurves.h"
#include "WindowFunctions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QThread>

BatWidgetSettings::BatWidgetSettings(const BatSettings& currentSettings, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("批量设置"));
    setMinimumHeight(390);
    setupUi();
    loadSettings(currentSettings);
    adjustSize();
}

void BatWidgetSettings::setupUi(){
    setStyleSheet(R"(
        QDialog { 
            background-color: #3B4453; 
            color: #E0E0E0;
        }
        QLabel { 
            font-size: 12px; 
            color: #E0E0E0; 
        }
        QCheckBox { 
            font-size: 12px; 
            color: #E0E0E0; 
            spacing: 4px;
        }
        QComboBox, QSpinBox { 
            font-size: 12px; 
            height: 18px; 
            background-color: rgb(78, 90, 110); 
            border: 1px solid #2B333E; 
            color: #E0E0E0; 
            padding-left: 2px; 
        }
        QComboBox QAbstractItemView { 
            background-color: rgb(78, 90, 110); 
            selection-background-color: #5A687A; 
            color: #E0E0E0; 
            border: 1px solid #2B333E; 
        }
        QPushButton { 
            font-size: 12px; 
            min-width: 60px; 
            min-height: 22px; max-height: 22px; 
            padding: 0 10px;
            background-color: rgb(78, 90, 110); 
            color: #E0E0E0; 
            border: 1px solid #1C222B; 
        }
        QPushButton:hover { 
            background-color: rgb(55, 65, 81); 
        }
        QPushButton:pressed { 
            background-color: rgb(30, 36, 45); 
        }
        QFrame[frameShape="4"] {
            color: #2B333E;
            background-color: #2B333E;
            border: none;
            max-height: 1px;
            min-height: 1px;
        }
    )");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(2, 2, 2, 3);
    gridLayout->setVerticalSpacing(2);
    gridLayout->setHorizontalSpacing(4);
    gridLayout->setColumnStretch(0, 0);
    gridLayout->setColumnStretch(1, 1);
    int row = 0;
    auto addRow = [&](QString text, QWidget *widget) {
        QLabel *label = new QLabel(text, this);
        gridLayout->addWidget(label, row, 0, Qt::AlignRight | Qt::AlignVCenter);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        gridLayout->addWidget(widget, row, 1, Qt::AlignVCenter);
        row++;
    };
    auto addSeparator = [&]() {
        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Plain);
        line->setStyleSheet("margin-top: 2px; margin-bottom: 2px; background-color: #2B333E;"); 
        gridLayout->addWidget(line, row, 0, 1, 2); 
        row++;
    };

    // 1. 模式
    m_cmbMode = new QComboBox(this);
    m_cmbMode->addItem(tr("全量"), QVariant::fromValue(BatchMode::FullLoad));
    m_cmbMode->addItem(tr("流式"), QVariant::fromValue(BatchMode::Streaming));
    m_cmbMode->setToolTip(tr("全量：加载整个文件到内存，速度最快，但内存占用高。\n流式：分块读取和处理，节省内存，适合超大文件或低配机器。"));
    addRow(tr("模式："), m_cmbMode);

    // 2. 线程
    m_cmbThreads = new QComboBox(this);
    int maxThreads = QThread::idealThreadCount();
    if (maxThreads < 1) maxThreads = 1;
    for (int i = maxThreads; i >= 1; --i) {
        m_cmbThreads->addItem(QString::number(i), i);
    }
    m_cmbThreads->setCurrentIndex(0);
    addRow(tr("线程："), m_cmbThreads);

    // --- 分隔线 ---
    addSeparator();

    // 3. 输入
    m_cbInput = new QCheckBox(tr("扫描子文件夹"), this);
    addRow(tr("输入："), m_cbInput);

    // 4. 输出
    m_cbOutput = new QCheckBox(tr("保持目录结构"), this);
    addRow(tr("输出："), m_cbOutput);

    // --- 分隔线 ---
    addSeparator();

    // 5. 高度
    m_cmbHeight = new QComboBox(this);
    const std::vector<int> specialHeights = { 16385, 8193, 4097, 2049, 1025, 513, 257, 129, 65, 33 };
    for (int h : specialHeights) m_cmbHeight->addItem(QString::number(h));
    m_cmbHeight->insertSeparator(specialHeights.size());
    for (int h = 8000; h >= 100; h -= 100) m_cmbHeight->addItem(QString::number(h)); 
    addRow(tr("高度："), m_cmbHeight);

    // 6. 精度
    m_cmbPrecision = new QComboBox(this);
    m_cmbPrecision->addItem(tr("自动"), 0.0);
    const QStringList intervals = {"1", "0.5", "0.25", "0.2", "0.1", "0.05", "0.025", "0.02", "0.01", "0.005", "0.0025", "0.002", "0.001"};
    for (const auto& interval : intervals) {
        m_cmbPrecision->addItem(interval, interval.toDouble());
    }
    addRow(tr("精度："), m_cmbPrecision);

    // 7. 窗函数
    m_cmbWindow = new QComboBox(this);
    auto addWin = [&](WindowType t) {
         m_cmbWindow->addItem(WindowFunctions::getName(t), QVariant::fromValue(t));
    };
    addWin(WindowType::Rectangular);
    addWin(WindowType::Triangular);
    addWin(WindowType::Hann);
    addWin(WindowType::Hamming);
    addWin(WindowType::Blackman);
    addWin(WindowType::BlackmanHarris);
    addWin(WindowType::BlackmanNuttall);
    addWin(WindowType::Nuttall);
    addWin(WindowType::FlatTop);
    addWin(WindowType::Minimum4Term);
    addWin(WindowType::Minimum7Term);
    addWin(WindowType::Gaussian);
    addWin(WindowType::Kaiser);
    addWin(WindowType::Tukey);
    addWin(WindowType::Lanczos);
    addRow(tr("窗口："), m_cmbWindow);

    // 8. 映射 (最宽项，决定列宽)
    m_cmbMapping = new QComboBox(this);
    const QList<CurveInfo> curves = MappingCurves::getAllCurves();
    for (const auto& info : curves) {
        m_cmbMapping->addItem(info.displayText, QVariant::fromValue(info.type));
        if (info.hasSeparator) {
            m_cmbMapping->insertSeparator(m_cmbMapping->count());
        }
    }
    addRow(tr("映射："), m_cmbMapping);

    // 9. 配色
    m_cmbColor = new QComboBox(this);
    QMap<PaletteType, QString> palettes = ColorPalette::getPaletteNames();
    for(auto it = palettes.begin(); it != palettes.end(); ++it) {
        m_cmbColor->addItem(it.value(), QVariant::fromValue(it.key()));
    }
    addRow(tr("配色："), m_cmbColor);

    // 10. dB
    QWidget *dbContainer = new QWidget(this);
    QHBoxLayout *dbLayout = new QHBoxLayout(dbContainer);
    dbLayout->setContentsMargins(0, 0, 0, 0);
    dbLayout->setSpacing(2); 
    QLabel *lblMax = new QLabel(tr("上限"), this);
    m_cmbDbMax = new QComboBox(this);
    for(int i=0; i>=-299; i--) m_cmbDbMax->addItem(QString::number(i));
    QLabel *lblMin = new QLabel(tr(" 下限"), this);
    m_cmbDbMin = new QComboBox(this);
    for(int i=-1; i>=-300; i--) m_cmbDbMin->addItem(QString::number(i));
    m_cmbDbMax->setFixedWidth(65);
    m_cmbDbMin->setFixedWidth(65);

    // 布局：Label Max Min Label Min Max Stretch
    dbLayout->addWidget(lblMax);
    dbLayout->addWidget(m_cmbDbMax);
    dbLayout->addSpacing(4); 
    dbLayout->addWidget(lblMin);
    dbLayout->addWidget(m_cmbDbMin);
    dbLayout->addStretch();
    addRow(tr("dB："), dbContainer);
    connect(m_cmbDbMax, &QComboBox::textActivated, this, &BatWidgetSettings::onDbMaxChanged);
    connect(m_cmbDbMin, &QComboBox::textActivated, this, &BatWidgetSettings::onDbMinChanged);

    // --- 分隔线 ---
    addSeparator();

    // 11. 网格
    m_cbGrid = new QCheckBox(tr("绘制频率与时间的网格线"), this);
    addRow(tr("网格："), m_cbGrid);

    // 12. 组件
    m_cbComponents = new QCheckBox(tr("绘制文件名、频率轴、时间轴、dB轴"), this);
    m_cbComponents->setToolTip(tr("取消勾选将仅保存纯净的频谱图，不包含任何文字和坐标轴信息。"));
    addRow(tr("组件："), m_cbComponents);

    // 12. 限宽
    QWidget *widthContainer = new QWidget(this);
    QHBoxLayout *widthLayout = new QHBoxLayout(widthContainer);
    widthLayout->setContentsMargins(0, 0, 0, 0);
    widthLayout->setSpacing(4);
    m_cbWidth = new QCheckBox(this); 
    m_spinWidth = new QSpinBox(this);
    m_spinWidth->setRange(500, 10000);
    m_spinWidth->setValue(2000);
    m_spinWidth->setFixedWidth(65);
    QLabel* lblWidthLimit = new QLabel(tr("限制图像最大宽度"), this);
    widthLayout->addWidget(m_cbWidth);
    widthLayout->addWidget(m_spinWidth);
    widthLayout->addWidget(lblWidthLimit);
    widthLayout->addStretch(); 
    addRow(tr("限宽："), widthContainer);
    connect(m_cbWidth, &QCheckBox::toggled, m_spinWidth, &QWidget::setEnabled);

    // --- 分隔线 ---
    addSeparator();

    // 13. 格式
    m_cmbFormat = new QComboBox(this);
    m_cmbFormat->addItem("PNG (libpng)", "PNG");       
    m_cmbFormat->addItem("PNG (Qt)", "QtPNG");         
    m_cmbFormat->addItem("BMP", "BMP");                
    m_cmbFormat->addItem("TIFF", "TIFF");              
    m_cmbFormat->addItem("JPG", "JPG");                
    m_cmbFormat->addItem("JPEG 2000", "JPEG 2000");    
    m_cmbFormat->addItem("WebP", "WebP");              
    m_cmbFormat->addItem("AVIF", "AVIF"); 
    addRow(tr("格式："), m_cmbFormat);

    // 14. 质量
    m_lblQualityTitle = new QLabel(tr("质量："), this);
    m_qualityContainer = new QWidget(this);
    QHBoxLayout *qualityLayout = new QHBoxLayout(m_qualityContainer);
    qualityLayout->setContentsMargins(0, 0, 0, 0);
    qualityLayout->setSpacing(5);
    m_sliderQuality = new QSlider(Qt::Horizontal, this);
    m_sliderQuality->setRange(0, 100);
    m_sliderQuality->setValue(80);
    m_lblQualityVal = new QLabel("80", this);
    m_lblQualityVal->setFixedWidth(25); 
    m_lblQualityVal->setAlignment(Qt::AlignCenter);
    connect(m_sliderQuality, &QSlider::valueChanged, m_lblQualityVal, qOverload<int>(&QLabel::setNum));
    qualityLayout->addWidget(m_sliderQuality);
    qualityLayout->addWidget(m_lblQualityVal);
    gridLayout->addWidget(m_lblQualityTitle, row, 0, Qt::AlignRight | Qt::AlignVCenter);
    m_qualityContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    gridLayout->addWidget(m_qualityContainer, row, 1, Qt::AlignVCenter);
    connect(m_cmbFormat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatWidgetSettings::onFormatChanged);
    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(); 
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->setContentsMargins(0, 5, 0, 0);
    QPushButton *btnOk = new QPushButton(tr("确定"), this);
    QPushButton *btnCancel = new QPushButton(tr("取消"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void BatWidgetSettings::onDbMaxChanged(const QString& text) {
    bool ok; double newMax = text.toDouble(&ok); if(!ok) return;
    double currentMin = m_cmbDbMin->currentText().toDouble();
    if (newMax <= currentMin) {
        double newMin = newMax - 1;
        if (newMin < -300) { newMin = -300; newMax = -299;
            m_cmbDbMax->blockSignals(true);
            m_cmbDbMax->setCurrentText(QString::number((int)newMax));
            m_cmbDbMax->blockSignals(false);
        }
        m_cmbDbMin->blockSignals(true);
        m_cmbDbMin->setCurrentText(QString::number((int)newMin));
        m_cmbDbMin->blockSignals(false);
    }
}

void BatWidgetSettings::onDbMinChanged(const QString& text) {
    bool ok; double newMin = text.toDouble(&ok); if(!ok) return;
    double currentMax = m_cmbDbMax->currentText().toDouble();
    if (newMin >= currentMax) {
        double newMax = newMin + 1;
        if (newMax > 0) { newMax = 0; newMin = -1;
            m_cmbDbMin->blockSignals(true);
            m_cmbDbMin->setCurrentText(QString::number((int)newMin));
            m_cmbDbMin->blockSignals(false);
        }
        m_cmbDbMax->blockSignals(true);
        m_cmbDbMax->setCurrentText(QString::number((int)newMax));
        m_cmbDbMax->blockSignals(false);
    }
}

void BatWidgetSettings::onFormatChanged(int index) {
    if (index < 0) return;
    QString id = m_cmbFormat->itemData(index).toString();
    m_sliderQuality->blockSignals(true);
    if (id == "PNG") {
        m_lblQualityTitle->setVisible(true);
        m_qualityContainer->setVisible(true);
        m_lblQualityTitle->setText(tr("压缩："));
        m_sliderQuality->setRange(0, 9);
        m_sliderQuality->setValue(AppConfig::PNG_SAVE_DEFAULT_COMPRESSION_LEVEL); 
    } 
    else if (id == "QtPNG" || id == "BMP" || id == "TIFF") {
        m_lblQualityTitle->setVisible(false);
        m_qualityContainer->setVisible(false);
    } 
    else {
        m_lblQualityTitle->setVisible(true);
        m_qualityContainer->setVisible(true);
        m_lblQualityTitle->setText(tr("质量："));
        m_sliderQuality->setRange(1, 100);
        
        if (id == "JPG") m_sliderQuality->setValue(AppConfig::JPG_SAVE_DEFAULT_QUALITY);
        else if (id == "WebP") { m_sliderQuality->setRange(1, 101); m_sliderQuality->setValue(75); }
        else if (id == "AVIF") m_sliderQuality->setValue(55);
        else if (id == "JPEG 2000") m_sliderQuality->setValue(45);
    }
    m_lblQualityVal->setNum(m_sliderQuality->value());
    m_sliderQuality->blockSignals(false);
}

void BatWidgetSettings::loadSettings(const BatSettings& settings) {
    int modeIdx = m_cmbMode->findData(QVariant::fromValue(settings.mode));
    if (modeIdx >= 0) m_cmbMode->setCurrentIndex(modeIdx);
    int threadIdx = m_cmbThreads->findData(settings.threadCount);
    if (threadIdx >= 0) {
        m_cmbThreads->setCurrentIndex(threadIdx);
    } else {
        m_cmbThreads->setCurrentIndex(0);
    }
    m_cbInput->setChecked(settings.includeSubfolders);
    m_cbOutput->setChecked(settings.reuseSubfolderStructure);
    m_cmbHeight->setCurrentText(QString::number(settings.imageHeight));
    int idx = m_cmbPrecision->findData(settings.timeInterval);
    if (idx >= 0) {
        m_cmbPrecision->setCurrentIndex(idx);
    } else {
        if (settings.timeInterval <= 0.000000001) {
            m_cmbPrecision->setCurrentIndex(0);
        } else {
            int defaultIdx = m_cmbPrecision->findText("0.1");
            if (defaultIdx >= 0) m_cmbPrecision->setCurrentIndex(defaultIdx);
        }
    }
    idx = m_cmbMapping->findData(QVariant::fromValue(settings.curveType));
    if (idx >= 0) m_cmbMapping->setCurrentIndex(idx);
    idx = m_cmbWindow->findData(QVariant::fromValue(settings.windowType));
    if (idx >= 0) m_cmbWindow->setCurrentIndex(idx);
    idx = m_cmbColor->findData(QVariant::fromValue(settings.paletteType));
    if (idx >= 0) m_cmbColor->setCurrentIndex(idx);
    m_cmbDbMax->setCurrentText(QString::number((int)settings.maxDb));
    m_cmbDbMin->setCurrentText(QString::number((int)settings.minDb));
    m_cbGrid->setChecked(settings.enableGrid);
    m_cbComponents->setChecked(settings.enableComponents);
    m_cbWidth->setChecked(settings.enableWidthLimit);
    m_spinWidth->setValue(settings.maxWidth);
    m_spinWidth->setEnabled(settings.enableWidthLimit);
    idx = m_cmbFormat->findData(settings.exportFormat);
    if (idx < 0) idx = 0; 
    if (m_cmbFormat->currentIndex() != idx) {
        m_cmbFormat->setCurrentIndex(idx); 
    } else {
        onFormatChanged(idx); 
    }
    if (m_qualityContainer->isVisible()) {
        m_sliderQuality->blockSignals(true);
        m_sliderQuality->setValue(settings.qualityLevel);
        m_lblQualityVal->setNum(settings.qualityLevel);
        m_sliderQuality->blockSignals(false);
    }
}

BatSettings BatWidgetSettings::getSettings() const {
    BatSettings s;
    s.mode = m_cmbMode->currentData().value<BatchMode>();
    s.threadCount = m_cmbThreads->currentData().toInt();
    s.includeSubfolders = m_cbInput->isChecked();
    s.reuseSubfolderStructure = m_cbOutput->isChecked();
    s.imageHeight = m_cmbHeight->currentText().toInt();
    s.timeInterval = m_cmbPrecision->currentData().toDouble();
    s.curveType = m_cmbMapping->currentData().value<CurveType>();
    s.windowType = m_cmbWindow->currentData().value<WindowType>();
    s.paletteType = m_cmbColor->currentData().value<PaletteType>();
    s.maxDb = m_cmbDbMax->currentText().toDouble();
    s.minDb = m_cmbDbMin->currentText().toDouble();
    s.enableGrid = m_cbGrid->isChecked();
    s.enableComponents = m_cbComponents->isChecked();
    s.enableWidthLimit = m_cbWidth->isChecked();
    s.maxWidth = m_spinWidth->value();
    s.exportFormat = m_cmbFormat->currentData().toString();
    s.qualityLevel = m_sliderQuality->value();
    return s;
}