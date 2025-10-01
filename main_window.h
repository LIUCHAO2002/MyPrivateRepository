#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QAction>
#include "network_canvas.h"
#include "client_node.h"
#include "link.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNodeSelected(ClientNode* node);
    void onLinkSelected(Link* link);
    void onNothingSelected();
    void onGridSizeChanged();
    void onItemEdited(QTreeWidgetItem* item, int column);

private:
    void createMenus();
    void updatePropertyView(ClientNode* node);
    void updatePropertyView(Link* link);
    void clearPropertyView();

    NetworkCanvas* m_canvas;
    QTreeWidget* m_propertyView;
    QAction* m_gridSizeAction;
};

#endif // MAIN_WINDOW_H
    