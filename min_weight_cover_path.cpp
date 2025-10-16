#include "min_weight_cover_path.h"
#include "network_canvas.h"
#include <QDebug>
#include <algorithm>
#include <queue>
#include <limits>
#include <cmath>

void MinWeightCoverPath::initialize(NetworkCanvas *canvas)
{
    m_canvas = canvas;
    m_allNodes = canvas->getNodes();
    m_targetNodes.clear();
    m_adjacency.clear();

    // 1. 筛选目标节点（含文件块的节点）
    for (ClientNode *node : m_allNodes)
    {
        if (isTargetNode(node))
        {
            m_targetNodes.append(node);
        }
    }

    // 2. 构建邻接表（使用链路权重作为边权）
    for (Link *link : canvas->getLinks())
    {
        ClientNode *n1 = link->node1();
        ClientNode *n2 = link->node2();
        if (!n1 || !n2)
            continue;

        // 获取链路权重（使用画布提供的有向权重计算方法）
        double weight1 = canvas->getDirectedEdgeWeight(n1, n2);
        double weight2 = canvas->getDirectedEdgeWeight(n2, n1);

        // 双向添加到邻接表（无向图处理，权重可能不同）
        m_adjacency[n1].push_back(MinWeightCoverPath::Edge(n2, weight1));
        m_adjacency[n2].push_back(MinWeightCoverPath::Edge(n1, weight2));
    }

    m_resultDetails.clear();
}

void MinWeightCoverPath::setParameters(const QMap<QString, QVariant> &params)
{
    if (params.contains("dpThreshold"))
    {
        m_dpThreshold = params["dpThreshold"].toInt();
        m_dpThreshold = std::max(2, std::min(16, m_dpThreshold)); // 限制在2-16之间
    }
}

QVector<ClientNode *> MinWeightCoverPath::dijkstra(ClientNode *start, ClientNode *end, double &totalWeight)
{
    totalWeight = 0.0;
    if (!start || !end)
        return {};

    // 存储节点到起点的最短距离
    QMap<ClientNode *, double> dist;
    // 存储前驱节点（用于重建路径）
    QMap<ClientNode *, ClientNode *> prev;
    // 优先队列（按距离升序）
    using NodeDist = std::pair<double, ClientNode *>;
    std::priority_queue<NodeDist, std::vector<NodeDist>, std::greater<NodeDist>> pq;

    // 初始化距离
    for (ClientNode *node : m_allNodes)
    {
        dist[node] = std::numeric_limits<double>::infinity();
    }
    dist[start] = 0.0;
    pq.emplace(0.0, start);

    while (!pq.empty())
    {
        auto [currentDist, u] = pq.top();
        pq.pop();

        if (u == end)
            break; // 到达目标节点，提前退出
        if (currentDist > dist[u])
            continue; // 跳过非最短路径

        // 遍历邻居节点
        for (const Edge &e : m_adjacency[u])
        {
            ClientNode *v = e.to;
            double newDist = currentDist + e.weight;

            if (newDist < dist[v])
            {
                dist[v] = newDist;
                prev[v] = u;
                pq.emplace(newDist, v);
            }
        }
    }

    // 重建路径
    QVector<ClientNode *> path;
    for (ClientNode *at = end; at != nullptr; at = prev[at])
    {
        path.prepend(at);
    }

    // 验证路径有效性
    if (path.isEmpty() || path.first() != start)
    {
        return {};
    }

    totalWeight = dist[end];
    return path;
}

QVector<QVector<double>> MinWeightCoverPath::buildDistanceMatrix(
    const QVector<ClientNode *> &targets,
    QMap<ClientNode *, int> &nodeIndexMap)
{
    int m = targets.size();
    QVector<QVector<double>> distMatrix(m, QVector<double>(m, 0.0));
    nodeIndexMap.clear();

    // 建立目标节点到索引的映射
    for (int i = 0; i < m; ++i)
    {
        nodeIndexMap[targets[i]] = i;
    }

    // 计算任意两个目标节点间的最短路径权重
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < m; ++j)
        {
            if (i == j)
            {
                distMatrix[i][j] = 0.0;
                continue;
            }
            double weight = 0.0;
            dijkstra(targets[i], targets[j], weight);
            distMatrix[i][j] = weight;
        }
    }

    return distMatrix;
}

QVector<int> MinWeightCoverPath::heldKarp(const QVector<QVector<double>> &distMatrix)
{
    int m = distMatrix.size();
    if (m <= 1)
        return {0};

    const double INF = std::numeric_limits<double>::infinity();
    int fullMask = (1 << m) - 1;

    // DP表：dp[mask][u] 表示访问过mask中的节点且最后停在u的最小成本
    QVector<QVector<double>> dp(1 << m, QVector<double>(m, INF));
    // 前驱表：记录路径
    QVector<QVector<int>> prev(1 << m, QVector<int>(m, -1));

    // 初始化：从单个节点出发
    for (int i = 0; i < m; ++i)
    {
        dp[1 << i][i] = 0.0;
    }

    // 填充DP表
    for (int mask = 1; mask < (1 << m); ++mask)
    {
        for (int u = 0; u < m; ++u)
        {
            if (!(mask & (1 << u)))
                continue; // u不在mask中
            if (dp[mask][u] == INF)
                continue;

            // 尝试访问未包含的节点v
            for (int v = 0; v < m; ++v)
            {
                if (mask & (1 << v))
                    continue; // v已在mask中
                int newMask = mask | (1 << v);
                double newCost = dp[mask][u] + distMatrix[u][v];

                if (newCost < dp[newMask][v])
                {
                    dp[newMask][v] = newCost;
                    prev[newMask][v] = u;
                }
            }
        }
    }

    // 找到最优终点
    double minCost = INF;
    int end = 0;
    for (int u = 0; u < m; ++u)
    {
        if (dp[fullMask][u] < minCost)
        {
            minCost = dp[fullMask][u];
            end = u;
        }
    }

    // 回溯重建路径顺序
    QVector<int> order;
    int currentMask = fullMask;
    int current = end;
    while (current != -1)
    {
        order.prepend(current);
        int p = prev[currentMask][current];
        currentMask &= ~(1 << current);
        current = p;
    }

    return order;
}

QVector<int> MinWeightCoverPath::greedy2Opt(const QVector<QVector<double>> &distMatrix)
{
    int m = distMatrix.size();
    if (m <= 1)
        return {0};

    // 贪心初始路径：从0开始，每次选择最近的未访问节点
    QVector<int> order;
    QVector<bool> visited(m, false);
    order.append(0);
    visited[0] = true;

    for (int i = 1; i < m; ++i)
    {
        int last = order.back();
        int best = -1;
        double minDist = std::numeric_limits<double>::infinity();

        for (int j = 0; j < m; ++j)
        {
            if (!visited[j] && distMatrix[last][j] < minDist)
            {
                minDist = distMatrix[last][j];
                best = j;
            }
        }

        if (best != -1)
        {
            order.append(best);
            visited[best] = true;
        }
    }

    // 2opt优化：交换路径中的两个节点，尝试减少总距离
    bool improved = true;
    while (improved)
    {
        improved = false;
        for (int i = 0; i < m - 1; ++i)
        {
            for (int j = i + 1; j < m; ++j)
            {
                if (j - i <= 1)
                    continue;

                // 原路径：i -> i+1, j -> j+1
                double oldCost = distMatrix[order[i]][order[i + 1]] + distMatrix[order[j]][order[(j + 1) % m]];
                // 交换后：i -> j, i+1 -> j+1
                double newCost = distMatrix[order[i]][order[j]] + distMatrix[order[i + 1]][order[(j + 1) % m]];

                if (newCost < oldCost - 1e-9)
                {
                    // 反转i+1到j之间的路径
                    std::reverse(order.begin() + i + 1, order.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }

    return order;
}

QVector<ClientNode *> MinWeightCoverPath::stitchPath(
    const QVector<ClientNode *> &targets,
    const QVector<int> &order,
    const QMap<QPair<ClientNode *, ClientNode *>, QVector<ClientNode *>> &shortPaths)
{
    if (order.isEmpty())
        return {};

    QVector<ClientNode *> fullPath;
    // 添加起点到第一个目标节点的路径
    fullPath.append(targets[order[0]]);

    // 拼接后续目标节点的路径（去重）
    for (int i = 1; i < order.size(); ++i)
    {
        ClientNode *prevNode = targets[order[i - 1]];
        ClientNode *currNode = targets[order[i]];
        auto key = qMakePair(prevNode, currNode);

        if (shortPaths.contains(key))
        {
            const auto &subPath = shortPaths[key];
            // 跳过重复的起点（避免节点重复）
            for (int j = 1; j < subPath.size(); ++j)
            {
                fullPath.append(subPath[j]);
            }
        }
    }

    return fullPath;
}

QVector<ClientNode *> MinWeightCoverPath::findBestPath()
{
    m_resultDetails.clear();
    if (m_targetNodes.isEmpty())
    {
        m_resultDetails["status"] = "no target nodes";
        return {};
    }
    if (m_targetNodes.size() == 1)
    {
        m_resultDetails["status"] = "single target";
        m_resultDetails["total_weight"] = 0.0;
        m_resultDetails["node_count"] = 1;
        return m_targetNodes;
    }

    // 1. 构建目标节点间的最短路径矩阵和路径缓存
    QMap<ClientNode *, int> nodeIndexMap;
    auto distMatrix = buildDistanceMatrix(m_targetNodes, nodeIndexMap);
    QMap<QPair<ClientNode *, ClientNode *>, QVector<ClientNode *>> shortPaths;
    QMap<QPair<ClientNode *, ClientNode *>, double> pathWeights;

    for (int i = 0; i < m_targetNodes.size(); ++i)
    {
        for (int j = 0; j < m_targetNodes.size(); ++j)
        {
            if (i == j)
                continue;
            ClientNode *a = m_targetNodes[i];
            ClientNode *b = m_targetNodes[j];
            double weight = 0.0;
            auto path = dijkstra(a, b, weight);
            if (!path.isEmpty())
            {
                shortPaths[qMakePair(a, b)] = path;
                pathWeights[qMakePair(a, b)] = weight;
            }
        }
    }

    // 2. 选择TSP求解算法（根据目标节点数量）
    QVector<int> order;
    if (m_targetNodes.size() <= m_dpThreshold)
    {
        order = heldKarp(distMatrix); // 动态规划（精确解）
        m_resultDetails["algorithm"] = "Held-Karp (exact)";
    }
    else
    {
        order = greedy2Opt(distMatrix); // 启发式（近似解）
        m_resultDetails["algorithm"] = "Greedy + 2Opt (heuristic)";
    }

    // 3. 拼接完整路径
    auto bestPath = stitchPath(m_targetNodes, order, shortPaths);

    // 4. 计算结果详情
    double totalWeight = 0.0;
    for (int i = 0; i < order.size() - 1; ++i)
    {
        ClientNode *a = m_targetNodes[order[i]];
        ClientNode *b = m_targetNodes[order[i + 1]];
        totalWeight += pathWeights[qMakePair(a, b)];
    }

    m_resultDetails["total_weight"] = totalWeight;
    m_resultDetails["node_count"] = bestPath.size();
    m_resultDetails["target_count"] = m_targetNodes.size();
    m_resultDetails["status"] = "success";

    return bestPath;
}

QMap<QString, QVariant> MinWeightCoverPath::getResultDetails() const
{
    return m_resultDetails;
}