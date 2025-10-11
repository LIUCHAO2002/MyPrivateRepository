#ifndef BIDIRECTIONAL_ASTAR_PATH_FINDER_H
#define BIDIRECTIONAL_ASTAR_PATH_FINDER_H

#include <QVector>
#include <QMap>
#include <QSet>
#include <limits>
#include "client_node.h"
#include "link.h"
#include "network_canvas.h"
#include "path_search_algorithm.h"

struct PathResult
{
    QVector<ClientNode *> path; // 最优路径
    double totalCost;           // 路径权值和
    int visited;                // 算法实际访问节点数（调试用）
};

class BidirectionalAstarPathFinder
{
public:
    explicit BidirectionalAstarPathFinder(NetworkCanvas *canvas);

    // 完全兼容原 MonteCarlo 接口
    void setIterations(int)
    { /* 本算法无需迭代 */
    }
    void setWeights(double, double)
    { /* 仅保留接口 */
    }
    QVector<ClientNode *> findBestPath();
    const PathResult &getLastResult() const { return m_result; }

private:
    NetworkCanvas *m_canvas;
    QVector<ClientNode *> m_targets;
    QMap<ClientNode *, QVector<QPair<ClientNode *, double>>> m_adj; // 邻接表 + 权值

    PathResult m_result;

    void buildGraph();
    QVector<ClientNode *> reconstructPath(QMap<ClientNode *, ClientNode *> &prev,
                                          ClientNode *start, ClientNode *end) const;
    double heuristic(ClientNode *a, ClientNode *b) const; // 可采纳下界
    PathResult bidirectionalAstar(ClientNode *s, ClientNode *t);
};
#endif