#ifndef MIN_WEIGHT_COVER_PATH_H
#define MIN_WEIGHT_COVER_PATH_H

#include "path_search_algorithm.h"
#include "client_node.h"
#include "link.h"
#include <QVector>
#include <QMap>
#include <QSet>
#include <limits>

// 路径搜索算法：覆盖所有含文件块的节点，且总边权最小
class MinWeightCoverPath : public PathSearchAlgorithm
{
public:
    MinWeightCoverPath() = default;
    ~MinWeightCoverPath() override = default;

    // 初始化算法（从画布获取图数据）
    void initialize(NetworkCanvas *canvas) override;

    // 设置算法参数（如动态规划阈值、启发式开关等）
    void setParameters(const QMap<QString, QVariant> &params) override;

    // 执行搜索并返回最佳路径
    QVector<ClientNode *> findBestPath() override;

    // 获取搜索结果详情（总权重、节点数等）
    QMap<QString, QVariant> getResultDetails() const override;

private:
    // 图结构定义
    struct Edge
    {
        ClientNode *to;
        double weight; // 边权重（越小越好）
        Edge(ClientNode *t, double w) : to(t), weight(w) {}
    };

    // 目标节点：含文件块数量不为0的节点
    QVector<ClientNode *> m_targetNodes;
    // 所有节点
    QVector<ClientNode *> m_allNodes;
    // 邻接表
    QMap<ClientNode *, QVector<Edge>> m_adjacency;
    // 画布指针
    NetworkCanvas *m_canvas = nullptr;
    // 算法参数
    int m_dpThreshold = 1024; // 动态规划阈值（目标节点数超过此值则启用启发式）
    // 结果详情
    QMap<QString, QVariant> m_resultDetails;

    // 辅助函数：判断节点是否为目标节点（含文件块）
    bool isTargetNode(ClientNode *node) const
    {
        return !node->storedBlocks().isEmpty();
    }

    // Dijkstra算法：计算两点间最短路径
    QVector<ClientNode *> dijkstra(ClientNode *start, ClientNode *end, double &totalWeight);

    // 构建目标节点间的最短路径矩阵
    QVector<QVector<double>> buildDistanceMatrix(
        const QVector<ClientNode *> &targets,
        QMap<ClientNode *, int> &nodeIndexMap);

    // 动态规划（Held-Karp）求解TSP问题（适用于目标节点少的情况）
    QVector<int> heldKarp(const QVector<QVector<double>> &distMatrix);

    // 启发式算法（贪心+2opt优化）求解TSP（适用于目标节点多的情况）
    QVector<int> greedy2Opt(const QVector<QVector<double>> &distMatrix);

    // 拼接路径：将目标节点访问顺序转换为完整路径
    QVector<ClientNode *> stitchPath(
        const QVector<ClientNode *> &targets,
        const QVector<int> &order,
        const QMap<QPair<ClientNode *, ClientNode *>, QVector<ClientNode *>> &shortPaths);
};

#endif // MIN_WEIGHT_COVER_PATH_H