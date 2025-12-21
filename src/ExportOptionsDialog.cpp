#include "ExportOptionsDialog.h"
#include "AppConfig.h"

#include <QLabel>
#include <QSlider>
#include <QFileInfo>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSpacerItem>
#include <QMessageBox>
#include <QDialogButtonBox>


ExportOptionsDialog::ExportOptionsDialog(
    const QString& defaultPath, 
    const QString& defaultBaseName, 
    bool isJpegAvailable,
    bool isWebpAvailable,
    bool isAvifAvailable,
    QWidget *parent)
    : QDialog(parent)
{
    setupUi(isJpegAvailable, isWebpAvailable, isAvifAvailable);
    setWindowTitle(tr("Export Image"));

    if (!defaultPath.isEmpty()) {
        m_pathLineEdit->setText(QDir::toNativeSeparators(defaultPath));
    }
    
    m_fileNameLineEdit->setText(defaultBaseName);
    m_fileNameLineEdit->setSelection(0, defaultBaseName.length());
    m_fileNameLineEdit->setFocus();
}


void ExportOptionsDialog::setupUi(bool isJpegAvailable, bool isWebpAvailable, bool isAvifAvailable)
{
    setFixedSize(500, 300);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 10);
    mainLayout->setSpacing(10);

    auto gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(10);
    gridLayout->setVerticalSpacing(12);

    auto pathLabel = new QLabel(tr("Path:"), this);
    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);
    m_pathLineEdit->setPlaceholderText(tr("Click 'Browse' to select a save path"));
    m_browseButton = new QPushButton(tr("Browse"), this);
    connect(m_browseButton, &QPushButton::clicked, this, &ExportOptionsDialog::onBrowsePath);

    gridLayout->addWidget(pathLabel, 0, 0, Qt::AlignRight);
    gridLayout->addWidget(m_pathLineEdit, 0, 1);
    gridLayout->addWidget(m_browseButton, 0, 2);

    auto fileNameLabel = new QLabel(tr("Name:"), this);
    m_fileNameLineEdit = new QLineEdit(this);

    gridLayout->addWidget(fileNameLabel, 1, 0, Qt::AlignRight);
    gridLayout->addWidget(m_fileNameLineEdit, 1, 1);

    auto formatLabel = new QLabel(tr("Format:"), this);
    m_formatComboBox = new QComboBox(this);
    m_formatComboBox->addItem("PNG  ( *.png )  libpng", "PNG");
    m_formatComboBox->addItem("PNG  ( *.png )  Qt", "QtPNG");
    m_formatComboBox->addItem("BMP  ( *.bmp )", "BMP");
    m_formatComboBox->addItem("TIFF  ( *.tif )", "TIFF");
    if (isJpegAvailable) {
        m_formatComboBox->addItem("JPG  ( *.jpg )", "JPG");}
    m_formatComboBox->addItem("JPEG 2000  ( *.jp2 )", "JPEG 2000");
    if (isWebpAvailable) {
        m_formatComboBox->addItem("WebP  ( *.webp )", "WebP");}
    if (isAvifAvailable) {
        m_formatComboBox->addItem("AVIF  ( *.avif )", "AVIF");}
    m_formatComboBox->setItemData(0, tr("Uses libpng, offers 0-9 compression levels"), Qt::ToolTipRole);
    m_formatComboBox->setItemData(1, tr("Uses Qt internal engine, optimized default compression, no parameters"), Qt::ToolTipRole);
    m_formatComboBox->setMinimumWidth(140); 
    m_formatComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
    
    gridLayout->addWidget(formatLabel, 2, 0, Qt::AlignRight);
    gridLayout->addWidget(m_formatComboBox, 2, 1);

    m_qualityLabel = new QLabel(tr("Compression:"), this);
    m_qualitySlider = new QSlider(Qt::Horizontal, this);
    m_qualityValueLabel = new QLabel(this);
    m_qualityValueLabel->setMinimumWidth(25);

    gridLayout->addWidget(m_qualityLabel, 3, 0, Qt::AlignRight);
    gridLayout->addWidget(m_qualitySlider, 3, 1);
    gridLayout->addWidget(m_qualityValueLabel, 3, 2);

    gridLayout->setColumnStretch(1, 1);

    mainLayout->addLayout(gridLayout);

    m_tooltipLabel = new QLabel(this);
    m_tooltipLabel->setWordWrap(true);
    m_tooltipLabel->setStyleSheet("color: #AAAAAA; padding-left: 60px;"); 
    m_tooltipLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); 
    m_tooltipLabel->setMinimumHeight(35);
    mainLayout->addWidget(m_tooltipLabel);
    
    mainLayout->addStretch();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("Save"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ExportOptionsDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportOptionsDialog::onFormatChanged);
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualityValueLabel, qOverload<int>(&QLabel::setNum));

    onFormatChanged(m_formatComboBox->currentIndex());
}


void ExportOptionsDialog::onAccept()
{
    if (m_pathLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Path"), tr("Please click the 'Browse' button to select a save path first."));
        return;
    }
    QDialog::accept();
}


QString ExportOptionsDialog::getSelectedFormatIdentifier() const {
    return m_formatComboBox->currentData().toString();
}


QString ExportOptionsDialog::getSelectedFilePath() const {

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


int ExportOptionsDialog::qualityLevel() const {
    return m_qualitySlider->value();
}


void ExportOptionsDialog::onBrowsePath() {
    QString startPath = m_pathLineEdit->text();
    if (startPath.isEmpty()) {
        startPath = QDir::homePath();}

    QString directory = QFileDialog::getExistingDirectory(this, tr("Select Save Folder"), startPath);
    if (!directory.isEmpty()) {
        m_pathLineEdit->setText(QDir::toNativeSeparators(directory));}
}


void ExportOptionsDialog::onFormatChanged(int index) {

    if (index < 0) return;

    QString identifier = m_formatComboBox->itemData(index).toString();
    bool showQualityControls = true;

    if (identifier == "PNG") {
        m_qualityLabel->setText(tr("Compression:"));
        m_qualitySlider->setRange(0, 9);
        m_qualitySlider->setValue(AppConfig::PNG_SAVE_DEFAULT_COMPRESSION_LEVEL);
        m_tooltipLabel->setText(tr(
            "Compression Level (Lossless): 0 - 9\n"
            "                         0: No compression, large file, fast save\n"
            "                         9: Max compression, small file, slow save"
        ));
    } else if (identifier == "JPG") {
        m_qualityLabel->setText(tr("Quality:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(AppConfig::JPG_SAVE_DEFAULT_QUALITY);
        m_tooltipLabel->setText(tr(
            "Image Quality (Lossy): 1 - 100\n"
            "                         1: Low quality, small file\n"
            "                     100: High quality, large file"
        ));
    } else if (identifier == "WebP") {
        m_qualityLabel->setText(tr("Quality:"));
        m_qualitySlider->setRange(1, 101);
        m_qualitySlider->setValue(75);
        m_tooltipLabel->setText(tr(
            "Image Quality (Lossy): 1 - 100\n"
            "                         1: Low quality, small file\n"
            "                     100: High quality, large file\n"
            "Image Quality (Lossless): 101\n"
            "                     101: No compression, largest file\n"
        ));
    } else if (identifier == "AVIF") {
        m_qualityLabel->setText(tr("Quality:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(75);
        m_tooltipLabel->setText(tr(
            "Image Quality (Lossy): 1 - 99\n"
            "                         1: Low quality, small file\n"
            "                       99: High quality, large file\n"
            "Image Quality (Lossless): 100\n"
            "                     100: No compression, largest file\n"
        ));
    } else if (identifier == "JPEG 2000") {
        m_qualityLabel->setText(tr("Quality:"));
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(75);
        m_tooltipLabel->setText(tr(
            "Image Quality (Lossy): 1 - 99\n"
            "                         1: Low quality, small file\n"
            "                       99: High quality, large file\n"
            "Image Quality (Lossless): 100\n"
            "                     100: No compression, largest file\n"
        ));
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