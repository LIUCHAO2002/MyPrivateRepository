#ifndef CLIENT_NODE_H
#define CLIENT_NODE_H

#include <QPoint>
#include <QUuid>

class ClientNode
{
public:
    ClientNode(int x, int y);

    // 节点名称相关方法
    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    // 获取和设置位置
    QPoint position() const { return m_position; }
    void setPosition(const QPoint &pos) { m_position = pos; }

    // 获取节点ID
    QString id() const { return m_id; }

    // 处理能力相关
    double processingCapability() const { return m_processingCapability; }
    void setProcessingCapability(double capability);

    // 兼容方法：计算能力（与processingCapability同义）
    double computingPower() const { return m_processingCapability; }
    void setComputingPower(double power) { setProcessingCapability(power); }

    // 其他属性的getter和setter
    double storageCapacity() const { return m_storageCapacity; }
    void setStorageCapacity(double capacity) { m_storageCapacity = capacity; }

    double accessFrequency() const { return m_accessFrequency; }
    void setAccessFrequency(double frequency) { m_accessFrequency = frequency; }

    double transceiverCapability() const { return m_transceiverCapability; }
    void setTransceiverCapability(double capability) { m_transceiverCapability = capability; }

    double loadStatus() const { return m_loadStatus; }
    void setLoadStatus(double status) { m_loadStatus = status; }

    double stability() const { return m_stability; }
    void setStability(double stability) { m_stability = stability; }

private:
    QPoint m_position;
    QString m_id;
    static int m_nextId;  // 静态变量用于自增ID
    QString m_name;                 // 节点名称
    double m_storageCapacity;       // 存储容量
    double m_processingCapability;  // 计算能力
    double m_accessFrequency;       // 访问频率
    double m_transceiverCapability; // 收发处理能力
    double m_loadStatus;            // 负载状态
    double m_stability;             // 稳定性
};

#endif // CLIENT_NODE_H