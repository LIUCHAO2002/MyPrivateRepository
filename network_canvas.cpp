#include "network_canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QInputDialog>
#include <cmath>
#include <qDebug>

NetworkCanvas::NetworkCanvas(QWidget *parent)
    : QWidget(parent), m_gridSize(50), // 默认网格大小50像素
      m_selectedNode(nullptr), m_selectedLink(nullptr),
      m_draggingNode(false), m_draggedNode(nullptr),
      m_linkStartNode(nullptr)
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
            painter.setBrush(Qt::green);
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