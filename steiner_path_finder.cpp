// path_cover_solver.cpp
// C++11 + Qt containers implementation for PathCoverSolver

#include "steiner_path_finder.h"
#include <QtGlobal>
#include <QDebug>
#include <algorithm> // for std::reverse, std::min
#include <limits>
#include "client_node.h"
#include "link.h"
#include "network_canvas.h"

// ----------------- ADJUST THIS SECTION to your API -------------------
// Implement these helpers to match your Link/Node API.
// Replace the bodies with your real methods (source/target/weight/dataRatio).
ClientNode *PathCoverSolver::linkSrc(Link *L)
{
    // Example placeholder:
    return L->node1();
    // ----- MODIFY: put your code here -----
    // return nullptr;
}
ClientNode *PathCoverSolver::linkDst(Link *L)
{
    // Example placeholder:
    return L->node2();
    // ----- MODIFY: put your code here -----
    // return nullptr;
}
double PathCoverSolver::linkWeight(Link *L)
{
    // Example placeholder:
    // return L->weight();
    // ----- MODIFY: put your code here -----
    return 1.0;
}
inline long long PathCoverSolver::nodeId(const ClientNode *n) const
{
    return reinterpret_cast<long long>(n); // pointer-based id (adjust if you have real id)
}
double PathCoverSolver::nodeDataRatio(const ClientNode *n) const
{
    // Example placeholder:
    return n->dataRatio();
    // ----- MODIFY: put your code here -----
    // return 0.0;
}
// ------------------------------------------------------------------

// Small binary min-heap based on QVector<QPair<double,int>>
class MinHeap
{
public:
    MinHeap() { data.reserve(256); }

    bool empty() const { return data.isEmpty(); }

    void push(double priority, int node)
    {
        QPair<double, int> item(priority, node);
        data.append(item);
        siftUp(data.size() - 1);
    }

    QPair<double, int> top() const
    {
        return data.first();
    }

    void pop()
    {
        if (data.isEmpty())
            return;
        data[0] = data.last();
        data.pop_back();
        if (!data.isEmpty())
            siftDown(0);
    }

private:
    QVector<QPair<double, int>> data;

    void siftUp(int idx)
    {
        while (idx > 0)
        {
            int parent = (idx - 1) >> 1;
            if (data[parent].first <= data[idx].first)
                break;
            qSwap(data[parent], data[idx]);
            idx = parent;
        }
    }

    void siftDown(int idx)
    {
        int n = data.size();
        while (true)
        {
            int l = (idx << 1) + 1;
            int r = l + 1;
            int smallest = idx;
            if (l < n && data[l].first < data[smallest].first)
                smallest = l;
            if (r < n && data[r].first < data[smallest].first)
                smallest = r;
            if (smallest == idx)
                break;
            qSwap(data[idx], data[smallest]);
            idx = smallest;
        }
    }
};

// ---------------- PathCoverSolver implementation ----------------

PathCoverSolver::PathCoverSolver(NetworkCanvas *canvas)
    : m_canvas(canvas), exactThreshold(16)
{
    buildGraphFromCanvas();
}

void PathCoverSolver::setExactThreshold(int t)
{
    exactThreshold = qMax(2, t);
}

QVector<ClientNode *> PathCoverSolver::findBestPath()
{
    return find_path_covering_terminals();
}

void PathCoverSolver::buildGraphFromCanvas()
{
    nodes = m_canvas->getNodes();
    links = m_canvas->getLinks();
    int N = nodes.size();
    adj.clear();
    adj.resize(N);
    idmap.clear();
    for (int i = 0; i < N; ++i)
        idmap.insert(nodeId(nodes[i]), i);
    for (int li = 0; li < links.size(); ++li)
    {
        Link *L = links[li];
        ClientNode *a = linkSrc(L);
        ClientNode *b = linkDst(L);
        if (!a || !b)
            continue; // user must implement helpers
        bool okA = idmap.contains(nodeId(a));
        bool okB = idmap.contains(nodeId(b));
        if (!okA || !okB)
            continue;
        int u = idmap.value(nodeId(a));
        int v = idmap.value(nodeId(b));
        double w = linkWeight(L);
        if (!(w >= 0.0))
            w = 1.0;
        Edge ea = {v, w, li};
        Edge eb = {u, w, li};
        adj[u].append(ea);
        adj[v].append(eb);
    }

    // collect terminals
    terminals.clear();
    for (int i = 0; i < N; ++i)
    {
        if (nodeDataRatio(nodes[i]) > 0.0)
            terminals.append(i);
    }
}

// Early-stop Dijkstra: stop when we've found needFoundCount targets
PathCoverSolver::DijkstraResult PathCoverSolver::dijkstra_earlystop(int s,
                                                                    const QSet<int> &targets,
                                                                    int needFoundCount)
{
    int N = adj.size();
    const double INF = std::numeric_limits<double>::infinity();
    QVector<double> dist;
    dist.resize(N);
    QVector<int> parent;
    parent.resize(N);
    for (int i = 0; i < N; ++i)
    {
        dist[i] = INF;
        parent[i] = -1;
    }
    MinHeap pq;
    dist[s] = 0.0;
    pq.push(0.0, s);
    int found = 0;
    QVector<char> seen;
    seen.resize(N);
    for (int i = 0; i < N; ++i)
        seen[i] = 0;
    while (!pq.empty())
    {
        QPair<double, int> cur = pq.top();
        pq.pop();
        double d = cur.first;
        int u = cur.second;
        if (seen[u])
            continue;
        seen[u] = 1;
        if (targets.contains(u))
        {
            ++found;
            if (found >= needFoundCount)
                break;
        }
        const QVector<Edge> &edges = adj[u];
        for (int ei = 0; ei < edges.size(); ++ei)
        {
            const Edge &e = edges[ei];
            int v = e.to;
            double nd = d + e.w;
            if (nd + 1e-12 < dist[v])
            {
                dist[v] = nd;
                parent[v] = u;
                pq.push(nd, v);
            }
        }
    }
    return {dist, parent};
}

QVector<int> PathCoverSolver::reconstruct_path_from_parent(int source, int target, const QVector<int> &parent)
{
    QVector<int> temp;
    int cur = target;
    if (parent.size() <= cur)
        return {};
    if (parent[cur] == -1 && cur != source)
    {
        return {};
    }
    while (cur != -1)
    {
        temp.append(cur);
        if (cur == source)
            break;
        cur = parent[cur];
    }
    std::reverse(temp.begin(), temp.end());
    return temp;
}

QVector<int> PathCoverSolver::held_karp_open(const QVector<QVector<double>> &dist)
{
    int m = dist.size();
    int FULL = 1 << m;
    const double INF = 1e100;
    // dp and parent stored as QVector<QVector<...>>
    QVector<QVector<double>> dp;
    dp.resize(FULL);
    QVector<QVector<int>> parent;
    parent.resize(FULL);
    for (int mask = 0; mask < FULL; ++mask)
    {
        dp[mask].resize(m);
        parent[mask].resize(m);
        for (int j = 0; j < m; ++j)
        {
            dp[mask][j] = INF;
            parent[mask][j] = -1;
        }
    }
    for (int i = 0; i < m; ++i)
        dp[1 << i][i] = 0.0;
    for (int mask = 1; mask < FULL; ++mask)
    {
        for (int last = 0; last < m; ++last)
            if (mask & (1 << last))
            {
                double cur = dp[mask][last];
                if (cur >= INF)
                    continue;
                for (int nxt = 0; nxt < m; ++nxt)
                    if (!(mask & (1 << nxt)))
                    {
                        int nm = mask | (1 << nxt);
                        double cand = cur + dist[last][nxt];
                        if (cand + 1e-12 < dp[nm][nxt])
                        {
                            dp[nm][nxt] = cand;
                            parent[nm][nxt] = last;
                        }
                    }
            }
    }
    double best = INF;
    int bestLast = -1;
    int full = FULL - 1;
    for (int last = 0; last < m; ++last)
    {
        if (dp[full][last] < best)
        {
            best = dp[full][last];
            bestLast = last;
        }
    }
    QVector<int> order;
    int curmask = full, cur = bestLast;
    while (cur != -1)
    {
        order.append(cur);
        int p = parent[curmask][cur];
        curmask ^= (1 << cur);
        cur = p;
    }
    std::reverse(order.begin(), order.end());
    return order;
}

double PathCoverSolver::seq_cost(const QVector<int> &seq, const QVector<QVector<double>> &dist)
{
    double s = 0;
    for (int i = 0; i + 1 < seq.size(); ++i)
        s += dist[seq[i]][seq[i + 1]];
    return s;
}

QVector<int> PathCoverSolver::greedy_nn_then_2opt(const QVector<QVector<double>> &dist)
{
    int m = dist.size();
    if (m == 0)
        return {};
    QVector<int> bestSeq;
    double bestCost = 1e200;
    int tries = qMin(m, 20);
    for (int start = 0; start < tries; ++start)
    {
        QVector<char> used;
        used.resize(m);
        for (int i = 0; i < m; ++i)
            used[i] = 0;
        int cur = start;
        used[cur] = 1;
        QVector<int> seq;
        seq.append(cur);
        for (int step = 1; step < m; ++step)
        {
            int nxt = -1;
            double best = 1e200;
            for (int j = 0; j < m; ++j)
                if (!used[j])
                {
                    if (dist[cur][j] < best)
                    {
                        best = dist[cur][j];
                        nxt = j;
                    }
                }
            if (nxt == -1)
                break;
            seq.append(nxt);
            used[nxt] = 1;
            cur = nxt;
        }
        bool improved = true;
        int iter = 0;
        while (improved && iter < 200)
        {
            improved = false;
            ++iter;
            for (int i = 0; i + 2 < seq.size(); ++i)
            {
                for (int j = i + 2; j < seq.size(); ++j)
                {
                    double before = dist[seq[i]][seq[i + 1]] + ((j + 1 < seq.size()) ? dist[seq[j]][seq[j + 1]] : 0);
                    double after = dist[seq[i]][seq[j]] + ((j + 1 < seq.size()) ? dist[seq[i + 1]][seq[j + 1]] : 0);
                    if (after + 1e-12 < before)
                    {
                        std::reverse(seq.begin() + i + 1, seq.begin() + j + 1);
                        improved = true;
                    }
                }
            }
        }
        double c = seq_cost(seq, dist);
        if (c < bestCost)
        {
            bestCost = c;
            bestSeq = seq;
        }
    }
    return bestSeq;
}

void PathCoverSolver::remove_nonterminal_cycles(QVector<int> &walk, const QVector<char> &isTerminal)
{
    bool changed = true;
    int n;
    while (changed)
    {
        changed = false;
        n = walk.size();
        QHash<int, int> pos;
        QVector<int> prefixTerm;
        prefixTerm.resize(n + 1);
        prefixTerm[0] = 0;
        for (int i = 0; i < n; ++i)
            prefixTerm[i + 1] = prefixTerm[i] + (isTerminal[walk[i]] ? 1 : 0);
        for (int i = 0; i < n; ++i)
        {
            int v = walk[i];
            if (!pos.contains(v))
            {
                pos.insert(v, i);
            }
            else
            {
                int p = pos.value(v);
                int termCount = prefixTerm[i] - prefixTerm[p + 1];
                if (termCount == 0)
                {
                    QVector<int> newWalk;
                    newWalk.reserve(walk.size() - (i - p - 1));
                    for (int k = 0; k <= p; ++k)
                        newWalk.append(walk[k]);
                    for (int k = i; k < n; ++k)
                        newWalk.append(walk[k]);
                    walk.swap(newWalk);
                    changed = true;
                    break;
                }
                else
                {
                    pos.insert(v, i);
                }
            }
        }
    }
}

QVector<ClientNode *> PathCoverSolver::find_path_covering_terminals()
{
    int N = nodes.size();
    int m = terminals.size();
    if (m < 2)
    {
        if (m == 1)
            return {nodes[terminals[0]]};
        return {};
    }

    // compute pairwise term dists (and parents)
    QVector<QVector<double>> termDist;
    termDist.resize(m);
    for (int i = 0; i < m; ++i)
        termDist[i].resize(m);
    QVector<QVector<int>> parents_per_source;
    parents_per_source.resize(m);
    for (int i = 0; i < m; ++i)
        parents_per_source[i].fill(-1, N);

    for (int i = 0; i < m; ++i)
    {
        QSet<int> targets;
        for (int j = 0; j < m; ++j)
            if (j != i)
                targets.insert(terminals[j]);
        DijkstraResult res = dijkstra_earlystop(terminals[i], targets, targets.size());
        parents_per_source[i] = res.parent;
        for (int j = 0; j < m; ++j)
            termDist[i][j] = res.dist[terminals[j]];
    }

    // solve TSP-path on terminal metric
    QVector<int> termOrderIdx;
    if (m <= exactThreshold)
    {
        termOrderIdx = held_karp_open(termDist);
    }
    else
    {
        termOrderIdx = greedy_nn_then_2opt(termDist);
    }
    if (termOrderIdx.isEmpty())
    {
        termOrderIdx.resize(m);
        for (int i = 0; i < m; ++i)
            termOrderIdx[i] = i;
    }

    // expand to walk
    QVector<int> walk;
    for (int k = 0; k + 1 < termOrderIdx.size(); ++k)
    {
        int si = termOrderIdx[k];
        int ti = termOrderIdx[k + 1];
        int sNode = terminals[si];
        int tNode = terminals[ti];
        QVector<int> piece = reconstruct_path_from_parent(sNode, tNode, parents_per_source[si]);
        if (piece.isEmpty())
        {
            QSet<int> targ;
            targ.insert(tNode);
            DijkstraResult resfull = dijkstra_earlystop(sNode, targ, 1);
            piece = reconstruct_path_from_parent(sNode, tNode, resfull.parent);
        }
        if (k == 0)
        {
            for (int v : piece)
                walk.append(v);
        }
        else
        {
            for (int ii = 1; ii < piece.size(); ++ii)
                walk.append(piece[ii]);
        }
    }

    // remove non-terminal cycles
    QVector<char> isTerminal;
    isTerminal.resize(N);
    for (int i = 0; i < N; ++i)
        isTerminal[i] = 0;
    for (int idx : terminals)
        isTerminal[idx] = 1;
    remove_nonterminal_cycles(walk, isTerminal);

    // warn if duplicates remain
    {
        QSet<int> seen;
        bool dup = false;
        for (int v : walk)
        {
            if (seen.contains(v))
            {
                dup = true;
                break;
            }
            seen.insert(v);
        }
        if (dup)
        {
            qWarning("PathCoverSolver: result still has repeated nodes; returning best-effort walk.");
        }
    }

    QVector<ClientNode *> result;
    result.reserve(walk.size());
    for (int v : walk)
        result.append(nodes[v]);
    return result;
}
