#ifndef MONTE_CARLO_PATH_FINDER_H
#define MONTE_CARLO_PATH_FINDER_H

#include <QVector>
#include <QMap>
#include <QSet>
#include <limits>
#include "client_node.h"
#include "link.h"
#include "network_canvas.h"

// 路径评估结果结构体（用于存储路径的各项指标）
struct PathEvaluation
{
    QVector<ClientNode *> path; // 路径节点序列
    int nodeCount;              // 节点数量
    double totalCost;           // 总链路成本
    double score;               // 综合评分（越低越好）
};

class MonteCarloPathFinder
{
public:
    // 构造函数：接收网络画布对象（包含所有节点和链路）
    explicit MonteCarloPathFinder(NetworkCanvas *canvas);

    // 设置迭代次数（默认10000次，可根据精度需求调整）
    void setIterations(int iterations) { m_iterations = iterations; }

    // 设置评估权重（节点数量权重 + 链路成本权重 = 1.0）
    void setWeights(double nodeWeight, double linkWeight);

    // 核心接口：执行搜索并返回最佳路径
    QVector<ClientNode *> findBestPath();

    // 获取最后一次搜索的评估结果（用于调试或展示详细信息）
    const PathEvaluation &getLastEvaluation() const { return m_bestEval; }

private:
    NetworkCanvas *m_canvas;                                     // 网络画布指针（不负责内存管理）
    QVector<ClientNode *> m_targetNodes;                         // 目标节点集合（dataRatio() > 0的节点）
    QMap<ClientNode *, QVector<ClientNode *>> m_adjacencyList;   // 图的邻接表
    QMap<QPair<ClientNode *, ClientNode *>, double> m_linkCosts; // 链路成本缓存
    int m_iterations;                                            // 蒙特卡洛迭代次数
    double m_nodeWeight;                                         // 节点数量在评分中的权重
    double m_linkWeight;                                         // 链路成本在评分中的权重
    PathEvaluation m_bestEval;                                   // 最佳路径的评估结果

    // 构建图数据（邻接表和链路成本）
    void buildGraphData();

    // 生成一条随机路径（蒙特卡洛核心步骤）
    PathEvaluation generateRandomPath() const;

    // 计算路径的总链路成本
    double calculatePathCost(const QVector<ClientNode *> &path) const;

    // 基于Dijkstra算法的最短路径搜索（两点之间）
    QVector<ClientNode *> findShortestPath(ClientNode *start, ClientNode *end) const;

    // 评估路径得分（综合节点数量和链路成本）
    double evaluatePathScore(int nodeCount, double linkCost) const;
};

#endif // MONTE_CARLO_PATH_FINDER_H
