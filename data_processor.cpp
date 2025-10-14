#include "data_processor.h"
#include <cmath>
#include <QVector>

// 节点属性统计量计算（存储所有必要的统计参数）
QMap<QString, double> DataProcessor::calculateNodeStats(const QVector<ClientNode *> &nodes)
{
    QMap<QString, double> stats;
    if (nodes.isEmpty())
        return stats;

    // 初始化累加器
    double sumStorage = 0, sumComputing = 0, sumAccess = 0;
    double sumStorage2 = 0, sumComputing2 = 0, sumAccess2 = 0;
    double minStorage = nodes[0]->storageCapacity(), maxStorage = minStorage;
    double minComputing = nodes[0]->computingPower(), maxComputing = minComputing;
    double minAccess = nodes[0]->accessFrequency(), maxAccess = minAccess;
    int count = nodes.size();

    // 遍历节点计算统计量
    for (ClientNode *node : nodes)
    {
        double storage = node->storageCapacity();
        double computing = node->computingPower();
        double access = node->accessFrequency();

        // 求和与平方和（用于均值和标准差）
        sumStorage += storage;
        sumComputing += computing;
        sumAccess += access;
        sumStorage2 += storage * storage;
        sumComputing2 += computing * computing;
        sumAccess2 += access * access;

        // 最大/最小值
        minStorage = qMin(minStorage, storage);
        maxStorage = qMax(maxStorage, storage);
        minComputing = qMin(minComputing, computing);
        maxComputing = qMax(maxComputing, computing);
        minAccess = qMin(minAccess, access);
        maxAccess = qMax(maxAccess, access);
    }

    // 计算均值
    double meanStorage = sumStorage / count;
    double meanComputing = sumComputing / count;
    double meanAccess = sumAccess / count;

    // 计算标准差（样本标准差，除以n-1）
    auto calcStd = [count](double sum, double sum2, double mean)
    {
        if (count <= 1)
            return 0.0;
        double variance = (sum2 - 2 * mean * sum + count * mean * mean) / (count - 1);
        return variance > 0 ? std::sqrt(variance) : 0.0;
    };
    double stdStorage = calcStd(sumStorage, sumStorage2, meanStorage);
    double stdComputing = calcStd(sumComputing, sumComputing2, meanComputing);
    double stdAccess = calcStd(sumAccess, sumAccess2, meanAccess);

    // 存储统计量（键格式：属性_统计类型）
    stats["storage_mean"] = meanStorage;
    stats["storage_std"] = stdStorage;
    stats["storage_min"] = minStorage;
    stats["storage_max"] = maxStorage;

    stats["computing_mean"] = meanComputing;
    stats["computing_std"] = stdComputing;
    stats["computing_min"] = minComputing;
    stats["computing_max"] = maxComputing;

    stats["access_mean"] = meanAccess;
    stats["access_std"] = stdAccess;
    stats["access_min"] = minAccess;
    stats["access_max"] = maxAccess;

    return stats;
}

// 链路属性统计量计算
QMap<QString, double> DataProcessor::calculateLinkStats(const QVector<Link *> &links)
{
    QMap<QString, double> stats;
    if (links.isEmpty())
        return stats;

    // 初始化累加器
    double sumBandwidth = 0, sumDistance = 0;
    double sumBandwidth2 = 0, sumDistance2 = 0;
    double minBandwidth = links[0]->bandwidth(), maxBandwidth = minBandwidth;
    double minDistance = links[0]->distance(), maxDistance = minDistance;
    int count = links.size();

    // 遍历链路计算统计量
    for (Link *link : links)
    {
        double bandwidth = link->bandwidth();
        double distance = link->distance();

        sumBandwidth += bandwidth;
        sumDistance += distance;
        sumBandwidth2 += bandwidth * bandwidth;
        sumDistance2 += distance * distance;

        minBandwidth = qMin(minBandwidth, bandwidth);
        maxBandwidth = qMax(maxBandwidth, bandwidth);
        minDistance = qMin(minDistance, distance);
        maxDistance = qMax(maxDistance, distance);
    }

    // 计算均值
    double meanBandwidth = sumBandwidth / count;
    double meanDistance = sumDistance / count;

    // 计算标准差
    auto calcStd = [count](double sum, double sum2, double mean)
    {
        if (count <= 1)
            return 0.0;
        double variance = (sum2 - 2 * mean * sum + count * mean * mean) / (count - 1);
        return variance > 0 ? std::sqrt(variance) : 0.0;
    };
    double stdBandwidth = calcStd(sumBandwidth, sumBandwidth2, meanBandwidth);
    double stdDistance = calcStd(sumDistance, sumDistance2, meanDistance);

    // 存储统计量
    stats["bandwidth_mean"] = meanBandwidth;
    stats["bandwidth_std"] = stdBandwidth;
    stats["bandwidth_min"] = minBandwidth;
    stats["bandwidth_max"] = maxBandwidth;

    stats["distance_mean"] = meanDistance;
    stats["distance_std"] = stdDistance;
    stats["distance_min"] = minDistance;
    stats["distance_max"] = maxDistance;

    return stats;
}

// 归一化实现（处理除零情况）
double DataProcessor::normalize(double value, double min, double max)
{
    if (max - min < 1e-9)
        return 0.0; // 避免除零
    return (value - min) / (max - min);
}

// 标准化实现（处理标准差为零的情况）
double DataProcessor::standardize(double value, double mean, double std)
{
    if (std < 1e-9)
        return 0.0; // 避免除零
    return (value - mean) / std;
}

// 节点属性处理（返回标准化/归一化后的标签）
QMap<QString, double> DataProcessor::processNodeFeatures(ClientNode *node,
                                                         const QMap<QString, double> &stats,
                                                         ProcessType type)
{
    QMap<QString, double> result;

    // 处理存储容量
    if (type == ProcessType::Normalization)
    {
        result["storage_capacity"] = normalize(node->storageCapacity(),
                                               stats["storage_min"],
                                               stats["storage_max"]);
    }
    else
    {
        result["storage_capacity"] = standardize(node->storageCapacity(),
                                                 stats["storage_mean"],
                                                 stats["storage_std"]);
    }

    // 处理计算能力
    if (type == ProcessType::Normalization)
    {
        result["computing_power"] = normalize(node->computingPower(),
                                              stats["computing_min"],
                                              stats["computing_max"]);
    }
    else
    {
        result["computing_power"] = standardize(node->computingPower(),
                                                stats["computing_mean"],
                                                stats["computing_std"]);
    }

    // 处理访问频率
    if (type == ProcessType::Normalization)
    {
        result["access_frequency"] = normalize(node->accessFrequency(),
                                               stats["access_min"],
                                               stats["access_max"]);
    }
    else
    {
        result["access_frequency"] = standardize(node->accessFrequency(),
                                                 stats["access_mean"],
                                                 stats["access_std"]);
    }

    // 负载状态（已在[0,1]范围，无需处理）
    result["load_status"] = node->loadStatus();
    // 稳定性（已在[0,1]范围，无需处理）
    result["stability"] = node->stability();

    return result;
}

// 链路属性处理（返回标准化/归一化后的标签）
QMap<QString, double> DataProcessor::processLinkFeatures(Link *link,
                                                         const QMap<QString, double> &stats,
                                                         ProcessType type)
{
    QMap<QString, double> result;

    // 处理带宽
    if (type == ProcessType::Normalization)
    {
        result["bandwidth"] = normalize(link->bandwidth(),
                                        stats["bandwidth_min"],
                                        stats["bandwidth_max"]);
    }
    else
    {
        result["bandwidth"] = standardize(link->bandwidth(),
                                          stats["bandwidth_mean"],
                                          stats["bandwidth_std"]);
    }

    // 处理距离
    if (type == ProcessType::Normalization)
    {
        result["distance"] = normalize(link->distance(),
                                       stats["distance_min"],
                                       stats["distance_max"]);
    }
    else
    {
        result["distance"] = standardize(link->distance(),
                                         stats["distance_mean"],
                                         stats["distance_std"]);
    }

    // 拥塞程度（已在[0,1]范围，无需处理）
    result["congestion"] = link->congestion();

    return result;
}

QVector<QVector<double>> DataProcessor::generateFeatureMatrix(const QVector<ClientNode *> &nodes)
{
    QVector<QVector<double>> matrix;
    if (nodes.isEmpty())
        return matrix;

    // 1. 计算归一化所需的最大值
    double maxStorage = 0, maxComputing = 0, maxAccess = 0;
    for (ClientNode *node : nodes)
    {
        maxStorage = qMax(maxStorage, node->storageCapacity());
        maxComputing = qMax(maxComputing, node->computingPower());
        maxAccess = qMax(maxAccess, node->accessFrequency());
    }

    // 2. 生成每个节点的特征向量
    for (ClientNode *node : nodes)
    {
        QVector<double> features;

        // 归一化存储容量
        features.append(maxStorage > 0 ? node->storageCapacity() / maxStorage : 0);
        // 归一化计算能力
        features.append(maxComputing > 0 ? node->computingPower() / maxComputing : 0);
        // 负载状态（已在0-1范围）
        features.append(node->loadStatus());
        // 稳定性（已在0-1范围）
        features.append(node->stability());
        // 归一化访问频率
        features.append(maxAccess > 0 ? node->accessFrequency() / maxAccess : 0);

        matrix.append(features);
    }

    return matrix;
}

QVector<QVector<double>> DataProcessor::generateFeatureMatrix(const QVector<Link *> &links)
{
    QVector<QVector<double>> matrix;
    if (links.isEmpty())
        return matrix;

    // 1. 计算归一化所需的最大值（带宽和距离）
    double maxBandwidth = 0, maxDistance = 0;
    for (Link *link : links)
    {
        if (!link)
            continue; // 跳过无效链路
        maxBandwidth = qMax(maxBandwidth, link->bandwidth());
        maxDistance = qMax(maxDistance, link->distance());
    }

    // 2. 为每个链路生成特征向量
    for (Link *link : links)
    {
        if (!link)
            continue; // 跳过无效链路

        QVector<double> features;

        features.append(static_cast<double>(link->node1()->id().toDouble()));
        features.append(static_cast<double>(link->node2()->id().toDouble()));

        // 归一化带宽（避免除零）
        features.append(maxBandwidth > 0 ? link->bandwidth() / maxBandwidth : 0);
        // 归一化距离（避免除零）
        features.append(maxDistance > 0 ? link->distance() / maxDistance : 0);
        // 拥塞程度（已在[0,1]范围，无需额外处理）
        features.append(link->congestion());

        matrix.append(features);
    }

    return matrix;
}