#include "network_canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>
#include <qmath.h>
#include <qDebug>

namespace
{
    static constexpr double kIntersectAngle = 25.0; // 相交判断角度阈值

    static bool nearlyParallel(const QPointF &r, const QPointF &s, double maxDegrees = kIntersectAngle)
    {
        const double dot = r.x() * s.x() + r.y() * s.y();
        const double lenR = std::hypot(r.x(), r.y());
        const double lenS = std::hypot(s.x(), s.y());
        if (qFuzzyIsNull(lenR) || qFuzzyIsNull(lenS))
            return true;
        double cosTheta = dot / (lenR * lenS);
        cosTheta = qBound(-1.0, cosTheta, 1.0);
        const double deg = qAcos(cosTheta) * 180.0 / M_PI;
        return deg < maxDegrees;
    }

    static bool segmentsIntersect(const QPointF &p1, const QPointF &p2,
                                  const QPointF &q1, const QPointF &q2)
    {
        const auto cross = [](const QPointF &a, const QPointF &b)
        {
            return a.x() * b.y() - a.y() * b.x();
        };
        const QPointF r = p2 - p1;
        const QPointF s = q2 - q1;
        const QPointF qp = q1 - p1;
        const double rxs = cross(r, s);
        if (qFuzzyIsNull(rxs) || nearlyParallel(r, s, kIntersectAngle))
            return false;
        const double t = cross(qp, s) / rxs;
        const double u = cross(qp, r) / rxs;
        return t > 0.0 && t < 1.0 && u > 0.0 && u < 1.0;
    }
} // namespace

NetworkCanvas::NetworkCanvas(QWidget *parent)
    : QWidget(parent), m_gridSize(50), // 默认网格大小50像素
      m_selectedNode(nullptr), m_selectedLink(nullptr),
      m_draggingNode(false), m_draggedNode(nullptr),
      m_linkStartNode(nullptr),
      m_dataTransferEnabled(false)
{
    setMinimumSize(600, 400);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &NetworkCanvas::customContextMenuRequested,
            this, [this](const QPoint &pos)
            {
            QContextMenuEvent event(QContextMenuEvent::Mouse, pos, mapToGlobal(pos));
            contextMenuEvent(&event); });
}

NetworkCanvas::~NetworkCanvas()
{
    // 清理节点和链路
    qDeleteAll(m_links);
    qDeleteAll(m_nodes);
}

void NetworkCanvas::setGridSize(int size)
{
    if (size > 20 && size < 200)
    { // 限制网格大小范围
        m_gridSize = size;
        update(); // 重绘
    }
}

void NetworkCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.fillRect(rect(), Qt::white);

    // 绘制网格
    painter.setPen(QPen(Qt::lightGray, 1));

    // 绘制水平线
    for (int y = 0; y < height(); y += m_gridSize)
    {
        painter.drawLine(0, y, width(), y);
    }

    // 绘制垂直线
    for (int x = 0; x < width(); x += m_gridSize)
    {
        painter.drawLine(x, 0, x, height());
    }

    // 绘制链路
    foreach (Link *link, m_links)
    {
        if (!link || !link->node1() || !link->node2())
            continue;

        QPoint p1 = link->node1()->position() * m_gridSize;
        QPoint p2 = link->node2()->position() * m_gridSize;

        // 选中的链路使用不同样式
        if (link == m_selectedLink)
        {
            painter.setPen(QPen(Qt::blue, 3));
        }
        else
        {
            painter.setPen(QPen(link->statusColor(), 2));
        }

        painter.drawLine(p1, p2);

        // 绘制链路中间点（用于选择）
        QPoint midPoint = (p1 + p2) / 2;
        painter.drawEllipse(midPoint, 5, 5);
    }

    // 绘制临时链路（正在创建的链路）
    if (m_linkStartNode)
    {
        painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));
        painter.drawLine(m_linkStartNode->position() * m_gridSize,
                         mapFromGlobal(QCursor::pos()));
    }

    // 绘制节点
    foreach (ClientNode *node, m_nodes)
    {
        if (!node)
            continue;

        QPoint pos = node->position() * m_gridSize;

        // 选中的节点使用不同颜色
        if (node == m_selectedNode)
        {
            painter.setBrush(Qt::yellow);
        }
        else
        {
            // 根据数据比例调整颜色
            // double dataRatio = node->dataRatio();
            if (!node->hasDataBlocks())
            {
                // 数据比例为0时使用默认绿色
                painter.setBrush(QColor(150, 150, 150));
            }
            else
            {
                // 同色系（蓝色系）设置：固定色相=240（纯蓝）
                int hue = 240;
                // 饱和度固定为200（保证蓝色纯度）
                int saturation = 200;
                // 亮度随比例变化：比例越大，亮度越低（颜色越深）
                // 亮度范围：0.0→220（最浅蓝），1.0→120（最深蓝），线性过渡
                int lightness = qBound(100, (220 - (node->storedBlocks().size() * 20)), 220);

                QColor color;
                color.setHsl(hue, saturation, lightness);
                painter.setBrush(color);
            }
        }

        painter.setPen(QPen(Qt::black, 2));
        painter.drawEllipse(pos, 15, 15);

        // 绘制节点ID（显示后4位）
        painter.drawText(pos.x() - 10, pos.y() + 3, node->id().right(4));
    }
}

void NetworkCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QPoint gridPos = snapToGrid(event->pos()) / m_gridSize;

        // 检查是否点击了节点
        ClientNode *clickedNode = findNodeAt(event->pos());
        if (clickedNode)
        {
            m_selectedNode = clickedNode;
            m_selectedLink = nullptr;
            m_draggingNode = true;
            m_draggedNode = clickedNode;
            emit nodeSelected(clickedNode);
            update();
            if (!m_linkStartNode)
            {
                return;
            }
        }

        // 检查是否点击了链路
        Link *clickedLink = findLinkAt(event->pos());
        if (clickedLink)
        {
            m_selectedLink = clickedLink;
            m_selectedNode = nullptr;
            emit linkSelected(clickedLink);
            update();
            return;
        }

        // 如果正在创建链路
        if (m_linkStartNode)
        {
            ClientNode *endNode = findNodeAt(event->pos());
            if (endNode && endNode != m_linkStartNode)
            {
                // 检查链路是否已存在
                bool linkExists = false;
                foreach (Link *link, m_links)
                {
                    if ((link->node1() == m_linkStartNode && link->node2() == endNode) ||
                        (link->node1() == endNode && link->node2() == m_linkStartNode))
                    {
                        linkExists = true;
                        break;
                    }
                }

                if (!linkExists)
                {
                    Link *newLink = new Link(m_linkStartNode, endNode);
                    m_links.append(newLink);
                }
            }
            m_linkStartNode = nullptr;
            update();
            return;
        }

        // 未点击任何对象
        m_selectedNode = nullptr;
        m_selectedLink = nullptr;
        emit nothingSelected();
        update();
    }
}

void NetworkCanvas::mouseMoveEvent(QMouseEvent *event)
{
#if 0
    if (m_draggingNode && m_draggedNode)
    {
        QPoint gridPos = snapToGrid(event->pos()) / m_gridSize;
        m_draggedNode->setPosition(gridPos);

        // 更新与该节点相关的链路距离
        foreach (Link *link, m_links)
        {
            if (link->node1() == m_draggedNode || link->node2() == m_draggedNode)
            {
                QPoint p1 = link->node1()->position();
                QPoint p2 = link->node2()->position();
                double dist = std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
                link->setDistance(dist);
            }
        }

        emit nodeSelected(m_draggedNode); // 更新属性视图
        update();
    }
#endif
}

void NetworkCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_draggingNode = false;
}

void NetworkCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    // 添加节点动作（空白区域右键显示）
    QAction *addNodeAction = menu.addAction("添加节点");
    connect(addNodeAction, &QAction::triggered, this, &NetworkCanvas::addNode);

    // 检查是否右键点击了节点
    QPoint gridPos = snapToGrid(event->pos()) / m_gridSize;
    ClientNode *clickedNode = findNodeAt(event->pos());
    Link *clickedLink = findLinkAt(event->pos());

    if (clickedNode)
    {
        menu.removeAction(addNodeAction);
        // 添加链路动作（已实现基础）
        QAction *addLinkAction = menu.addAction("添加链路从此节点开始");
        connect(addLinkAction, &QAction::triggered, this, [this, clickedNode]()
                {
            m_linkStartNode = clickedNode;
            update(); });

        // 新增：删除节点动作
        QAction *deleteNodeAction = menu.addAction("删除节点");
        connect(deleteNodeAction, &QAction::triggered, this, [this, clickedNode]()
                {
            // 先删除与该节点相关的所有链路
            QVector<Link*> linksToRemove;
            foreach (Link* link, m_links) {
                if (link->node1() == clickedNode || link->node2() == clickedNode) {
                    linksToRemove.append(link);
                }
            }
            foreach (Link* link, linksToRemove) {
                m_links.removeOne(link);
                delete link;
            }
            
            // 再删除节点本身
            m_nodes.removeOne(clickedNode);
            delete clickedNode;
            
            // 重置选中状态
            if (m_selectedNode == clickedNode) {
                m_selectedNode = nullptr;
                emit nothingSelected();
            }
            
            update(); });
    }
    else if (clickedLink)
    {
        menu.removeAction(addNodeAction);

        QAction *deleteLinkAction = menu.addAction("删除链路");
        connect(deleteLinkAction, &QAction::triggered, this, [this, clickedLink]()
                {
            // 从链路列表中移除并删除
            m_links.removeOne(clickedLink);
            delete clickedLink;
            
            // 重置选中状态
            if (m_selectedLink == clickedLink) {
                m_selectedLink = nullptr;
                emit nothingSelected();
            }
            
            update(); });
    }

    menu.exec(event->globalPos());
}

void NetworkCanvas::addNode()
{
    QPoint gridPos = snapToGrid(mapFromGlobal(QCursor::pos())) / m_gridSize;

    // 检查该位置是否已有节点
    if (!findNodeAt(mapFromGlobal(QCursor::pos())))
    {
        ClientNode *newNode = new ClientNode(gridPos.x(), gridPos.y());
        m_nodes.append(newNode);
        m_selectedNode = newNode;
        m_selectedLink = nullptr;
        emit nodeSelected(newNode);
        update();
    }
}

QPoint NetworkCanvas::snapToGrid(const QPoint &pos) const
{
    int x = (pos.x() / m_gridSize) * m_gridSize;
    int y = (pos.y() / m_gridSize) * m_gridSize;
    return QPoint(x, y);
}

ClientNode *NetworkCanvas::findNodeAt(const QPoint &pos) const
{
    // 节点在画布上的实际绘制半径（15像素，与paintEvent中保持一致）
    const int nodeRadius = 15;
    // 将网格坐标转换为实际像素坐标
    QPoint pixelPos = pos * m_gridSize;

    foreach (ClientNode *node, m_nodes)
    {
        if (!node)
            continue;

        // 节点中心的像素坐标
        QPoint nodePixelPos = node->position() * m_gridSize;
        // 计算点击位置与节点中心的距离（像素单位）
        double dist = distance(pos, nodePixelPos);

        // 如果距离小于节点半径，则视为选中该节点
        if (dist <= nodeRadius)
        {
            return node;
        }
    }
    return nullptr;
}

Link *NetworkCanvas::findLinkAt(const QPoint &pos) const
{
    // 检查是否点击了链路的中间点
    foreach (Link *link, m_links)
    {
        if (!link || !link->node1() || !link->node2())
            continue;

        QPoint p1 = link->node1()->position() * m_gridSize;
        QPoint p2 = link->node2()->position() * m_gridSize;
        QPoint midPoint = (p1 + p2) / 2;

        if (distance(pos, midPoint) < 10)
        { // 10像素内视为点击了链路
            return link;
        }
    }
    return nullptr;
}

double NetworkCanvas::distance(const QPoint &p1, const QPoint &p2) const
{
    return std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
}

// 生成随机连通图（5-15个节点，基础树+随机额外链路）
void NetworkCanvas::generateRandomConnectedGraph(int minNodes, int maxNodes,
                                                 int fileCount, int blocksPerFile,
                                                 int minReplica, int maxReplica,
                                                 bool randomBlockSize)
{
    clearAll(); // 先清空现有内容

    int actualMin = qMax(2, minNodes);
    int actualMax = qMax(actualMin, maxNodes);

    // 随机生成5-15个节点
    int nodeCount = QRandomGenerator::global()->bounded(actualMin, actualMax + 1);
    auto nodes = generateRandomNodes(nodeCount);
    m_nodes = nodes;

    // 生成最小生成树（保证连通性）
    generateSpanningTree(nodes);

    // 随机添加额外链路（0到节点数-1条）
    int extraLinks = QRandomGenerator::global()->bounded(nodeCount);
    addRandomExtraLinks(nodes, extraLinks);

    if (m_dataTransferEnabled)
    {
        allocateRandomDataBlocks(nodes, fileCount, blocksPerFile, minReplica, maxReplica, randomBlockSize);
    }

    update(); // 重绘
    emit contentModified();
}

// 生成随机节点（避免重叠）
QVector<ClientNode *> NetworkCanvas::generateRandomNodes(int count)
{
    QVector<ClientNode *> nodes;
    int maxX = (width() / m_gridSize) - 2; // 边界留出1格
    int maxY = (height() / m_gridSize) - 2;

    for (int i = 0; i < count; ++i)
    {
        // 随机位置（确保不超出画布且不重叠）
        while (true)
        {
            int x = QRandomGenerator::global()->bounded(1, maxX);
            int y = QRandomGenerator::global()->bounded(1, maxY);
            QPoint pos(x, y);

            // 检查是否与已有节点重叠
            bool overlap = false;
            foreach (auto node, nodes)
            {
                if (node->position() == pos || distance(node->position(), pos) < std::sqrt(std::pow(2, 2) + std::pow(2, 2)))
                {
                    overlap = true;
                    break;
                }
            }
            if (!overlap)
            {
                ClientNode *node = new ClientNode(x, y);

                if (m_dataTransferEnabled)
                {
                    if (QRandomGenerator::global()->bounded(10) < 3)
                    { // 30%概率
                        double dataRatio = 0.1 + (QRandomGenerator::global()->generateDouble() * 0.9);
                        node->setDataRatio(dataRatio);
                    }
                }

                nodes.append(node);

                break;
            }
        }
    }
    return nodes;
}

// 生成最小生成树（Prim算法简化版）
void NetworkCanvas::generateSpanningTree(QVector<ClientNode *> &nodes)
{
    int n = nodes.size();
    if (n < 2)
        return;

    QVector<bool> inTree(n, false);
    QVector<double> minDist(n, std::numeric_limits<double>::max());
    QVector<int> parent(n, -1);

    minDist[0] = 0.0;

    for (int i = 0; i < n; ++i)
    {
        int u = -1;
        for (int j = 0; j < n; ++j)
        {
            if (!inTree[j] && (u == -1 || minDist[j] < minDist[u]))
                u = j;
        }

        inTree[u] = true;

        if (parent[u] != -1)
        {
            Link *link = new Link(nodes[parent[u]], nodes[u]);
            link->setBandwidth(500 + QRandomGenerator::global()->bounded(1500));
            link->setCongestion(QRandomGenerator::global()->bounded(0.5));
            m_links.append(link);
        }

        for (int v = 0; v < n; ++v)
        {
            if (!inTree[v])
            {
                double dist = distance(nodes[u]->position(), nodes[v]->position());
                if (dist < minDist[v])
                {
                    minDist[v] = dist;
                    parent[v] = u;
                }
            }
        }
    }
}

// 随机添加额外链路（不重复）
void NetworkCanvas::addRandomExtraLinks(QVector<ClientNode *> &nodes, int extraCount)
{
    const int n = nodes.size();
    if (n < 2 || extraCount <= 0)
        return;

    int added = 0;
    int attempts = 0;
    const int maxAttempts = extraCount * 20; // 防死循环

    while (added < extraCount && attempts < maxAttempts)
    {
        attempts++;

        int i = QRandomGenerator::global()->bounded(n);
        int j = QRandomGenerator::global()->bounded(n);
        if (i == j)
            continue;

        ClientNode *n1 = nodes[i];
        ClientNode *n2 = nodes[j];

        // 是否已存在
        bool exists = false;
        for (const Link *lnk : m_links)
        {
            if ((lnk->node1() == n1 && lnk->node2() == n2) ||
                (lnk->node1() == n2 && lnk->node2() == n1))
            {
                exists = true;
                break;
            }
        }
        if (exists)
            continue;

        // 是否与已有边相交
        QPointF p1 = n1->position();
        QPointF p2 = n2->position();
        bool intersects = false;
        for (const Link *lnk : m_links)
        {
            QPointF q1 = lnk->node1()->position();
            QPointF q2 = lnk->node2()->position();
            if (segmentsIntersect(p1, p2, q1, q2))
            {
                intersects = true;
                break;
            }
        }
        if (intersects)
            continue;

        double minAngle = 25.0; // 阈值可调
        auto tooSharp = [&](ClientNode *n, const QPointF &newP)
        {
            for (const Link *lnk : m_links)
            {
                if (lnk->node1() != n && lnk->node2() != n)
                    continue;
                QPointF oldP = (lnk->node1() == n) ? lnk->node2()->position()
                                                   : lnk->node1()->position();
                QPointF r = oldP - n->position();
                QPointF s = newP - n->position();
                if (nearlyParallel(r, s, minAngle))
                    return true; // 夹角太小
            }
            return false;
        };

        if (tooSharp(n1, p2) || tooSharp(n2, p1))
            continue;

        // 可以添加
        Link *lnk = new Link(n1, n2);
        lnk->setBandwidth(500 + QRandomGenerator::global()->bounded(1500));
        lnk->setCongestion(QRandomGenerator::global()->bounded(0.8));
        m_links.append(lnk);
        added++;
    }
}

void NetworkCanvas::allocateRandomDataBlocks(
    const QVector<ClientNode *> &nodes,
    int fileCount,
    int blocksPerFile,
    int minReplica,
    int maxReplica,
    bool randomBlockSize)
{
    if (m_nodes.isEmpty() || fileCount <= 0 || blocksPerFile <= 0)
        return;

    // 1. 随机选择部分节点作为"存储节点"（例如总节点的60%）
    int storageNodeCount = qMax(1, (int)(m_nodes.size() * 0.6)); // 至少1个存储节点
    QVector<ClientNode *> storageNodes = m_nodes;
    std::shuffle(storageNodes.begin(), storageNodes.end(), std::default_random_engine(QRandomGenerator::global()->generate()));
    storageNodes.resize(storageNodeCount);

    // 2. 生成文件及数据块
    for (int fileIdx = 0; fileIdx < fileCount; ++fileIdx)
    {
        QString fileId = QString(QChar('A' + fileIdx)); // 文件标识：F1, F2...
        double totalFileSize = 0.0;

        // 2.1 计算每个数据块大小（10-100MB）
        QVector<double> blockSizes;
        for (int i = 0; i < blocksPerFile; ++i)
        {
            double size;
            if (randomBlockSize)
            {
                // 随机大小：10-100MB
                size = 10.0 + QRandomGenerator::global()->generateDouble() * 90.0;
            }
            else
            {
                // 固定大小：50MB（可根据需求调整默认值）
                size = 50.0;
            }
            blockSizes.append(size);
            totalFileSize += size;
        }

        // 2.2 为每个数据块分配主块和备份
        for (int blockIdx = 0; blockIdx < blocksPerFile; ++blockIdx)
        {
            double blockSize = blockSizes[blockIdx];
            double ratio = totalFileSize > 0 ? blockSize / totalFileSize : 0.0;

            // 随机生成副本数（在配置范围内）
            int replicaCount = minReplica + QRandomGenerator::global()->bounded(maxReplica - minReplica + 1);
            int totalCopies = 1 + replicaCount; // 1个主块 + N个备份

            // 从存储节点中选择不重复的节点存储主块和备份
            if (storageNodes.size() < totalCopies)
            {
                continue; // 存储节点不足时跳过（实际应增加判断）
            }

            QVector<ClientNode *> selectedNodes;
            QSet<int> usedIndices;
            for (int i = 0; i < totalCopies; ++i)
            {
                // 随机选择未使用的存储节点
                int idx;
                do
                {
                    idx = QRandomGenerator::global()->bounded(storageNodes.size());
                } while (usedIndices.contains(idx));
                usedIndices.insert(idx);
                selectedNodes.append(storageNodes[idx]);
            }

            // 分配主块（第0个为为）
            selectedNodes[0]->addDataBlock(DataBlockInfo(
                fileId, blockIdx + 1, blockSize, ratio, false));

            // 分配备份块（剩余为备份）
            for (int i = 1; i < totalCopies; ++i)
            {
                selectedNodes[i]->addDataBlock(DataBlockInfo(
                    fileId, blockIdx + 1, blockSize, ratio, true));
            }
        }
    }
}

// 初始化归一化参数（缓存最大值）
void NetworkCanvas::initNormalizationParams() const
{
    if (m_maxProcessingCapability > 0)
        return; // 已初始化

    // 计算节点属性最大值
    for (ClientNode *node : m_nodes)
    {
        m_maxProcessingCapability = qMax(m_maxProcessingCapability, node->processingCapability());
    }

    // 计算链路属性最大值
    for (Link *link : m_links)
    {
        m_maxBandwidth = qMax(m_maxBandwidth, link->bandwidth());
        m_maxDistance = qMax(m_maxDistance, link->distance());
    }

    // 避免除零（设置默认最小值）
    if (m_maxProcessingCapability <= 0)
        m_maxProcessingCapability = 1.0;
    if (m_maxBandwidth <= 0)
        m_maxBandwidth = 1.0;
    if (m_maxDistance <= 0)
        m_maxDistance = 1.0;
}

// 计算单条有向边的权重
double NetworkCanvas::calculateDirectedEdgeWeight(ClientNode *source, ClientNode *target, Link *link) const
{
    initNormalizationParams();

    // 1. 归一化节点属性（映射到0-1范围）
    double normSourceLoad = 1.0 - source->loadStatus();                                       // 源节点负载反向（0-1）
    double normTargetProcessing = target->processingCapability() / m_maxProcessingCapability; // 目标节点处理能力（0-1）
    double normTargetStability = target->stability();                                         // 目标节点稳定性（已在0-1）

    // 2. 归一化链路属性（映射到0-1范围）
    double normBandwidth = link->bandwidth() / m_maxBandwidth;            // 带宽（0-1）
    double normCongestion = 1.0 - link->congestion();                     // 拥塞反向（0-1）
    double normDistance = 1.0 / (1.0 + link->distance() / m_maxDistance); // 距离反向（0-1）

    // 3. 加权计算总价值（权重系数可根据业务调整）
    const double wLoad = 0.15;       // 源节点负载权重
    const double wProcessing = 0.25; // 目标节点处理能力权重
    const double wStability = 0.1;   // 目标节点稳定性权重
    const double wBandwidth = 0.2;   // 链路带宽权重
    const double wCongestion = 0.2;  // 链路拥塞权重
    const double wDistance = 0.1;    // 链路距离权重

    double totalValue =
        wLoad * normSourceLoad +
        wProcessing * normTargetProcessing +
        wStability * normTargetStability +
        wBandwidth * normBandwidth +
        wCongestion * normCongestion +
        wDistance * normDistance;

    return totalValue;
}

// 获取有向边权重（从源到目标）
double NetworkCanvas::getDirectedEdgeWeight(ClientNode *source, ClientNode *target) const
{
    if (!source || !target)
        return 0.0;

    // 检查缓存，避免重复计算
    auto key = qMakePair(source, target);
    if (m_directedEdgeWeights.contains(key))
    {
        return m_directedEdgeWeights[key];
    }

    // 查找连接源和目标的链路
    Link *link = nullptr;
    for (Link *l : m_links)
    {
        if ((l->node1() == source && l->node2() == target) ||
            (l->node1() == target && l->node2() == source))
        {
            link = l;
            break;
        }
    }

    if (!link)
        return 0.0; // 无直接连接的链路

    // 计算权重（有向性体现在源和目标的属性差异）
    double weight = calculateDirectedEdgeWeight(source, target, link);
    m_directedEdgeWeights[key] = weight; // 缓存结果

    return weight;
}
