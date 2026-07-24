#include "AudioExtensionWhitelistDialog.h"
#include "AudioExtensionList.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QScrollArea>
#include <QToolButton>
#include <QRegularExpression>
#include <QMessageBox>
#include <QFontMetrics>
#include <QLayout>
#include <algorithm>

namespace {
    class FlowLayout : public QLayout {
    public:
        explicit FlowLayout(int margin = 0, int hSpacing = 3, int vSpacing = 2)
            : m_hSpace(hSpacing), m_vSpace(vSpacing) {
            setContentsMargins(margin, margin, margin, margin);
        }
        ~FlowLayout() override {
            QLayoutItem *item;
            while ((item = takeAt(0))) delete item;
        }
        void addItem(QLayoutItem *item) override { itemList.append(item); }
        int horizontalSpacing() const { return m_hSpace; }
        int verticalSpacing() const { return m_vSpace; }
        int count() const override { return itemList.size(); }
        QLayoutItem *itemAt(int index) const override { return itemList.value(index); }
        QLayoutItem *takeAt(int index) override {
            if (index >= 0 && index < itemList.size()) return itemList.takeAt(index);
            return nullptr;
        }
        Qt::Orientations expandingDirections() const override { return { }; }
        bool hasHeightForWidth() const override { return true; }
        int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
        void setGeometry(const QRect &rect) override {
            QLayout::setGeometry(rect);
            doLayout(rect, false);
        }
        QSize sizeHint() const override { return minimumSize(); }
        QSize minimumSize() const override {
            QSize size;
            for (QLayoutItem *item : itemList) size = size.expandedTo(item->minimumSize());
            int left, top, right, bottom;
            getContentsMargins(&left, &top, &right, &bottom);
            size += QSize(left + right, top + bottom);
            return size;
        }
    private:
        int doLayout(const QRect &rect, bool testOnly) const {
            int left, top, right, bottom;
            getContentsMargins(&left, &top, &right, &bottom);
            QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
            int x = effectiveRect.x();
            int y = effectiveRect.y();
            int lineHeight = 0;
            for (QLayoutItem *item : itemList) {
                int spaceX = horizontalSpacing();
                int spaceY = verticalSpacing();
                int nextX = x + item->sizeHint().width() + spaceX;
                if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                    x = effectiveRect.x();
                    y = y + lineHeight + spaceY;
                    nextX = x + item->sizeHint().width() + spaceX;
                    lineHeight = 0;
                }
                if (!testOnly) item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
                x = nextX;
                lineHeight = qMax(lineHeight, item->sizeHint().height());
            }
            return y + lineHeight - rect.y() + bottom;
        }
        QList<QLayoutItem*> itemList;
        int m_hSpace;
        int m_vSpace;
    };
}

AudioExtensionWhitelistDialog::AudioExtensionWhitelistDialog(const QStringList& currentExtensions, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("配置扩展名白名单"));
    setMinimumSize(350, 480);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    calculateDynamicWidths();
    QStringList defaults = AudioExtensionList::getDefaultExtensions();
    for (const QString& ext : currentExtensions) {
        if (!defaults.contains(ext, Qt::CaseInsensitive)) {
            m_customExtStates.insert(ext.toLower(), true);
        }
    }
    setupUi();
    populateBuiltInData(currentExtensions);
    rebuildCustomPanel();
    updateStats();
}

AudioExtensionWhitelistDialog::~AudioExtensionWhitelistDialog() {
    for (auto group : m_builtInGroups) {
        for (auto item : group->items) delete item;
        delete group;
    }
    clearCustomPanel();
}

void AudioExtensionWhitelistDialog::calculateDynamicWidths() {
    QFontMetrics fm(font());
    int maxLen = 0;
    QString longestStr = "WWWW";
    QStringList defaults = AudioExtensionList::getDefaultExtensions();
    for (const QString& ext : defaults) {
        if (ext.length() > maxLen) {
            maxLen = ext.length();
            longestStr = ext;
        }
    }
    m_chipWidth = fm.horizontalAdvance(longestStr.toUpper()) + 10;
    m_customChipWidth = m_chipWidth + 20;
}

void AudioExtensionWhitelistDialog::setupUi() {
    setStyleSheet(R"(
        QDialog { 
            background-color: #3B4453; 
            color: #E0E0E0; 
        }
        QLabel { 
            color: #E0E0E0; 
        }
        QLabel.group-header { 
            color: #8899A6; 
            margin-top: 4px; 
            margin-bottom: 2px; 
        }
        QLineEdit { 
            background-color: rgb(78, 90, 110); 
            border: 1px solid #2B333E; 
            color: #E0E0E0; 
            padding: 2px; 
            border-radius: 0px;
        }
        QSplitter::handle { 
            background-color: #2B333E; 
        }
        QScrollArea { 
            border: 1px solid #2B333E; 
            background-color: rgb(47, 54, 66); 
            border-radius: 0px;
        }
        QScrollArea > QWidget > QWidget { 
            background-color: rgb(47, 54, 66); 
        }
        QToolButton { 
            background: rgb(78, 90, 110); 
            border: 1px solid #2B333E; 
            padding: 0px; 
            color: #E0E0E0; 
            border-radius: 0px;
        }
        QToolButton:hover { 
            background: rgb(60, 70, 90); 
        }
        QToolButton:checked { 
            background: #4A90E2; 
            border: 1px solid #4A90E2; 
            color: #FFFFFF; 
        }
        QToolButton.delete-btn {
            background: rgb(78, 90, 110);
            border: 1px solid #2B333E;
            color: #8899A6;
            border-radius: 0px;
        }
        QToolButton.delete-btn:hover {
            color: #FF5555;
            background: rgba(255, 85, 85, 0.2);
            border: 1px solid #FF5555;
        }
        QPushButton { 
            background: rgb(78, 90, 110); 
            border: 1px solid #1C222B; 
            min-width: 60px; 
            min-height: 22px; 
            padding: 0 10px; 
            color: #E0E0E0; 
            border-radius: 0px;
        }
        QPushButton:hover { background: rgb(55, 65, 81); }
        QPushButton:pressed { background: rgb(30, 36, 45); }
        QPushButton.primary-btn {
            background: #4A90E2;
            border: 1px solid #357ABD;
            color: white;
        }
        QPushButton.primary-btn:hover { background: #357ABD; }
        QPushButton.primary-btn:pressed { background: #285E8E; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);

    // --- 顶部搜索栏 ---
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(2, 2, 2, 2);
    topLayout->setSpacing(4);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索"));
    m_searchEdit->setClearButtonEnabled(true);
    m_statsLabel = new QLabel(this);
    topLayout->addWidget(m_searchEdit, 1);
    topLayout->addSpacing(40);
    topLayout->addWidget(m_statsLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addLayout(topLayout);


    // --- 中部双面板 ---
    m_splitter = new QSplitter(Qt::Vertical, this);
    
    // 内置扩展名
    QWidget* topPanel = new QWidget(this);
    QVBoxLayout* topPanelLayout = new QVBoxLayout(topPanel);
    topPanelLayout->setContentsMargins(0, 0, 0, 0);
    topPanelLayout->setSpacing(2);
    QLabel* topTitle = new QLabel(tr("内置扩展名"), this);
    topTitle->setStyleSheet("margin-bottom: 2px;");
    topPanelLayout->addWidget(topTitle);
    QScrollArea* topScroll = new QScrollArea(this);
    topScroll->setWidgetResizable(true);
    m_builtInWidget = new QWidget(topScroll);
    m_builtInLayout = new QVBoxLayout(m_builtInWidget);
    m_builtInLayout->setContentsMargins(5, 2, 5, 5);
    m_builtInLayout->setSpacing(2);
    topScroll->setWidget(m_builtInWidget);
    topPanelLayout->addWidget(topScroll);

    // 自定义扩展名
    QWidget* bottomPanel = new QWidget(this);
    QVBoxLayout* bottomPanelLayout = new QVBoxLayout(bottomPanel);
    bottomPanelLayout->setContentsMargins(0, 0, 0, 0);
    bottomPanelLayout->setSpacing(2);
    QLabel* bottomTitle = new QLabel(tr("自定义扩展名"), this);
    bottomTitle->setStyleSheet("margin-bottom: 2px; margin-top: 4px;");
    bottomPanelLayout->addWidget(bottomTitle);
    QScrollArea* bottomScroll = new QScrollArea(this);
    bottomScroll->setWidgetResizable(true);
    m_customWidget = new QWidget(bottomScroll);
    m_customLayout = new QVBoxLayout(m_customWidget);
    m_customLayout->setContentsMargins(5, 2, 5, 5);
    m_customLayout->setSpacing(2); 
    m_customLayout->addStretch(1); 
    bottomScroll->setWidget(m_customWidget);
    bottomPanelLayout->addWidget(bottomScroll);

    // 自定义输入区
    QHBoxLayout* addCustomLayout = new QHBoxLayout();
    addCustomLayout->setContentsMargins(0, 2, 0, 0);
    addCustomLayout->setSpacing(2);
    m_customExtEdit = new QLineEdit(this);
    m_customExtEdit->setPlaceholderText(tr("输入自定义扩展名"));
    QPushButton* btnAdd = new QPushButton(tr("+ 添加"), this);
    addCustomLayout->addWidget(m_customExtEdit);
    addCustomLayout->addWidget(btnAdd);
    bottomPanelLayout->addLayout(addCustomLayout);
    m_splitter->addWidget(topPanel);
    m_splitter->addWidget(bottomPanel);
    m_splitter->setStretchFactor(0, 8);
    m_splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(m_splitter, 1);

    // --- 底部按钮 ---
    QHBoxLayout* batchActionLayout = new QHBoxLayout();
    batchActionLayout->setContentsMargins(0, 5, 0, 10);
    batchActionLayout->setSpacing(5);
    QPushButton* btnSelectAll = new QPushButton(tr("全选"), this);
    QPushButton* btnClear = new QPushButton(tr("清空"), this);
    QPushButton* btnInvert = new QPushButton(tr("反选"), this);
    QPushButton* btnRestore = new QPushButton(tr("恢复默认"), this);
    batchActionLayout->addWidget(btnSelectAll);
    batchActionLayout->addWidget(btnClear);
    batchActionLayout->addWidget(btnInvert);
    batchActionLayout->addWidget(btnRestore);
    batchActionLayout->addStretch();
    
    mainLayout->addLayout(batchActionLayout);

    // --- 底部按钮 ---
    QHBoxLayout* dialogButtonLayout = new QHBoxLayout();
    dialogButtonLayout->setContentsMargins(0, 5, 0, 0);
    dialogButtonLayout->setSpacing(5);
    QPushButton* btnOk = new QPushButton(tr("确定"), this);
    QPushButton* btnCancel = new QPushButton(tr("取消"), this);

    dialogButtonLayout->addStretch();
    dialogButtonLayout->addWidget(btnOk);
    dialogButtonLayout->addWidget(btnCancel);
    mainLayout->addLayout(dialogButtonLayout);

    // --- 信号 ---
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AudioExtensionWhitelistDialog::onSearchTextChanged);
    connect(btnAdd, &QPushButton::clicked, this, &AudioExtensionWhitelistDialog::onAddCustomExtension);
    m_customExtEdit->installEventFilter(this);
    m_searchEdit->installEventFilter(this);
    connect(btnSelectAll, &QPushButton::clicked, this, &AudioExtensionWhitelistDialog::onSelectAllBuiltIn);
    connect(btnClear, &QPushButton::clicked, this, &AudioExtensionWhitelistDialog::onClearBuiltIn);
    connect(btnInvert, &QPushButton::clicked, this, &AudioExtensionWhitelistDialog::onInvertBuiltIn);
    connect(btnRestore, &QPushButton::clicked, this, &AudioExtensionWhitelistDialog::onRestoreDefaults);
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void AudioExtensionWhitelistDialog::populateBuiltInData(const QStringList& currentExtensions) {
    QStringList defaults = AudioExtensionList::getDefaultExtensions();
    defaults.sort(Qt::CaseInsensitive);
    QMap<QChar, QStringList> groupedDefaults;
    for (const QString& ext : defaults) {
        if (ext.isEmpty()) continue;
        QChar firstLetter = ext.at(0).toUpper();
        if (firstLetter.isDigit()) {
            groupedDefaults[QChar('#')].append(ext);
        } else {
            groupedDefaults[firstLetter].append(ext);
        }
    }
    for (auto it = groupedDefaults.constBegin(); it != groupedDefaults.constEnd(); ++it) {
        ExtensionGroup* group = new ExtensionGroup();
        group->letter = it.key();
        QString headerText = (it.key() == QChar('#')) ? tr("0-9") : QString(it.key());
        group->headerLabel = new QLabel(headerText, m_builtInWidget);
        group->headerLabel->setProperty("class", "group-header");
        m_builtInLayout->addWidget(group->headerLabel);
        group->gridContainer = new QWidget(m_builtInWidget);
        FlowLayout* flowLayout = new FlowLayout(0, 3, 2);
        group->gridContainer->setLayout(flowLayout);
        for (const QString& ext : it.value()) {
            ExtensionItem* item = new ExtensionItem();
            item->ext = ext;
            item->isBuiltIn = true;
            item->btn = new QToolButton(group->gridContainer);
            item->btn->setText(ext.toUpper());
            item->btn->setCheckable(true);
            item->btn->setFixedSize(m_chipWidth, CHIP_HEIGHT);
            item->btn->setCursor(Qt::PointingHandCursor);
            if (currentExtensions.contains(ext, Qt::CaseInsensitive)) {
                item->btn->setChecked(true);
            }
            connect(item->btn, &QToolButton::toggled, this, &AudioExtensionWhitelistDialog::updateStats);

            flowLayout->addWidget(item->btn);
            group->items.append(item);
        }
        m_builtInLayout->addWidget(group->gridContainer);
        m_builtInGroups.append(group);
    }
    m_builtInLayout->addStretch(1);
}

void AudioExtensionWhitelistDialog::rebuildCustomPanel() {
    clearCustomPanel();
    QStringList customExts = m_customExtStates.keys();
    if (customExts.isEmpty()) {
        m_customLayout->addStretch(1);
        return;
    }
    customExts.sort(Qt::CaseInsensitive);
    ExtensionGroup* group = new ExtensionGroup();
    group->letter = ' ';
    group->headerLabel = new QLabel("", m_customWidget);
    group->headerLabel->hide();
    m_customLayout->addWidget(group->headerLabel);
    group->gridContainer = new QWidget(m_customWidget);
    FlowLayout* flowLayout = new FlowLayout(0, 3, 2);
    group->gridContainer->setLayout(flowLayout);
    for (const QString& ext : customExts) {
        ExtensionItem* item = new ExtensionItem();
        item->ext = ext;
        item->isBuiltIn = false;
        item->container = new QWidget(group->gridContainer);
        item->container->setFixedSize(m_customChipWidth, CHIP_HEIGHT);
        QHBoxLayout* hl = new QHBoxLayout(item->container);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(1);
        item->btn = new QToolButton(item->container);
        item->btn->setText(ext.toUpper());
        item->btn->setCheckable(true);
        item->btn->setChecked(m_customExtStates.value(ext, true));
        item->btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        item->btn->setFixedSize(m_chipWidth, CHIP_HEIGHT);
        item->btn->setCursor(Qt::PointingHandCursor);
        QToolButton* delBtn = new QToolButton(item->container);
        delBtn->setText("×");
        delBtn->setProperty("class", "delete-btn");
        delBtn->setFixedSize(18, CHIP_HEIGHT);
        delBtn->setCursor(Qt::PointingHandCursor);
        hl->addWidget(item->btn);
        hl->addWidget(delBtn);
        connect(item->btn, &QToolButton::toggled, this, [this, ext](bool checked){
            m_customExtStates[ext] = checked;
            updateStats();
        });
        connect(delBtn, &QToolButton::clicked, this, [this, ext](){
            m_customExtStates.remove(ext);
            rebuildCustomPanel();
            updateStats();
            onSearchTextChanged(m_searchEdit->text());
        });
        flowLayout->addWidget(item->container);
        group->items.append(item);
    }
    m_customLayout->addWidget(group->gridContainer);
    m_customGroups.append(group);
    m_customLayout->addStretch(1);
}

void AudioExtensionWhitelistDialog::clearCustomPanel() {
    QLayoutItem* child;
    while ((child = m_customLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    for (auto group : m_customGroups) {
        for (auto item : group->items) {
            delete item;
        }
        delete group;
    }
    m_customGroups.clear();
}

void AudioExtensionWhitelistDialog::onSearchTextChanged(const QString& text) {
    QString filter = text.trimmed().toLower();
    auto filterGroups = [&](QList<ExtensionGroup*>& groups) {
        for (ExtensionGroup* group : groups) {
            int visibleCount = 0;
            for (ExtensionItem* item : group->items) {
                bool match = filter.isEmpty() || item->ext.toLower().contains(filter);
                if (item->container) {
                    item->container->setVisible(match);
                } else {
                    item->btn->setVisible(match);
                }
                if (match) visibleCount++;
            }
            bool groupVisible = (visibleCount > 0);
            if (group->headerLabel) {
                group->headerLabel->setVisible(groupVisible);
            }
            group->gridContainer->setVisible(groupVisible);
        }
    };
    filterGroups(m_builtInGroups);
    filterGroups(m_customGroups);
}

void AudioExtensionWhitelistDialog::onAddCustomExtension() {
    QString ext = m_customExtEdit->text().trimmed().toLower();
    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }
    if (ext.isEmpty()) return;
    QRegularExpression re("^[a-z0-9]+$");
    if (!re.match(ext).hasMatch()) {
        QMessageBox::warning(this, tr("输入错误"), tr("扩展名仅支持输入英文字母和数字。"));
        return;
    }
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            if (item->ext.compare(ext, Qt::CaseInsensitive) == 0) {
                item->btn->setChecked(true);
                m_customExtEdit->clear();
                updateStats();
                return;
            }
        }
    }
    m_customExtStates.insert(ext, true);
    rebuildCustomPanel();
    m_customExtEdit->clear();
    updateStats();
    onSearchTextChanged(m_searchEdit->text());
}

void AudioExtensionWhitelistDialog::updateStats() {
    int builtInTotal = 0;
    int builtInChecked = 0;
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            builtInTotal++;
            if (item->btn->isChecked()) builtInChecked++;
        }
    }
    int customTotal = m_customExtStates.size();
    int customChecked = 0;
    for (bool state : m_customExtStates.values()) {
        if (state) customChecked++;
    }
    m_statsLabel->setText(QString(tr("内置：%1/%2    自定义：%3/%4"))
        .arg(builtInChecked)
        .arg(builtInTotal)
        .arg(customChecked)
        .arg(customTotal));
}

void AudioExtensionWhitelistDialog::onSelectAllBuiltIn() {
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            item->btn->setChecked(true);
        }
    }
    updateStats();
}

void AudioExtensionWhitelistDialog::onClearBuiltIn() {
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            item->btn->setChecked(false);
        }
    }
    updateStats();
}

void AudioExtensionWhitelistDialog::onInvertBuiltIn() {
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            item->btn->setChecked(!item->btn->isChecked());
        }
    }
    updateStats();
}

void AudioExtensionWhitelistDialog::onRestoreDefaults() {
    onSelectAllBuiltIn();
    m_customExtStates.clear();
    rebuildCustomPanel();
    updateStats();
}

QStringList AudioExtensionWhitelistDialog::getSelectedExtensions() const {
    QStringList selected;
    for (ExtensionGroup* group : m_builtInGroups) {
        for (ExtensionItem* item : group->items) {
            if (item->btn->isChecked()) {
                selected.append(item->ext);
            }
        }
    }
    for (auto it = m_customExtStates.constBegin(); it != m_customExtStates.constEnd(); ++it) {
        if (it.value()) {
            selected.append(it.key());
        }
    }
    return selected;
}

bool AudioExtensionWhitelistDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (obj == m_customExtEdit) {
                onAddCustomExtension();
                return true;
            } 
            else if (obj == m_searchEdit) {
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}