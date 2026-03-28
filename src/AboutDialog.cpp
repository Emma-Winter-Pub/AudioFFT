#include "AboutDialog.h"
#include "AppConfig.h"

#include <QDir>
#include <QIcon>
#include <QFile>
#include <QLabel>
#include <QLocale>
#include <QWidget>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTextBrowser>
#include <QSettings>
#include <QCoreApplication>

static QString detectSystemLanguage() {
    QLocale systemLocale = QLocale::system();
    QLocale::Language lang = systemLocale.language();
    switch (lang) {
        case QLocale::German: return "de";
        case QLocale::English: return "en";
        case QLocale::French: return "fr";
        case QLocale::Japanese: return "ja";
        case QLocale::Korean: return "ko";
        case QLocale::Russian: return "ru";
        case QLocale::Chinese:
            if (systemLocale.script() == QLocale::SimplifiedChineseScript || 
                systemLocale.country() == QLocale::China) {
                return "zh-JT";
            }
            return "zh-FT";
        default: return "";
    }
}

AboutDialog::AboutDialog(QWidget *parent): QDialog(parent) {
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    m_currentLangCode = settings.value("language").toString(); 
    if (m_currentLangCode.isEmpty()) {
        m_currentLangCode = detectSystemLanguage();
    }
    if (m_currentLangCode.isEmpty()) {
        m_currentLangCode = "zh-JT";
    }
    setupUi();
    setWindowTitle(tr("AudioFFT - 帮助"));
    setFixedHeight(300); 
    setMinimumWidth(450);
}

QString AboutDialog::loadLocalizedMdText(const QString& baseName) {
    QString targetPath = QString(":/text/%1_%2.md").arg(m_currentLangCode).arg(baseName);
    if (!QFile::exists(targetPath)) {
        targetPath = QString(":/text/zh-JT_%1.md").arg(baseName);
    }
    QFile file(targetPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("错误: 无法加载文件 %1").arg(targetPath);
    }
    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    return in.readAll();
}

void AboutDialog::setupUi() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createProgramInfoTab(), tr("关于"));
    m_tabWidget->addTab(createIntroTab(), tr("简介"));
    m_tabWidget->addTab(createUserManualTab(), tr("使用说明"));
    m_tabWidget->addTab(createChangelogTab(), tr("更新记录"));
    m_tabWidget->addTab(createCopyrightTab(), tr("声明"));
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);
    setLayout(mainLayout);
}

QWidget* AboutDialog::createProgramInfoTab() {
    auto infoWidget = new QWidget;
    auto mainLayout = new QVBoxLayout(infoWidget);
    mainLayout->setContentsMargins(0, 30, 0, 20);
    mainLayout->setSpacing(40);
    auto headerWidget = new QWidget;
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(40, 0, 40, 0);
    headerLayout->setSpacing(0);
    auto iconLabel = new QLabel(this);
    QIcon appIcon(":/AudioFFT_logo.ico");
    if (!appIcon.isNull()) {
        iconLabel->setPixmap(appIcon.pixmap(64, 64));
    } else {
        iconLabel->setFixedSize(64, 64);
    }
    auto titleLabel = new QLabel("AudioFFT", this);
    QFont f = titleLabel->font();
    f.setPointSize(24);
    f.setBold(true);
    titleLabel->setFont(f);
    headerLayout->addWidget(iconLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);
    headerLayout->addSpacing(64);
    mainLayout->addWidget(headerWidget);
    auto detailsWidget = new QWidget;
    auto gridLayout = new QGridLayout(detailsWidget);
    gridLayout->setContentsMargins(20, 0, 20, 0);
    gridLayout->setHorizontalSpacing(15); 
    gridLayout->setVerticalSpacing(8);    
    gridLayout->setColumnStretch(0, 1); 
    gridLayout->setColumnStretch(3, 1);
    int row = 0;
    auto addInfoRow = [&](const QString& labelText, const QString& valueText) {
        auto label = new QLabel(labelText, this);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto value = new QLabel(valueText, this);
        value->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse); 
        gridLayout->addWidget(label, row, 1);
        gridLayout->addWidget(value, row, 2);
        row++;
    };
    addInfoRow(tr("版本："), "1.1");
    addInfoRow(tr("时间："), AppConfig::getCompileTimestamp());
    addInfoRow(tr("作者："), "Emma Winter"); 
    addInfoRow(tr("性质："), tr("免费"));
    mainLayout->addWidget(detailsWidget);
    mainLayout->addStretch(); 
    infoWidget->setLayout(mainLayout);
    return infoWidget;
}

QWidget* AboutDialog::createIntroTab() {
    auto guideWidget = new QWidget;
    auto layout = new QVBoxLayout(guideWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    auto textBrowser = new QTextBrowser(this);
    textBrowser->setReadOnly(true);
    textBrowser->setMarkdown(loadLocalizedMdText("Intro"));
    layout->addWidget(textBrowser);
    guideWidget->setLayout(layout);
    return guideWidget;
}

QWidget* AboutDialog::createUserManualTab() {
    auto manualWidget = new QWidget;
    auto layout = new QVBoxLayout(manualWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    auto textBrowser = new QTextBrowser(this);
    textBrowser->setReadOnly(true);
    textBrowser->setMarkdown(loadLocalizedMdText("UserManual"));
    layout->addWidget(textBrowser);
    manualWidget->setLayout(layout);
    return manualWidget;
}

QWidget* AboutDialog::createChangelogTab() {
    auto changelogWidget = new QWidget;
    auto layout = new QVBoxLayout(changelogWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    auto textBrowser = new QTextBrowser(this);
    textBrowser->setReadOnly(true);
    textBrowser->setMarkdown(loadLocalizedMdText("Changelog"));
    layout->addWidget(textBrowser);
    changelogWidget->setLayout(layout);
    return changelogWidget;
}

QWidget* AboutDialog::createCopyrightTab() {
    auto copyrightWidget = new QWidget;
    auto layout = new QVBoxLayout(copyrightWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    auto textBrowser = new QTextBrowser(this);
    textBrowser->setReadOnly(true);
    textBrowser->setOpenExternalLinks(true);
    textBrowser->setMarkdown(loadLocalizedMdText("Copyright"));
    layout->addWidget(textBrowser);
    copyrightWidget->setLayout(layout);
    return copyrightWidget;
}