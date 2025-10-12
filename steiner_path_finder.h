#ifndef PATH_COVER_SOLVER_H
#define PATH_COVER_SOLVER_H

// C++11 + Qt containers version of PathCoverSolver
// NOTE: You must implement the ADJUST SECTION helpers in the .cpp file
// and ensure your build links against QtCore.

#include <QVector>
#include <QHash>
#include <QSet>
#include <QPair>
#include "client_node.h"
#include "link.h"
#include "network_canvas.h"

class PathCoverSolver
{
public:
    explicit PathCoverSolver(NetworkCanvas *canvas);

    // Main entry: returns ordered list of nodes forming the path.
    // May contain duplicates in adversarial graphs (see docs).
    QVector<ClientNode *> findBestPath();

    // Optional: tune threshold for exact Held-Karp (default 16)
    void setExactThreshold(int t);

private:
    NetworkCanvas *m_canvas;
    int exactThreshold;

    // Graph representation
    struct Edge
    {
        int to;
        double w;
        int linkIndex;
    };

    // internal data (Qt containers)
    QVector<QVector<Edge>> adj;
    QVector<ClientNode *> nodes;
    QVector<Link *> links;
    QHash<long long, int> idmap;
    QVector<int> terminals;

    // ----------------- ADJUST THIS SECTION to your API -------------------
    // Implementations are in the .cpp file; you must adapt them to your real API.
    ClientNode *linkSrc(Link *L);
    ClientNode *linkDst(Link *L);
    double linkWeight(Link *L);
    inline long long nodeId(const ClientNode *n) const;
    double nodeDataRatio(const ClientNode *n) const;
    // ------------------------------------------------------------------

    // helpers
    struct DijkstraResult
    {
        QVector<double> dist;
        QVector<int> parent;
    };

    void buildGraphFromCanvas();

    DijkstraResult dijkstra_earlystop(int s,
                                      const QSet<int> &targets,
                                      int needFoundCount);

    QVector<int> reconstruct_path_from_parent(int source, int target, const QVector<int> &parent);

    QVector<int> held_karp_open(const QVector<QVector<double>> &dist);

    double seq_cost(const QVector<int> &seq, const QVector<QVector<double>> &dist);
    QVector<int> greedy_nn_then_2opt(const QVector<QVector<double>> &dist);

    void remove_nonterminal_cycles(QVector<int> &walk, const QVector<char> &isTerminal);

    QVector<ClientNode *> find_path_covering_terminals();
};

#endif // PATH_COVER_SOLVER_H
