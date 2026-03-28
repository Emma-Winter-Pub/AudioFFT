#pragma once

#include <QDialog>

class QTabWidget;

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

private:
    void setupUi();
    QWidget* createProgramInfoTab();
    QWidget* createIntroTab();
    QWidget* createUserManualTab();
    QWidget* createChangelogTab();
    QWidget* createCopyrightTab();
    QString loadLocalizedMdText(const QString& baseName);
    QTabWidget *m_tabWidget;
    QString m_currentLangCode;
};