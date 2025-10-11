#ifndef STEINER_PATH_FINDER_H
#define STEINER_PATH_FINDER_H

#include <QVector>
#include <QMap>
#include <QSet>
#include "client_node.h"
#include "link.h"

class SteinerPathFinder
{
public:
    explicit SteinerPathFinder(const QMap<ClientNode *, QVector<Link *>> &adj);

    /** 设置终端（dataRatio>0） */
    void setTerminals(const QList<ClientNode *> &terminals);

    /** 返回最佳路径（Steiner 链） */
    QVector<ClientNode *> findBestPath();

    /** 导出训练数据：边级 (u,v,cost,label) */
    struct EdgeLabel
    {
        ClientNode *u;
        ClientNode *v;
        double cost;
        int label;
    };
    QVector<EdgeLabel> exportLabels(int negPerPos = 3);

private:
    const QMap<ClientNode *, QVector<Link *>> &m_adj;
    QList<ClientNode *> m_terminals;

    static double linkCost(const Link *l);
    static QVector<ClientNode *> bidirectionalAstar(
        ClientNode *start, ClientNode *end,
        const QMap<ClientNode *, QVector<Link *>> &adj);
    static QVector<QPair<ClientNode *, ClientNode *>>
    steinerChainEdges(const QList<ClientNode *> &T,
                      const QMap<ClientNode *, QVector<Link *>> &adj);
};

#endif // STEINER_PATH_FINDER_H