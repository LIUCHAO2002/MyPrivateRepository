#include "bidirectional_astar_path_finder.h"
#include <queue>
#include <cmath>

typedef std::pair<double, ClientNode *> NodeCost;

BidirectionalAstarPathFinder::BidirectionalAstarPathFinder(NetworkCanvas *canvas)
    : m_canvas(canvas)
{
    buildGraph();
}

void BidirectionalAstarPathFinder::buildGraph()
{
    m_targets.clear();
    m_adj.clear();

    // 1. 收集 dataRatio()>0 的节点
    foreach (ClientNode *n, m_canvas->getNodes()) // Qt 容器可用 foreach
        if (n && n->dataRatio() > 0)
            m_targets.append(n);

    // 2. 建带权邻接表
    foreach (Link *l, m_canvas->getLinks())
    {
        if (!l)
            continue;
        ClientNode *a = l->node1();
        ClientNode *b = l->node2();
        if (!a || !b)
            continue;

        double bw = 1.0 / (1.0 + std::exp(-l->bandwidth() / 1000.0));
        double cost = l->congestion() * 0.5 + (1.0 - bw) * 0.3 + l->distance() / 100.0 * 0.2;

        m_adj[a].append(qMakePair(b, cost));
        m_adj[b].append(qMakePair(a, cost));
    }
}

double BidirectionalAstarPathFinder::heuristic(ClientNode *a, ClientNode *b) const
{
    Q_UNUSED(a);
    Q_UNUSED(b);
    return 0.0; // admissible
}

PathResult BidirectionalAstarPathFinder::bidirectionalAstar(ClientNode *s, ClientNode *t)
{
    PathResult res;
    res.visited = 0;

    if (s == t)
    {
        res.path = QVector<ClientNode *>() << s;
        res.totalCost = 0;
        return res;
    }

    QMap<ClientNode *, double> distF, distB;
    QMap<ClientNode *, ClientNode *> prevF, prevB;
    QSet<ClientNode *> visitedF, visitedB;

    std::priority_queue<NodeCost, std::vector<NodeCost>, std::greater<NodeCost>> pqF, pqB;

    distF[s] = 0;
    pqF.push(std::make_pair(0.0, s));
    distB[t] = 0;
    pqB.push(std::make_pair(0.0, t));

    double best = std::numeric_limits<double>::infinity();

    while (!pqF.empty() || !pqB.empty())
    {
        bool goForward = false;
        if (pqF.empty())
            goForward = false;
        else if (pqB.empty())
            goForward = true;
        else
        {
            double fTop = pqF.top().first + heuristic(pqF.top().second, t);
            double bTop = pqB.top().first + heuristic(pqB.top().second, s);
            goForward = (fTop <= bTop);
        }

        if (goForward)
        {
            double d = pqF.top().first;
            ClientNode *u = pqF.top().second;
            pqF.pop();
            if (d > distF[u])
                continue;
            if (d + heuristic(u, t) >= best)
                break;
            visitedF.insert(u);
            ++res.visited;
            if (visitedB.contains(u))
                best = qMin(best, distF[u] + distB[u]);

            QPair<ClientNode *, double> edge;
            foreach (edge, m_adj[u])
            {
                ClientNode *v = edge.first;
                double w = edge.second;
                double nd = d + w;
                if (!distF.contains(v) || nd < distF[v])
                {
                    distF[v] = nd;
                    prevF[v] = u;
                    pqF.push(std::make_pair(nd, v));
                }
            }
        }
        else // backward
        {
            double d = pqB.top().first;
            ClientNode *u = pqB.top().second;
            pqB.pop();
            if (d > distB[u])
                continue;
            if (d + heuristic(u, s) >= best)
                break;
            visitedB.insert(u);
            ++res.visited;
            if (visitedF.contains(u))
                best = qMin(best, distB[u] + distF[u]);

            QPair<ClientNode *, double> edge;
            foreach (edge, m_adj[u])
            {
                ClientNode *v = edge.first;
                double w = edge.second;
                double nd = d + w;
                if (!distB.contains(v) || nd < distB[v])
                {
                    distB[v] = nd;
                    prevB[v] = u;
                    pqB.push(std::make_pair(nd, v));
                }
            }
        }
    }

    if (best == std::numeric_limits<double>::infinity())
        return res; // 不可达

    // 找相遇点
    ClientNode *meet = nullptr;
    for (QMap<ClientNode *, double>::const_iterator it = distF.begin();
         it != distF.end(); ++it)
    {
        ClientNode *n = it.key();
        if (distB.contains(n) && distF[n] + distB[n] == best)
        {
            meet = n;
            break;
        }
    }

    // 重建路径
    QVector<ClientNode *> pathF, pathB;
    for (ClientNode *at = meet; at; at = prevF.value(at, nullptr))
        pathF.prepend(at);
    for (ClientNode *at = prevB.value(meet, nullptr); at; at = prevB.value(at, nullptr))
        pathB.append(at);

    res.path = pathF + pathB;
    res.totalCost = best;
    return res;
}

QVector<ClientNode *> BidirectionalAstarPathFinder::findBestPath()
{
    if (m_targets.isEmpty())
        return QVector<ClientNode *>();
    if (m_targets.size() == 1)
        return QVector<ClientNode *>() << m_targets.first();

    ClientNode *start = m_targets.first();
    QVector<ClientNode *> fullPath;
    fullPath << start;
    double totalCost = 0;

    for (int i = 1; i < m_targets.size(); ++i)
    {
        PathResult res = bidirectionalAstar(fullPath.last(), m_targets[i]);
        if (res.path.isEmpty())
            continue; // 跳过不可达
        for (int j = 1; j < res.path.size(); ++j)
            fullPath << res.path[j];
        totalCost += res.totalCost;
    }

    m_result.path = fullPath;
    m_result.totalCost = totalCost;
    m_result.visited = 0;
    return fullPath;
}