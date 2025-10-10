#include "monte_carlo_path_finder.h"
#include <QRandomGenerator>
#include <algorithm>
#include <queue>
#include <cmath>

// 辅助函数：针对double类型实现clamp功能，兼容C++17之前的标准
double manual_clamp(double value, double min_val, double max_val)
{
    if (value < min_val)
        return min_val;
    if (value > max_val)
        return max_val;
    return value;
}

// 构造函数：初始化参数并构建图数据
MonteCarloPathFinder::MonteCarloPathFinder(NetworkCanvas *canvas)
    : m_canvas(canvas),
      m_iterations(10000), // 默认迭代次数
      m_nodeWeight(0.3),   // 默认节点数量权重30%
      m_linkWeight(0.7)    // 默认链路成本权重70%
{
    buildGraphData(); // 初始化图结构和目标节点
}

// 设置评估权重（自动归一化，确保总和为1.0）
void MonteCarloPathFinder::setWeights(double nodeWeight, double linkWeight)
{
    // 限制权重范围在[0, 1]，使用手动实现的double类型clamp函数
    m_nodeWeight = manual_clamp(nodeWeight, 0.0, 1.0);
    m_linkWeight = manual_clamp(linkWeight, 0.0, 1.0);

    // 归一化处理
    double sum = m_nodeWeight + m_linkWeight;
    if (sum > 0)
    {
        m_nodeWeight /= sum;
        m_linkWeight /= sum;
    }
    else
    {
        // 权重无效时使用默认值
        m_nodeWeight = 0.3;
        m_linkWeight = 0.7;
    }
}

// 核心方法：执行蒙特卡洛搜索并返回最佳路径
QVector<ClientNode *> MonteCarloPathFinder::findBestPath()
{
    // 边界条件处理
    if (m_targetNodes.isEmpty())
        return {};
    if (m_targetNodes.size() == 1)
        return {m_targetNodes.first()};

    // 初始化最佳评估结果
    m_bestEval.score = std::numeric_limits<double>::max();
    m_bestEval.nodeCount = std::numeric_limits<int>::max();
    m_bestEval.totalCost = std::numeric_limits<double>::max();

    // 多次迭代搜索最优路径
    for (int i = 0; i < m_iterations; ++i)
    {
        PathEvaluation current = generateRandomPath();
        if (current.score < m_bestEval.score)
        {
            m_bestEval = current;
        }
    }

    return m_bestEval.path;
}

// 构建图数据（邻接表和链路成本）
void MonteCarloPathFinder::buildGraphData()
{
    // 清空原有数据
    m_targetNodes.clear();
    m_adjacencyList.clear();
    m_linkCosts.clear();

    // 收集目标节点（数据占比不为0的节点）
    for (ClientNode *node : m_canvas->getNodes())
    {
        if (node && node->dataRatio() > 0)
        {
            m_targetNodes.append(node);
        }
    }

    // 构建邻接表和链路成本
    for (Link *link : m_canvas->getLinks())
    {
        if (!link)
            continue;
        ClientNode *n1 = link->node1();
        ClientNode *n2 = link->node2();
        if (!n1 || !n2)
            continue;

        // 计算链路成本（综合带宽、拥塞和距离，归一化处理）
        double bandwidthFactor = 1.0 / (1.0 + std::exp(-link->bandwidth() / 1000.0)); // 带宽越大成本越低
        double cost = (link->congestion() * 0.5) +                                    // 拥塞权重50%
                      ((1.0 - bandwidthFactor) * 0.3) +                               // 带宽权重30%（反向映射）
                      (link->distance() / 100.0 * 0.2);                               // 距离权重20%（假设最大距离100）

        // 双向添加到邻接表（无向图）
        m_adjacencyList[n1].append(n2);
        m_adjacencyList[n2].append(n1);
        // 缓存链路成本（双向相同）
        m_linkCosts[{n1, n2}] = cost;
        m_linkCosts[{n2, n1}] = cost;
    }
}

// 生成随机路径（蒙特卡洛核心逻辑）
PathEvaluation MonteCarloPathFinder::generateRandomPath() const
{
    PathEvaluation eval;
    QSet<ClientNode *> visitedTargets; // 记录已访问的目标节点
    QVector<ClientNode *> path;        // 存储当前路径

    // 随机选择起始目标节点
    ClientNode *current = m_targetNodes[QRandomGenerator::global()->bounded(m_targetNodes.size())];
    path.append(current);
    visitedTargets.insert(current);

    // 遍历所有目标节点
    while (visitedTargets.size() < m_targetNodes.size())
    {
        // 筛选未访问的目标节点
        QVector<ClientNode *> unvisited;
        for (ClientNode *node : m_targetNodes)
        {
            if (!visitedTargets.contains(node))
            {
                unvisited.append(node);
            }
        }

        // 随机选择下一个目标节点
        ClientNode *nextTarget = unvisited[QRandomGenerator::global()->bounded(unvisited.size())];

        // 查找当前节点到目标节点的最短路径（基于链路成本）
        QVector<ClientNode *> subPath = findShortestPath(current, nextTarget);
        if (subPath.isEmpty())
            continue; // 理论上连通图不会出现此情况

        // 添加子路径（排除重复的当前节点）
        for (int i = 1; i < subPath.size(); ++i)
        {
            path.append(subPath[i]);
        }

        current = nextTarget;
        visitedTargets.insert(current);
    }

    // 计算路径评估指标
    eval.path = path;
    eval.nodeCount = path.size();
    eval.totalCost = calculatePathCost(path);
    eval.score = evaluatePathScore(eval.nodeCount, eval.totalCost);

    return eval;
}

// 计算路径的总链路成本
double MonteCarloPathFinder::calculatePathCost(const QVector<ClientNode *> &path) const
{
    double cost = 0.0;
    for (int i = 0; i < path.size() - 1; ++i)
    {
        ClientNode *from = path[i];
        ClientNode *to = path[i + 1];
        auto key = qMakePair(from, to);
        if (m_linkCosts.contains(key))
        {
            cost += m_linkCosts[key];
        }
    }
    return cost;
}

// 基于Dijkstra算法的最短路径搜索（两点之间）
QVector<ClientNode *> MonteCarloPathFinder::findShortestPath(ClientNode *start, ClientNode *end) const
{
    if (!start || !end)
        return {};

    // Dijkstra算法实现（使用优先队列优化）
    QMap<ClientNode *, double> distances;          // 节点到起点的距离
    QMap<ClientNode *, ClientNode *> predecessors; // 路径前驱节点
    QSet<ClientNode *> visited;                    // 已访问节点

    // 初始化距离（默认为无穷大）
    for (ClientNode *node : m_canvas->getNodes())
    {
        distances[node] = std::numeric_limits<double>::infinity();
    }
    distances[start] = 0.0;

    // 优先队列（按距离从小到大排序）
    using NodeDist = std::pair<double, ClientNode *>;
    std::priority_queue<NodeDist, std::vector<NodeDist>, std::greater<NodeDist>> pq;
    pq.emplace(0.0, start);

    while (!pq.empty())
    {
        ClientNode *current = pq.top().second;
        pq.pop();

        if (visited.contains(current))
            continue;
        if (current == end)
            break; // 到达目标节点，提前退出

        visited.insert(current);

        // 遍历邻居节点并更新距离
        for (ClientNode *neighbor : m_adjacencyList[current])
        {
            if (visited.contains(neighbor))
                continue;

            double newDist = distances[current] + m_linkCosts[{current, neighbor}];
            if (newDist < distances[neighbor])
            {
                distances[neighbor] = newDist;
                predecessors[neighbor] = current;
                pq.emplace(newDist, neighbor);
            }
        }
    }

    // 重建路径
    QVector<ClientNode *> path;
    for (ClientNode *at = end; at != nullptr; at = predecessors[at])
    {
        path.prepend(at);
    }

    // 验证路径有效性（起点是否正确）
    return (path.front() == start) ? path : QVector<ClientNode *>();
}

// 评估路径得分（综合节点数量和链路成本）
double MonteCarloPathFinder::evaluatePathScore(int nodeCount, double linkCost) const
{
    // 归一化节点数量（假设最大路径长度为目标节点数的5倍）
    double normalizedNodes = static_cast<double>(nodeCount) / (m_targetNodes.size() * 5.0);
    // 综合评分（加权和，越低越好）
    return (normalizedNodes * m_nodeWeight) + (linkCost * m_linkWeight);
}
