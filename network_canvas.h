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

    // 获取节点和链路列表
    const QVector<ClientNode*>& getNodes() const { return m_nodes; }
    const QVector<Link*>& getLinks() const { return m_links; }
    
    // 添加节点和链路（用于加载文件）
    void addNodeFromFile(ClientNode* node) { m_nodes.append(node); }
    void addLink(Link* link) { m_links.append(link); }

    void generateRandomConnectedGraph(int minNodes, int maxNodes);
    
    // 清空所有数据
    void clearAll() {
        qDeleteAll(m_links);
        qDeleteAll(m_nodes);
        m_links.clear();
        m_nodes.clear();
        m_selectedNode = nullptr;
        m_selectedLink = nullptr;
        ClientNode::resetNextId();
        update();
    }

    void setDataTransferEnabled(bool enabled) { m_dataTransferEnabled = enabled; }

signals:
    void nodeSelected(ClientNode *node);
    void linkSelected(Link *link);
    void nothingSelected();
    void contentModified();

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
    ClientNode *findNodeAt(const QPoint &pos) const;
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
    bool m_dataTransferEnabled;

    QVector<ClientNode*> generateRandomNodes(int count);
    void generateSpanningTree(QVector<ClientNode*> &nodes);
    void addRandomExtraLinks(QVector<ClientNode*> &nodes, int extraCount);
};

#endif // NETWORK_CANVAS_H