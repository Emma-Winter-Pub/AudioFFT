#pragma once

#include <QDialog>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QKeyEvent>

class QLineEdit;
class QLabel;
class QSplitter;
class QVBoxLayout;
class QGridLayout;
class QToolButton;
class QScrollArea;

struct ExtensionItem {
    QString ext;
    QToolButton* btn = nullptr;
    QWidget* container = nullptr; 
    bool isBuiltIn = true;
};

struct ExtensionGroup {
    QChar letter;
    QLabel* headerLabel = nullptr;
    QWidget* gridContainer = nullptr;
    QGridLayout* layout = nullptr;
    QList<ExtensionItem*> items;
};

class AudioExtensionWhitelistDialog : public QDialog {
    Q_OBJECT

public:
    explicit AudioExtensionWhitelistDialog(const QStringList& currentExtensions, QWidget* parent = nullptr);
    ~AudioExtensionWhitelistDialog() override;
    QStringList getSelectedExtensions() const;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onAddCustomExtension();
    void updateStats();
    void onSelectAllBuiltIn();
    void onClearBuiltIn();
    void onInvertBuiltIn();
    void onRestoreDefaults();

private:
    void setupUi();
    void calculateDynamicWidths();
    void populateBuiltInData(const QStringList& currentExtensions);
    void rebuildCustomPanel();
    void clearCustomPanel();
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_statsLabel = nullptr;
    QSplitter* m_splitter = nullptr;
    QWidget* m_builtInWidget = nullptr;
    QVBoxLayout* m_builtInLayout = nullptr;
    QWidget* m_customWidget = nullptr;
    QVBoxLayout* m_customLayout = nullptr;
    QLineEdit* m_customExtEdit = nullptr;
    QList<ExtensionGroup*> m_builtInGroups;
    QList<ExtensionGroup*> m_customGroups;
    QMap<QString, bool> m_customExtStates;
    int m_chipWidth = 60;
    int m_customChipWidth = 80;
    const int CHIP_HEIGHT = 18;
    const int COLUMNS = 5;
    bool m_isAdding = false;
};