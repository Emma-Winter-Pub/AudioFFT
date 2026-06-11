#pragma once

#include <QDialog>

class QComboBox;
class QSlider;
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QPushButton;

class SingleFileExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit SingleFileExportDialog(
        const QString& defaultPath, 
        const QString& defaultBaseName, 
        bool isJpegAvailable,
        bool isWebpAvailable,
        bool isAvifAvailable,
        QWidget *parent = nullptr
    );
    QString getSelectedFormatIdentifier() const;
    QString getSelectedFilePath() const;
    int qualityLevel() const;

private slots:
    void onFormatChanged(int index);
    void onBrowsePath();
    void onAccept();

private:
    void setupUi(bool isJpegAvailable, bool isWebpAvailable, bool isAvifAvailable);
    QLineEdit* m_pathLineEdit;
    QLineEdit* m_fileNameLineEdit;
    QPushButton* m_browseButton;
    QComboBox* m_formatComboBox;
    QLabel* m_qualityLabel;
    QSlider* m_qualitySlider;
    QLabel* m_qualityValueLabel;
    QLabel* m_tooltipLabel; 
    QDialogButtonBox* m_buttonBox;
};