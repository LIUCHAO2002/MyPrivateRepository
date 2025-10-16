#ifndef SHORTCUT_SETTINGS_DIALOG_H
#define SHORTCUT_SETTINGS_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QSettings>
#include <QMap>
#include <QAction>

class ShortcutSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutSettingsDialog(QWidget *parent = nullptr);
    ~ShortcutSettingsDialog() override;

    void loadShortcuts(const QMap<QString, QKeySequence> &defaultShortcuts);
    QMap<QString, QKeySequence> getModifiedShortcuts() const;

private slots:
    void onRestoreDefaults();
    void onApplyChanges();

private:
    QTableWidget *m_shortcutTable;
    QPushButton *m_restoreButton;
    QPushButton *m_applyButton;
    QPushButton *m_closeButton;
    QMap<QString, QKeySequence> m_defaultShortcuts;
    QMap<QString, QKeySequence> m_modifiedShortcuts;

    void setupUI();
};

#endif // SHORTCUT_SETTINGS_DIALOG_H