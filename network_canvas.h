#ifndef NETWORK_CANVAS_H
#define NETWORK_CANVAS_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPair>
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
    const QVector<ClientNode *> &getNodes() const { return m_nodes; }
    const QVector<Link *> &getLinks() const { return m_links; }

    // 添加节点和链路（用于加载文件）
    void addNodeFromFile(ClientNode *node) { m_nodes.append(node); }
    void addLink(Link *link) { m_links.append(link); }

    void generateRandomConnectedGraph(int minNodes, int maxNodes,
                                      int fileCount, int blocksPerFile,
                                      int minReplica, int maxReplica,
                                      bool randomBlockSize);

    // 清空所有数据
    void clearAll()
    {
        qDeleteAll(m_links);
        qDeleteAll(m_nodes);
        m_links.clear();
        m_nodes.clear();
        m_selectedNode = nullptr;
        m_selectedLink = nullptr;
        m_selectedNodes.clear();
        m_selectedLinks.clear();
        ClientNode::resetNextId();
        update();
    }

    void setDataTransferEnabled(bool enabled) { m_dataTransferEnabled = enabled; }

    double getDirectedEdgeWeight(ClientNode *source, ClientNode *target) const;
    void clearEdgeWeightCache() { m_directedEdgeWeights.clear(); }

    void setAutoLabeling(bool enabled) { m_autoLabeling = enabled; }

    void setBestPath(const QVector<ClientNode *> &path);
    void clearBestPath()
    {
        m_selectedNodes.clear();
        m_selectedLinks.clear();
    };
    const QVector<ClientNode *> &bestPathNode() const { return m_selectedNodes; }
    const QVector<Link *> &bestPathLink() const { return m_selectedLinks; }

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
    bool m_autoLabeling = false;
    QVector<ClientNode *> m_bestPath;
    QVector<ClientNode *> m_selectedNodes;
    QVector<Link *> m_selectedLinks;

    QVector<ClientNode *> generateRandomNodes(int count);
    void generateSpanningTree(QVector<ClientNode *> &nodes);
    void addRandomExtraLinks(QVector<ClientNode *> &nodes, int extraCount);
    void allocateRandomDataBlocks(const QVector<ClientNode *> &nodes,
                                  int fileCount,
                                  int blocksPerFile,
                                  int minReplica,
                                  int maxReplica,
                                  bool randomBlockSize);

    mutable QMap<QPair<ClientNode *, ClientNode *>, double> m_directedEdgeWeights;
    // 归一化参数（缓存节点/链路属性的最大值，用于归一化）
    mutable double m_maxProcessingCapability = 0;
    mutable double m_maxBandwidth = 0;
    mutable double m_maxDistance = 0;

    // 初始化归一化参数（首次计算权重时调用）
    void initNormalizationParams() const;
    // 计算单条有向边的权重
    double calculateDirectedEdgeWeight(ClientNode *source, ClientNode *target, Link *link) const;

    bool isNodeSelected(ClientNode *node) const { return m_selectedNodes.contains(node); }
    bool isLinkSelected(Link *link) const { return m_selectedLinks.contains(link); }
};

#endif // NETWORK_CANVAS_H