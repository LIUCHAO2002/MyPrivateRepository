#include "client_node.h"

// 初始化静态成员变量（从0开始）
int ClientNode::m_nextId = 0;

ClientNode::ClientNode(int x, int y) 
    : m_position(x, y),
      m_id(QString::number(m_nextId++)),
      m_name(QString("Node_%1").arg(m_id)),  // 初始化名称（使用ID后4位）
      m_storageCapacity(1000.0),    // 默认1000GB
      m_processingCapability(8.0),  // 默认8核等效
      m_accessFrequency(100.0),     // 默认100次/分钟
      m_transceiverCapability(1000.0), // 默认1000Mbps
      m_loadStatus(0.3),            // 默认30%负载
      m_stability(0.95),             // 默认95%稳定性
      m_dataRatio(0.0)
{
}

// 实现处理能力设置函数
void ClientNode::setProcessingCapability(double capability) {
    // 确保值在合理范围内
    if (capability > 0 && capability <= 100) {
        m_processingCapability = capability;
    }
}