#include "shortcut_settings_dialog.h"
#include <QHeaderView>
#include <QMessageBox>

ShortcutSettingsDialog::ShortcutSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

ShortcutSettingsDialog::~ShortcutSettingsDialog()
{
}

void ShortcutSettingsDialog::setupUI()
{
    setWindowTitle("设置快捷键");
    setMinimumSize(500, 400);

    // 创建表格
    m_shortcutTable = new QTableWidget(this);
    m_shortcutTable->setColumnCount(3);
    m_shortcutTable->setHorizontalHeaderLabels({"功能", "当前快捷键", "修改快捷键"});
    m_shortcutTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 创建按钮
    m_restoreButton = new QPushButton("恢复默认", this);
    m_applyButton = new QPushButton("应用", this);
    m_closeButton = new QPushButton("关闭", this);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_restoreButton);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_closeButton);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_shortcutTable);
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(m_restoreButton, &QPushButton::clicked, this, &ShortcutSettingsDialog::onRestoreDefaults);
    connect(m_applyButton, &QPushButton::clicked, this, &ShortcutSettingsDialog::onApplyChanges);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ShortcutSettingsDialog::loadShortcuts(const QMap<QString, QKeySequence> &defaultShortcuts)
{
    m_defaultShortcuts = defaultShortcuts;
    m_modifiedShortcuts = defaultShortcuts;

    // 从设置中加载已保存的快捷键
    QSettings settings;
    settings.beginGroup("Shortcuts");

    m_shortcutTable->setRowCount(m_defaultShortcuts.size());

    int row = 0;
    for (auto it = m_defaultShortcuts.begin(); it != m_defaultShortcuts.end(); ++it)
    {
        QString actionName = it.key();
        QKeySequence defaultSeq = it.value();

        // 尝试从设置中获取保存的快捷键
        QKeySequence savedSeq = QKeySequence(settings.value(actionName, defaultSeq.toString()).toString());
        m_modifiedShortcuts[actionName] = savedSeq;

        // 功能名称
        QTableWidgetItem *nameItem = new QTableWidgetItem(actionName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutTable->setItem(row, 0, nameItem);

        // 当前快捷键
        QTableWidgetItem *currentItem = new QTableWidgetItem(savedSeq.toString());
        currentItem->setFlags(currentItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutTable->setItem(row, 1, currentItem);

        // 快捷键编辑控件
        QKeySequenceEdit *keyEdit = new QKeySequenceEdit(savedSeq, m_shortcutTable);
        m_shortcutTable->setCellWidget(row, 2, keyEdit);

        row++;
    }

    settings.endGroup();
}

QMap<QString, QKeySequence> ShortcutSettingsDialog::getModifiedShortcuts() const
{
    return m_modifiedShortcuts;
}

void ShortcutSettingsDialog::onRestoreDefaults()
{
    if (QMessageBox::question(this, "确认", "确定要恢复默认快捷键吗？") == QMessageBox::Yes)
    {
        for (int row = 0; row < m_shortcutTable->rowCount(); row++)
        {
            QString actionName = m_shortcutTable->item(row, 0)->text();
            QKeySequence defaultSeq = m_defaultShortcuts[actionName];

            m_shortcutTable->item(row, 1)->setText(defaultSeq.toString());

            QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit *>(
                m_shortcutTable->cellWidget(row, 2));
            if (keyEdit)
            {
                keyEdit->setKeySequence(defaultSeq);
            }
        }
    }
}

void ShortcutSettingsDialog::onApplyChanges()
{
    QSettings settings;
    settings.beginGroup("Shortcuts");

    for (int row = 0; row < m_shortcutTable->rowCount(); row++)
    {
        QString actionName = m_shortcutTable->item(row, 0)->text();

        QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit *>(
            m_shortcutTable->cellWidget(row, 2));
        if (keyEdit)
        {
            QKeySequence newSeq = keyEdit->keySequence();
            if (!newSeq.isEmpty())
            {
                m_shortcutTable->item(row, 1)->setText(newSeq.toString());
                m_modifiedShortcuts[actionName] = newSeq;
                settings.setValue(actionName, newSeq.toString());
            }
        }
    }

    settings.endGroup();
    QMessageBox::information(this, "提示", "快捷键已更新，部分修改需要重启应用生效");
}