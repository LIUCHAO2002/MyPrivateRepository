#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QAction>
#include <QLabel>
#include <QShortcut>
#include "network_canvas.h"
#include "client_node.h"
#include "link.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNodeSelected(ClientNode *node);
    void onLinkSelected(Link *link);
    void onNothingSelected();
    void onGridSizeChanged();
    void onItemEdited(QTreeWidgetItem *item, int column);
    void onSaveFile();   // 保存文件
    void onSaveAsFile(); // 另存为
    void onOpenFile();   // 打开文件
    void onClearAll();
    void onRefreshGraph();
    void onExportTrainingData();
    void onFindOptimalPath();
    void onSimulationSettings();
    void onShortcutSettings();
    void onShowShortcutHelp();

private:
    void createMenus();
    void createShortcut();
    void updatePropertyView(ClientNode *node);
    void updatePropertyView(Link *link);
    void clearPropertyView();
    bool saveFile(const QString &fileName); // 实际保存函数
    bool loadFile(const QString &fileName); // 加载函数
    bool maybeSave();                       // 退出前检查是否需要保存
    bool exportDataToJson(const QString &fileName);
    bool isGraphConnected();
    QString defaultSaveDirectory() const;
    QString defaultTrainDataDirectory() const;
    int fileCount() const { return m_fileCount; }
    int blockPerFile() const { return m_blockPerFile; }
    bool exportNodeFeatureMatrix();
    void loadSettings(); // 加载配置
    void saveSettings(); // 保存配置

    NetworkCanvas *m_canvas;
    QTreeWidget *m_propertyView;
    QAction *m_gridSizeAction;
    QString m_currentFileName; // 当前文件名
    bool m_isModified;         // 文件是否被修改

    QLabel *m_statusLabel;

    int m_minNodeCount;
    int m_maxNodeCount;

    int m_fileCount = 10;
    int m_blockPerFile = 8;
    bool m_randomBlockDistribution = false;
    int m_minReplicaCount = 1;
    int m_maxReplicaCount = 2;

    bool m_autoAnnotateEnabled;

    void updateShortcuts();
    QMap<QString, QShortcut *> m_shortcuts;
};

#endif // MAIN_WINDOW_H
