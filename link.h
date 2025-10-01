#ifndef LINK_H
#define LINK_H

#include "client_node.h"
#include <QColor>

class Link {
public:
    Link(ClientNode* node1, ClientNode* node2);
    
    // 属性访问方法
    double bandwidth() const { return m_bandwidth; }
    void setBandwidth(double value) { m_bandwidth = value; }
    
    double congestion() const { return m_congestion; }
    void setCongestion(double value) { m_congestion = value; }
    
    double distance() const { return m_distance; }
    void setDistance(double value) { m_distance = value; }
    
    ClientNode* node1() const { return m_node1; }
    ClientNode* node2() const { return m_node2; }
    
    // 获取链路状态颜色（基于拥塞程度）
    QColor statusColor() const;
    
private:
    ClientNode* m_node1;   // 连接的第一个节点
    ClientNode* m_node2;   // 连接的第二个节点
    double m_bandwidth;    // 传输带宽(Mbps)
    double m_congestion;   // 拥塞程度(0-1)
    double m_distance;     // 链路距离(单位)
};

#endif // LINK_H
