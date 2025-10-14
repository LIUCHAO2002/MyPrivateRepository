#include "client_node.h"
#include "utils.h"
#include <QRandomGenerator>

// 初始化静态成员变量（从0开始）
int ClientNode::m_nextId = 0;

ClientNode::ClientNode(int x, int y)
    : m_position(x, y),
      m_id(QString::number(m_nextId++)),
      m_name(QString("Node_%1").arg(m_id)),                  // 初始化名称（使用ID后4位）
      m_storageCapacity(randomInRange(500.0, 2000.0)),       // 默认1000GB
      m_processingCapability(randomInRange(2.0, 32.0)),      // 默认8核等效
      m_accessFrequency(randomInRange(50.0, 200.0)),         // 默认100次/分钟
      m_transceiverCapability(randomInRange(500.0, 2000.0)), // 默认1000Mbps
      m_loadStatus(randomInRange(0.1, 0.8)),                 // 默认30%负载
      m_stability(randomInRange(0.8, 0.99)),                 // 默认95%稳定性
      m_dataRatio(0.0)
{
}

// 实现处理能力设置函数
void ClientNode::setProcessingCapability(double capability)
{
    // 确保值在合理范围内
    if (capability > 0 && capability <= 100)
    {
        m_processingCapability = capability;
    }
}