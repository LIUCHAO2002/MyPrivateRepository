#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QAction>
#include <QLabel>
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

private:
    void createMenus();
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

    NetworkCanvas *m_canvas;
    QTreeWidget *m_propertyView;
    QAction *m_gridSizeAction;
    QString m_currentFileName; // 当前文件名
    bool m_isModified;         // 文件是否被修改

    QLabel* m_statusLabel;

    int m_minNodeCount;
    int m_maxNodeCount;
};

#endif // MAIN_WINDOW_H
