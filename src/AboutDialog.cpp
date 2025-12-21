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


AboutDialog::AboutDialog(QWidget *parent): QDialog(parent) {
    setupUi();
    setWindowTitle(tr("AudioFFT - Help"));
    setFixedSize(500, 300); 
}


QString AboutDialog::loadMdText(const QString& fileName) {
    QString path = ":/text/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("Error: Cannot load file %1").arg(path);
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
    m_tabWidget->addTab(createProgramInfoTab(), tr("About"));
    m_tabWidget->addTab(createIntroTab(), tr("Introduction"));
    m_tabWidget->addTab(createUserManualTab(), tr("User Manual"));
    m_tabWidget->addTab(createChangelogTab(), tr("Changelog"));
    m_tabWidget->addTab(createCopyrightTab(), tr("Third-party Assets"));

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

    headerLayout->setContentsMargins(50, 0, 50, 0);
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

    addInfoRow(tr("Version:"), "1.0");
    addInfoRow(tr("Build Time:"), AppConfig::getCompileTimestamp());
    addInfoRow(tr("Developer:"), "Emma Winter"); 
    addInfoRow(tr("Attribute:"), tr("Free software"));

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

    textBrowser->setMarkdown(loadMdText("Intro_EN.md"));
    
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

    textBrowser->setMarkdown(loadMdText("UserManual_EN.md"));
    
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

    textBrowser->setMarkdown(loadMdText("Changelog_EN.md"));

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

    textBrowser->setMarkdown(loadMdText("Copyright_EN.md"));

    layout->addWidget(textBrowser);
    copyrightWidget->setLayout(layout);
    return copyrightWidget;
}