#include "SteinerPathFinder.h"
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ================== 构造 ==================
SteinerPathFinder::SteinerPathFinder(const QMap<ClientNode *, QVector<Link *>> &adj)
    : m_adj(adj)
{
}

void SteinerPathFinder::setTerminals(const QList<ClientNode *> &terminals)
{
    m_terminals = terminals;
}

// ================== 边权函数 ==================
double SteinerPathFinder::linkCost(const Link *l)
{
    // 沿用 monte_carlo_path_finder.cpp 公式
    double bwFactor = 1.0 / (1.0 + std::exp(-l->bandwidth() / 1000.0));
    return l->congestion() * 0.5 +
           (1.0 - bwFactor) * 0.3 +
           l->distance() / 100.0 * 0.2;
}

// ================== 双向 A* ==================
QVector<ClientNode *> SteinerPathFinder::bidirectionalAstar(
    ClientNode *start, ClientNode *end,
    const QMap<ClientNode *, QVector<Link *>> &adj)
{
    if (start == end)
        return {start};

    using NodeDist = std::pair<double, ClientNode *>;
    auto cmp = [](const NodeDist &a, const NodeDist &b)
    { return a.first > b.first; };
    std::priority_queue<NodeDist, std::vector<NodeDist>, decltype(cmp)> pqF(cmp), pqB(cmp);

    std::unordered_map<ClientNode *, double> distF, distB;
    QMap<ClientNode *, ClientNode *> prevF, prevB;
    QSet<ClientNode *> visitedF, visitedB;

    auto h = [](ClientNode *a, ClientNode *b)
    {
        double dx = a->position().x() - b->position().x();
        double dy = a->position().y() - b->position().y();
        return std::sqrt(dx * dx + dy * dy) / 100.0;
    };

    distF[start] = 0;
    pqF.push({0, start});
    distB[end] = 0;
    pqB.push({0, end});
    double best = std::numeric_limits<double>::infinity();
    ClientNode *meet = nullptr;

    while (!pqF.empty() || !pqB.empty())
    {
        bool goF = false;
        if (pqF.empty())
            goF = false;
        else if (pqB.empty())
            goF = true;
        else
            goF = pqF.top().first + h(pqF.top().second, end) <=
                  pqB.top().first + h(pqB.top().second, start);

        if (goF)
        {
            auto [d, u] = pqF.top();
            pqF.pop();
            if (d > distF[u])
                continue;
            if (d + h(u, end) >= best)
                break;
            visitedF.insert(u);
            if (visitedB.contains(u))
            {
                meet = u;
                best = d + distB[u];
            }
            for (Link *l : adj[u])
            {
                ClientNode *v = (l->node1() == u) ? l->node2() : l->node1();
                double nd = d + linkCost(l);
                if (nd < distF[v] || !distF.count(v))
                {
                    distF[v] = nd;
                    prevF[v] = u;
                    pqF.push({nd + h(v, end), v});
                }
            }
        }
        else
        { /* 对称反向 */
            ...
        }
    }

    QVector<ClientNode *> path;
    for (ClientNode *at = meet; at; at = prevF.value(at, nullptr))
        path.prepend(at);
    for (ClientNode *at = prevB.value(meet, nullptr); at; at = prevB.value(at, nullptr))
        path.append(at);
    return path;
}

// ================== Steiner 链生成 ==================
QVector<QPair<ClientNode *, ClientNode *>>
SteinerPathFinder::steinerChainEdges(const QList<ClientNode *> &T,
                                     const QMap<ClientNode *, QVector<Link *>> &adj)
{
    // 1. 终端完全图 + 双向 A* 距离
    struct TEdge
    {
        ClientNode *a;
        ClientNode *b;
        double dist;
    };
    QVector<TEdge> edges;
    for (int i = 0; i < T.size(); ++i)
        for (int j = i + 1; j < T.size(); ++j)
        {
            QVector<ClientNode *> p = bidirectionalAstar(T[i], T[j], adj);
            double d = 0;
            for (int k = 0; k + 1 < p.size(); ++k)
                d += linkCost(adj[p[k]][0]); // 简化：取第一条邻接边
            edges.push_back({T[i], T[j], d});
        }

    // 2. Kruskal
    std::sort(edges.begin(), edges.end(),
              [](const TEdge &a, const TEdge &b)
              { return a.dist < b.dist; });
    QMap<ClientNode *, int> id;
    for (int i = 0; i < T.size(); ++i)
        id[T[i]] = i;
    std::vector<int> fa(T.size());
    std::iota(fa.begin(), fa.end(), 0);
    auto find = [&](int x)
    { return x == fa[x] ? x : fa[x] = find(fa[x]); };

    QVector<QPair<ClientNode *, ClientNode *>> chainEdges;
    for (const TEdge &e : edges)
    {
        int u = id[e.a], v = id[e.b];
        if (find(u) != find(v))
        {
            fa[find(u)] = find(v);
            QVector<ClientNode *> p = bidirectionalAstar(e.a, e.b, adj);
            for (int i = 0; i + 1 < p.size(); ++i)
                chainEdges.append(qMakePair(p[i], p[i + 1]));
        }
    }
    return chainEdges;
}

// ================== 主接口：最佳路径 ==================
QVector<ClientNode *> SteinerPathFinder::findBestPath()
{
    if (m_terminals.isEmpty())
        return {};
    if (m_terminals.size() == 1)
        return {m_terminals.first()};

    auto edges = steinerChainEdges(m_terminals, m_adj);
    // 合并成连续链（去重+顺序连接，这里简单返回第一条顺序）
    QVector<ClientNode *> path;
    QSet<QPair<ClientNode *, ClientNode *>> edgeSet;
    for (auto e : edges)
    {
        if (!edgeSet.contains(e))
        {
            edgeSet.insert(e);
            if (path.isEmpty() || path.back() == e.first)
                path << e.second;
            else
                path << e.first << e.second; // 粗略拼接
        }
    }
    return path;
}

// ================== 训练数据导出 ==================
QVector<SteinerPathFinder::EdgeLabel>
SteinerPathFinder::exportLabels(int negPerPos)
{
    QVector<EdgeLabel> samples;
    auto posEdges = steinerChainEdges(m_terminals, m_adj);
    QSet<QPair<ClientNode *, ClientNode *>> posSet;
    for (auto e : posEdges)
        posSet.insert(e);

    // 正样本
    for (auto e : posEdges)
    {
        double c = linkCost(m_adj[e.first][0]); // 简化
        samples.append({e.first, e.second, c, 1});
    }

    // 负样本：同起点随机非最优邻居
    for (auto e : posEdges)
    {
        ClientNode *u = e.first;
        const QVector<Link *> &neigh = m_adj[u];
        int cnt = 0;
        for (int i = 0; i < neigh.size() * 3 && cnt < negPerPos; ++i)
        {
            Link *l = neigh[rand() % neigh.size()];
            ClientNode *v = (l->node1() == u) ? l->node2() : l->node1();
            if (!posSet.contains(qMakePair(u, v)))
            {
                samples.append({u, v, linkCost(l), 0});
                ++cnt;
            }
        }
    }
    return samples;
}