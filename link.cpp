#include "link.h"
#include <cmath>

Link::Link(ClientNode* node1, ClientNode* node2)
    : m_node1(node1), m_node2(node2),
      m_bandwidth(1000.0),  // 默认1000Mbps
      m_congestion(0.2),    // 默认20%拥塞
      m_distance(0.0)
{
    // 计算节点间距离
    if (node1 && node2) {
        QPoint p1 = node1->position();
        QPoint p2 = node2->position();
        m_distance = std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
    }
}

QColor Link::statusColor() const {
    // 根据拥塞程度返回颜色（绿色-黄色-红色）
    int red = static_cast<int>(255 * m_congestion);
    int green = static_cast<int>(255 * (1 - m_congestion));
    return QColor(red, green, 0);
}
