#ifndef PATH_SEARCH_ALGORITHM_H
#define PATH_SEARCH_ALGORITHM_H

#include <QVector>
#include "client_node.h"
#include "network_canvas.h"

class PathSearchAlgorithm
{
public:
    virtual ~PathSearchAlgorithm() = default;

    // 初始化算法
    virtual void initialize(NetworkCanvas *canvas) = 0;

    // 设置算法参数
    virtual void setParameters(const QMap<QString, QVariant> &params) = 0;

    // 执行搜索并返回最佳路径
    virtual QVector<ClientNode *> findBestPath() = 0;

    // 获取搜索结果详情
    virtual QMap<QString, QVariant> getResultDetails() const = 0;
};

#endif // PATH_SEARCH_ALGORITHM_H