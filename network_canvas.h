#ifndef NETWORK_CANVAS_H
#define NETWORK_CANVAS_H

#include <QWidget>
#include <QVector>
#include "client_node.h"
#include "link.h"

class NetworkCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkCanvas(QWidget *parent = nullptr);
    ~NetworkCanvas() override;

    void setGridSize(int size);
    int gridSize() const { return m_gridSize; }

    // 获取选中节点和链路
    ClientNode *selectedNode() const { return m_selectedNode; }
    Link *selectedLink() const { return m_selectedLink; }

signals:
    void nodeSelected(ClientNode *node);
    void linkSelected(Link *link);
    void nothingSelected();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void addNode();

private:
    QPoint snapToGrid(const QPoint &pos) const;
    ClientNode *findNodeAt(const QPoint &gridPos) const;
    Link *findLinkAt(const QPoint &pos) const;
    double distance(const QPoint &p1, const QPoint &p2) const;

    int m_gridSize;
    QVector<ClientNode *> m_nodes;
    QVector<Link *> m_links;
    ClientNode *m_selectedNode;
    Link *m_selectedLink;
    bool m_draggingNode;
    ClientNode *m_draggedNode;
    ClientNode *m_linkStartNode;
};

#endif // NETWORK_CANVAS_H