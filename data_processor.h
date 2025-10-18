#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <QVector>
#include <QMap>
#include <QString>
#include "client_node.h"
#include "link.h"

// 数据处理类型枚举
enum class ProcessType
{
    Normalization,  // 归一化到[0,1]
    Standardization // 标准化（Z-score）
};

class DataProcessor
{
public:
    // 节点属性处理：返回处理后的属性标签（键为属性名，值为处理后的值）
    static QMap<QString, double> processNodeFeatures(ClientNode *node,
                                                     const QMap<QString, double> &stats,
                                                     ProcessType type);

    // 链路属性处理：返回处理后的属性标签
    static QMap<QString, double> processLinkFeatures(Link *link,
                                                     const QMap<QString, double> &stats,
                                                     ProcessType type);

    // 预计算节点属性的统计量（均值、标准差、最大/最小值）
    static QMap<QString, double> calculateNodeStats(const QVector<ClientNode *> &nodes);

    // 预计算链路属性的统计量
    static QMap<QString, double> calculateLinkStats(const QVector<Link *> &links);

    static QVector<QVector<double>> generateFeatureMatrix(const QVector<ClientNode *> &nodes,
                                                          const QVector<ClientNode *> &bestPath = QVector<ClientNode *>());
    static QVector<QVector<double>> generateFeatureMatrix(const QVector<Link *> &links,
                                                          const QVector<Link *> &bestPath = QVector<Link *>());

    static QStringList getNodeFeatureNames()
    {
        return {
            "pos_x",
            "pos_y",
            "storageCapacity",
            "computingPower",
            "loadStatus",
            "stability",
            "accessFrequency",
            "isTerminal"};
    }

    static QStringList getLinkFeatureNames()
    {
        return {
            "sourceNode",
            "targetNode",
            "bandwidth",
            "distance",
            "congestion"};
    }

private:
    // 归一化处理（Min-Max Scaling）
    static double normalize(double value, double min, double max);

    // 标准化处理（Z-score Standardization）
    static double standardize(double value, double mean, double std);

    // 避免实例化
    DataProcessor() = default;
};

#endif // DATA_PROCESSOR_H