#include "BatchFullLoadWidgetSettings.h"
#include "AppConfig.h"
#include "ColorPaletteFactory.h"
#include "MappingCurves.h"
#include "FFTWindowFunctions.h"
#include "AudioExtensionWhitelistDialog.h"

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
#include <QPainter>
#include <QStyledItemDelegate>

namespace {
    class PaletteComboBox : public QComboBox {
    public:
        explicit PaletteComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}
        QSize sizeHint() const override {
            QSize s = QComboBox::sizeHint();
            s.setWidth(s.width() + 110);
            return s;
        }
    protected:
        void paintEvent(QPaintEvent* event) override {
            QComboBox::paintEvent(event);
            int idx = currentIndex();
            if (idx >= 0) {
                QPixmap pm = itemData(idx, Qt::UserRole + 1).value<QPixmap>();
                if (!pm.isNull()) {
                    QPainter p(this);
                    QRect iconRect(rect().right() - 25 - 100, rect().top() + (rect().height() - 14) / 2, 100, 14);
                    p.drawPixmap(iconRect.topLeft(), pm);
                }
            }
        }
    };
    class PaletteComboBoxDelegate : public QStyledItemDelegate {
    public:
        explicit PaletteComboBoxDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            painter->save();
            if (opt.state & QStyle::State_Selected) {
                painter->fillRect(opt.rect, opt.palette.highlight());
                painter->setPen(opt.palette.highlightedText().color());
            } else {
                painter->fillRect(opt.rect, opt.palette.base());
                painter->setPen(opt.palette.text().color());
            }
            QString text = index.data(Qt::DisplayRole).toString();
            QPixmap pm = index.data(Qt::UserRole + 1).value<QPixmap>();
            QRect textRect = opt.rect.adjusted(5, 0, -100, 0);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
            if (!pm.isNull()) {
                QRect iconRect(opt.rect.right() - 105, opt.rect.top() + (opt.rect.height() - 14) / 2, 100, 14);
                painter->drawPixmap(iconRect.topLeft(), pm);
            }
            painter->restore();
        }
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            QSize s = QStyledItemDelegate::sizeHint(option, index);
            QString text = index.data(Qt::DisplayRole).toString();
            QFontMetrics fm(option.font);
            s.setWidth(fm.horizontalAdvance(text) + 100 + 30);
            return s;
        }
    };
}

BatchFullLoadWidgetSettings::BatchFullLoadWidgetSettings(const BatchSettings& currentSettings, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("批量设置"));
    setMinimumHeight(480);
    setupUi();
    loadSettings(currentSettings);
    adjustSize();
}

void BatchFullLoadWidgetSettings::setupUi(){
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
        QPushButton:disabled { 
            background-color: rgb(50, 58, 70); 
            color: #888888; 
            border: 1px solid #2B333E;
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
    QWidget *inputContainer = new QWidget(this);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(6);
    m_cbInput = new QCheckBox(tr("扫描子文件夹"), this);
    QWidget *whitelistWidget = new QWidget(this);
    QHBoxLayout *whitelistLayout = new QHBoxLayout(whitelistWidget);
    whitelistLayout->setContentsMargins(0, 0, 0, 0);
    whitelistLayout->setSpacing(5);
    m_cbWhitelist = new QCheckBox(tr("仅扫描白名单"), this);
    m_btnWhitelistConfig = new QPushButton(tr("配置白名单"), this);
    m_btnWhitelistConfig->setEnabled(false);
    whitelistLayout->addWidget(m_cbWhitelist);
    whitelistLayout->addWidget(m_btnWhitelistConfig);
    whitelistLayout->addStretch();
    m_cbExcludeVideo = new QCheckBox(tr("排除视频文件"), this);
    m_cbExcludeVideo->setToolTip(tr("跳过包含真实视频画面的文件"));
    inputLayout->addWidget(m_cbInput);
    inputLayout->addWidget(whitelistWidget);
    inputLayout->addWidget(m_cbExcludeVideo);
    addRow(tr("输入："), inputContainer);
    connect(m_cbWhitelist, &QCheckBox::toggled, m_btnWhitelistConfig, &QWidget::setEnabled);
    connect(m_btnWhitelistConfig, &QPushButton::clicked, this, [this]() {
        AudioExtensionWhitelistDialog dlg(m_currentWhitelist, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_currentWhitelist = dlg.getSelectedExtensions();
        }
    });

    // --- 分隔线 ---
    addSeparator();

    // 4. 输出
    QWidget *outputContainer = new QWidget(this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputContainer);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->setSpacing(6);
    m_cbOutput = new QCheckBox(tr("保持输入目录的层级结构"), this);
    m_cbCategorizeByCodec = new QCheckBox(tr("按音频编码类型进行分类"), this);
    outputLayout->addWidget(m_cbOutput);
    outputLayout->addWidget(m_cbCategorizeByCodec);
    addRow(tr("输出："), outputContainer);

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
    auto addWin = [&](FFTWindowType t) {
         m_cmbWindow->addItem(FFTWindowFunctions::getName(t), QVariant::fromValue(t));
    };
    addWin(FFTWindowType::Rectangular);
    addWin(FFTWindowType::Triangular);
    addWin(FFTWindowType::Hann);
    addWin(FFTWindowType::Hamming);
    addWin(FFTWindowType::Blackman);
    addWin(FFTWindowType::BlackmanHarris);
    addWin(FFTWindowType::FlatTop);
    addWin(FFTWindowType::Sine);
    addWin(FFTWindowType::Cauchy);
    addWin(FFTWindowType::Parzen);
    addWin(FFTWindowType::Poisson);
    addWin(FFTWindowType::Bohman);
    addWin(FFTWindowType::Nuttall);
    addWin(FFTWindowType::Lanczos);
    addWin(FFTWindowType::Welch);
    addWin(FFTWindowType::DolphChebyshev);
    addWin(FFTWindowType::BartlettHann);
    addWin(FFTWindowType::Minimum4Term);
    addWin(FFTWindowType::Minimum7Term);
    addWin(FFTWindowType::Gaussian);
    addWin(FFTWindowType::Kaiser);
    addWin(FFTWindowType::Tukey);
    addRow(tr("窗口："), m_cmbWindow);

    // 8. 映射
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
    QWidget* colorContainer = new QWidget(this);
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(6);
    m_cmbColor = new PaletteComboBox(this);
    m_cmbColor->setIconSize(QSize(100, 14));
    auto createColorIcon = [](const QString& id) -> QIcon {
        QPixmap pixmap(100, 14);
        QPainter p(&pixmap);
        const auto& colors = ColorPaletteFactory::instance().getPalette(id, false);
        for(int x = 0; x < 100; ++x) {
            int idx = x * 255 / 99;
            p.setPen(QColor(colors[idx]));
            p.drawLine(x, 0, x, 14);
        }
        return QIcon(pixmap);
    };
    auto palettesList = ColorPaletteFactory::instance().getAvailablePalettes();
    for (const auto& p : palettesList) {
        m_cmbColor->addItem(QString("%1    %2").arg(p.first).arg(p.second), p.first);
        m_cmbColor->setItemData(m_cmbColor->count() - 1, createColorIcon(p.first).pixmap(QSize(100, 14)), Qt::UserRole + 1);
    }
    m_cmbColor->setItemDelegate(new PaletteComboBoxDelegate(m_cmbColor));
    m_cbColorInvert = new QCheckBox(tr("反向"), this);
    m_cbColorNegative = new QCheckBox(tr("反相"), this);
    colorLayout->addWidget(m_cbColorInvert, 0);
    colorLayout->addWidget(m_cbColorNegative, 0);
    colorLayout->addWidget(m_cmbColor, 1);
    addRow(tr("配色："), colorContainer);

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
    connect(m_cmbDbMax, &QComboBox::textActivated, this, &BatchFullLoadWidgetSettings::onDbMaxChanged);
    connect(m_cmbDbMin, &QComboBox::textActivated, this, &BatchFullLoadWidgetSettings::onDbMinChanged);

    // --- 分隔线 ---
    addSeparator();

    // 11. 网格
    m_cbGrid = new QCheckBox(tr("绘制频率与时间的网格线"), this);
    addRow(tr("网格："), m_cbGrid);

    // 12. 组件
    m_cbComponents = new QCheckBox(tr("绘制文件名 频率轴 时间轴 dB轴"), this);
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
    connect(m_cmbFormat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BatchFullLoadWidgetSettings::onFormatChanged);
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

void BatchFullLoadWidgetSettings::onDbMaxChanged(const QString& text) {
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

void BatchFullLoadWidgetSettings::onDbMinChanged(const QString& text) {
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

void BatchFullLoadWidgetSettings::onFormatChanged(int index) {
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

void BatchFullLoadWidgetSettings::loadSettings(const BatchSettings& settings) {
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
    idx = m_cmbColor->findData(settings.paletteId);
    if (idx >= 0) m_cmbColor->setCurrentIndex(idx);
    m_cbColorInvert->setChecked(settings.paletteInverted);
    m_cbColorNegative->setChecked(settings.paletteNegative);
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
    m_cbWhitelist->setChecked(settings.enableWhitelist);
    m_btnWhitelistConfig->setEnabled(settings.enableWhitelist);
    m_currentWhitelist = settings.whitelistExtensions;
    m_cbExcludeVideo->setChecked(settings.excludeVideoFiles);
    m_cbCategorizeByCodec->setChecked(settings.categorizeByCodec);
}

BatchSettings BatchFullLoadWidgetSettings::getSettings() const {
    BatchSettings s;
    s.mode = m_cmbMode->currentData().value<BatchMode>();
    s.threadCount = m_cmbThreads->currentData().toInt();
    s.includeSubfolders = m_cbInput->isChecked();
    s.reuseSubfolderStructure = m_cbOutput->isChecked();
    s.imageHeight = m_cmbHeight->currentText().toInt();
    s.timeInterval = m_cmbPrecision->currentData().toDouble();
    s.curveType = m_cmbMapping->currentData().value<CurveType>();
    s.windowType = m_cmbWindow->currentData().value<FFTWindowType>();
    s.paletteId = m_cmbColor->currentData().toString();
    s.paletteInverted = m_cbColorInvert->isChecked();
    s.paletteNegative = m_cbColorNegative->isChecked();
    s.maxDb = m_cmbDbMax->currentText().toDouble();
    s.minDb = m_cmbDbMin->currentText().toDouble();
    s.enableGrid = m_cbGrid->isChecked();
    s.enableComponents = m_cbComponents->isChecked();
    s.enableWidthLimit = m_cbWidth->isChecked();
    s.maxWidth = m_spinWidth->value();
    s.exportFormat = m_cmbFormat->currentData().toString();
    s.qualityLevel = m_sliderQuality->value();
    s.enableWhitelist = m_cbWhitelist->isChecked();
    s.whitelistExtensions = m_currentWhitelist;
    s.excludeVideoFiles = m_cbExcludeVideo->isChecked();
    s.categorizeByCodec = m_cbCategorizeByCodec->isChecked();
    return s;
}