#pragma once


#include "MappingCurves.h" 
#include "BatTypes.h"

#include <QMainWindow>

class SingleFileWidget;
class QTextEdit;
class QCheckBox;
class QCloseEvent;
class QEvent;
class AboutDialog;
class QComboBox;
class QStackedWidget;
class BatWidget;
class BatProcessor;
class QThread;


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void startBatch(const BatSettings& settings);

public slots:
    void appendLogMessage(const QString& message);
    void toggleLogWindow(bool checked);

private slots:
    void updateWindowTitle(const QString& filePath);
    void onModeChanged(int index);
    void showAboutDialog();
    void onStartBatch(const BatSettings& settings);
    void onCurveTypeChanged(int index);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();

    QComboBox* m_modeComboBox;
    QComboBox* m_curveTypeComboBox;
    QStackedWidget* m_stackedWidget;
    SingleFileWidget *m_singleFileWidget;
    BatWidget* m_batWidget;
    
    QWidget* m_logWindow;
    QTextEdit* m_logTextEdit;
    QCheckBox* m_showLogCheckBox; 

    QThread* m_processorThread;
    BatProcessor* m_processor;

    CurveType m_currentCurveType;
};