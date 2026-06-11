#include "SingleFileExportDialog.h"
#include "AppConfig.h"

#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QSpacerItem>
#include <QMessageBox>
#include <QDialogButtonBox>

SingleFileExportDialog::SingleFileExportDialog(
    const QString& defaultPath, 
    const QString& defaultBaseName, 
    bool isJpegAvailable,
    bool isWebpAvailable,
    bool isAvifAvailable,
    QWidget *parent)
    : QDialog(parent)
{
    setupUi(isJpegAvailable, isWebpAvailable, isAvifAvailable);
    setWindowTitle(tr("导出图片"));
    if (!defaultPath.isEmpty()) {
        m_pathLineEdit->setText(QDir::toNativeSeparators(defaultPath));
    }
    m_fileNameLineEdit->setText(defaultBaseName);
    m_fileNameLineEdit->setSelection(0, defaultBaseName.length());
    m_fileNameLineEdit->setFocus();
}

void SingleFileExportDialog::setupUi(bool isJpegAvailable, bool isWebpAvailable, bool isAvifAvailable) {
    setStyleSheet(R"(
        QDialog { 
            background-color: #3B4453;
            color: #E0E0E0;
        }
        QLabel { 
            font-size: 12px;
            color: #E0E0E0; 
        }
        QLineEdit { 
            background-color: rgb(78, 90, 110); 
            border: 1px solid #2B333E; 
            color: #E0E0E0; 
            padding: 2px;
            font-size: 11px;
        }
        QComboBox { 
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
            outline: none;
        }
        QPushButton { 
            font-size: 12px; 
            min-width: 60px;
            min-height: 20px;
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
    )");
    setMinimumWidth(450);
    setMinimumHeight(270);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);
    auto gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(2, 2, 2, 3);
    gridLayout->setVerticalSpacing(6);
    gridLayout->setHorizontalSpacing(4);
    auto pathLabel = new QLabel(tr("储存路径:"), this);
    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);
    m_pathLineEdit->setPlaceholderText(tr("请点击“浏览”选择储存路径"));
    m_browseButton = new QPushButton(tr("浏览"), this);
    connect(m_browseButton, &QPushButton::clicked, this, &SingleFileExportDialog::onBrowsePath);
    gridLayout->addWidget(pathLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(m_pathLineEdit, 0, 1);
    gridLayout->addWidget(m_browseButton, 0, 2);
    auto fileNameLabel = new QLabel(tr("图片名称:"), this);
    m_fileNameLineEdit = new QLineEdit(this);
    gridLayout->addWidget(fileNameLabel, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(m_fileNameLineEdit, 1, 1, 1, 2);
    auto formatLabel = new QLabel(tr("储存格式:"), this);
    m_formatComboBox = new QComboBox(this);
    m_formatComboBox->addItem("PNG  ( *.png )  libpng", "PNG");
    m_formatComboBox->addItem("PNG  ( *.png )  Qt", "QtPNG");
    m_formatComboBox->addItem("BMP  ( *.bmp )", "BMP");
    m_formatComboBox->addItem("TIFF  ( *.tif )", "TIFF");
    if (isJpegAvailable) {
        m_formatComboBox->addItem("JPG  ( *.jpg )", "JPG");
    }
    m_formatComboBox->addItem("JPEG 2000  ( *.jp2 )", "JPEG 2000");
    if (isWebpAvailable) {
        m_formatComboBox->addItem("WebP  ( *.webp )", "WebP");
    }
    if (isAvifAvailable) {
        m_formatComboBox->addItem("AVIF  ( *.avif )", "AVIF");
    }
    m_formatComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents); 
    m_formatComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
    gridLayout->addWidget(formatLabel, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(m_formatComboBox, 2, 1, 1, 2);
    m_qualityLabel = new QLabel(tr("压缩等级:"), this);
    m_qualitySlider = new QSlider(Qt::Horizontal, this);
    m_qualityValueLabel = new QLabel(this);
    m_qualityValueLabel->setMinimumWidth(25);
    m_qualityValueLabel->setAlignment(Qt::AlignCenter);
    gridLayout->addWidget(m_qualityLabel, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
    gridLayout->addWidget(m_qualitySlider, 3, 1);
    gridLayout->addWidget(m_qualityValueLabel, 3, 2);
    gridLayout->setColumnStretch(1, 1); 
    mainLayout->addLayout(gridLayout);
    m_tooltipLabel = new QLabel(this);
    m_tooltipLabel->setWordWrap(true);
    m_tooltipLabel->setStyleSheet("color: #888888; margin-top: 5px;");
    m_tooltipLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop); 
    m_tooltipLabel->setMinimumHeight(40);
    mainLayout->addWidget(m_tooltipLabel);
    mainLayout->addStretch();
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(5);
    btnLayout->setContentsMargins(0, 5, 0, 0);
    m_buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    QPushButton *btnSave = m_buttonBox->addButton(tr("保存"), QDialogButtonBox::AcceptRole);
    QPushButton *btnCancel = m_buttonBox->addButton(tr("取消"), QDialogButtonBox::RejectRole);
    btnLayout->addStretch();
    btnLayout->addWidget(m_buttonBox);
    mainLayout->addLayout(btnLayout);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SingleFileExportDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SingleFileExportDialog::onFormatChanged);
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualityValueLabel, qOverload<int>(&QLabel::setNum));
    onFormatChanged(m_formatComboBox->currentIndex());
    adjustSize();
}

void SingleFileExportDialog::onAccept() {
    if (m_pathLineEdit->text().isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, tr("路径无效"), tr("请先点击“浏览”按钮选择一个保存路径。"), QMessageBox::Ok, this);
        msgBox.setButtonText(QMessageBox::Ok, tr("确定"));
        msgBox.exec();
        return;
    }
    QDialog::accept();
}

QString SingleFileExportDialog::getSelectedFormatIdentifier() const {
    return m_formatComboBox->currentData().toString();
}

QString SingleFileExportDialog::getSelectedFilePath() const {
    QString path = m_pathLineEdit->text();
    QString fileName = m_fileNameLineEdit->text();
    QString identifier = getSelectedFormatIdentifier();
    QString suffix;
    if (identifier == "PNG" || identifier == "QtPNG") suffix = ".png";
    else if (identifier == "BMP") suffix = ".bmp";
    else if (identifier == "TIFF") suffix = ".tif";
    else if (identifier == "JPG") suffix = ".jpg";
    else if (identifier == "JPEG 2000") suffix = ".jp2";
    else if (identifier == "WebP") suffix = ".webp";
    else if (identifier == "AVIF") suffix = ".avif";
    QString finalFileNameWithSuffix = fileName + suffix;
    return QDir(path).filePath(finalFileNameWithSuffix);
}

int SingleFileExportDialog::qualityLevel() const { return m_qualitySlider->value(); }

void SingleFileExportDialog::onBrowsePath() {
    QString startPath = m_pathLineEdit->text();
    if (startPath.isEmpty()) {
        startPath = QDir::homePath();
    }
    QString directory = QFileDialog::getExistingDirectory(this, tr("选择保存文件夹"), startPath);
    if (!directory.isEmpty()) {
        m_pathLineEdit->setText(QDir::toNativeSeparators(directory));
    }
}

void SingleFileExportDialog::onFormatChanged(int index) {
    if (index < 0) return;
    QString identifier = m_formatComboBox->itemData(index).toString();
    bool showQualityControls = true;
    if (identifier == "PNG") {
        m_qualityLabel->setText(tr("压缩级别:"));
        m_qualitySlider->setRange(0, 9);
        m_qualitySlider->setValue(AppConfig::PNG_SAVE_DEFAULT_COMPRESSION_LEVEL);
        m_tooltipLabel->setText(
            tr("压缩级别（无损）：0 - 9\n"
            "                         0：无压缩，文件大，保存快\n"
            "                         9：极限压缩，文件小，保存慢")
        );
    } else if (identifier == "JPG") {
        m_qualityLabel->setText(tr("图像质量:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(AppConfig::JPG_SAVE_DEFAULT_QUALITY);
        m_tooltipLabel->setText(
            tr("图像质量（有损）：1 - 100\n"
            "                         1：质量差，文件小\n"
            "                     100：质量高，文件大")
        );
    } else if (identifier == "WebP") {
        m_qualityLabel->setText(tr("图像质量:"));
        m_qualitySlider->setRange(1, 101);
        m_qualitySlider->setValue(75);
        m_tooltipLabel->setText(
            tr("图像质量（有损）：1 - 100\n"
            "                         1：质量差，文件小\n"
            "                     100：质量高，文件大\n\n"
            "图像质量（无损）：101\n"
            "                     101：无压缩，文件最大\n")
        );
    } else if (identifier == "AVIF") {
        m_qualityLabel->setText(tr("图像质量:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(55);
        m_tooltipLabel->setText(
            tr("图像质量（有损）：1 - 99\n"
            "                         1：质量差，文件小\n"
            "                       99：质量高，文件大\n\n"
            "图像质量（无损）：100\n"
            "                     100：无压缩，文件最大\n")
        );
    } else if (identifier == "JPEG 2000") {
        m_qualityLabel->setText(tr("图像质量:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(45);
        m_tooltipLabel->setText(
            tr("图像质量（有损）：1 - 99\n"
            "                         1：质量差，文件小\n"
            "                       99：质量高，文件大\n\n"
            "图像质量（无损）：100\n"
            "                     100：无压缩，文件最大\n")
        );
    } else {
        showQualityControls = false;
        m_tooltipLabel->setText("");
    }
    m_qualityLabel->setVisible(showQualityControls);
    m_qualitySlider->setVisible(showQualityControls);
    m_qualityValueLabel->setVisible(showQualityControls);
    m_tooltipLabel->setVisible(!m_tooltipLabel->text().isEmpty());
    if (showQualityControls) {
        m_qualityValueLabel->setNum(m_qualitySlider->value());
    }
}