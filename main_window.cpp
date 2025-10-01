#include "main_window.h"
#include <QMenuBar>
#include <QMenu>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDataStream>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("分布式存储可视化系统");
    setMinimumSize(1000, 600);

    // 创建中心部件和布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 创建属性视图
    m_propertyView = new QTreeWidget(this);
    m_propertyView->setHeaderLabels(QStringList() << "属性"
                                                  << "值");
    m_propertyView->setAlternatingRowColors(true);                     // 交替行颜色
    m_propertyView->setEditTriggers(QAbstractItemView::DoubleClicked); // 双击编辑
    mainLayout->addWidget(m_propertyView, 1);                          // 占1/4宽度

    // 创建画布
    m_canvas = new NetworkCanvas(this);
    mainLayout->addWidget(m_canvas, 3); // 占3/4宽度

    setCentralWidget(centralWidget);

    // 创建菜单
    createMenus();

    // 连接信号槽
    connect(m_canvas, &NetworkCanvas::nodeSelected, this, &MainWindow::onNodeSelected);
    connect(m_canvas, &NetworkCanvas::linkSelected, this, &MainWindow::onLinkSelected);
    connect(m_canvas, &NetworkCanvas::nothingSelected, this, &MainWindow::onNothingSelected);
    connect(m_propertyView, &QTreeWidget::itemChanged, this, &MainWindow::onItemEdited);

    connect(m_canvas, &NetworkCanvas::contentModified, this, [this]()
            { m_isModified = true; });
}

MainWindow::~MainWindow()
{
    // 资源由Qt父对象机制自动管理
}

void MainWindow::createMenus()
{
    // 创建文件菜单
    QMenu *fileMenu = menuBar()->addMenu("文件");

    // 文件菜单选项
    QAction *openAction = new QAction("打开文件", this);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    fileMenu->addAction(openAction);

    QAction *saveAction = new QAction("保存文件", this);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    fileMenu->addAction(saveAction);

    QAction *saveAsAction = new QAction("另存为", this);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsFile);
    fileMenu->addAction(saveAsAction);

    fileMenu->addSeparator(); // 添加分隔线

    QAction *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);

    // 创建视图菜单（原有代码）
    QMenu *viewMenu = menuBar()->addMenu("视图");
    m_gridSizeAction = new QAction("网格大小", this);
    connect(m_gridSizeAction, &QAction::triggered, this, &MainWindow::onGridSizeChanged);
    viewMenu->addAction(m_gridSizeAction);

    // 创建仿真菜单
    QMenu *simulationMenu = menuBar()->addMenu("仿真");

    // 仿真菜单选项
    QAction *dataTransAction = new QAction("数据传输", this);
    simulationMenu->addAction(dataTransAction);

    QAction *faultHandleAction = new QAction("故障处理", this);
    simulationMenu->addAction(faultHandleAction);

    // 添加帮助菜单
    QMenu *helpMenu = menuBar()->addMenu("帮助");

    // 帮助菜单选项
    QAction *manualAction = new QAction("使用手册", this);
    helpMenu->addAction(manualAction);

    QAction *shortcutAction = new QAction("快捷键", this);
    helpMenu->addAction(shortcutAction);
}

void MainWindow::onNodeSelected(ClientNode *node)
{
    if (node)
    {
        updatePropertyView(node);
    }
}

void MainWindow::onLinkSelected(Link *link)
{
    if (link)
    {
        updatePropertyView(link);
    }
}

void MainWindow::onNothingSelected()
{
    clearPropertyView();
    m_propertyView->setHeaderLabel("未选择任何对象");
}

void MainWindow::onGridSizeChanged()
{
    bool ok;
    int currentSize = m_canvas->gridSize();
    int newSize = QInputDialog::getInt(this, "网格大小设置",
                                       "请输入网格大小(20-200):",
                                       currentSize, 20, 200, 1, &ok);

    if (ok)
    {
        m_canvas->setGridSize(newSize);
    }
}

void MainWindow::onItemEdited(QTreeWidgetItem *item, int column)
{
    if (column != 1)
        return; // 只处理值列的编辑

    QString property = item->text(0);
    QString valueStr = item->text(1);
    bool ok;
    double value = valueStr.toDouble(&ok);

    if (!ok)
        return; // 转换失败

    // 更新节点属性
    if (m_canvas->selectedNode())
    {
        ClientNode *node = m_canvas->selectedNode();

        if (property == "存储容量(GB)")
        {
            node->setStorageCapacity(value);
        }
        else if (property == "计算能力(GFLOPS)")
        {
            node->setComputingPower(value);
        }
        else if (property == "访问频率(次/秒)")
        {
            node->setAccessFrequency(value);
        }
        else if (property == "收发处理能力(Mbps)")
        {
            node->setProcessingCapability(value);
        }
        else if (property == "负载状态(0-1)")
        {
            node->setLoadStatus(qBound(0.0, value, 1.0));
        }
        else if (property == "稳定性(0-1)")
        {
            node->setStability(qBound(0.0, value, 1.0));
        }
    }
    // 更新链路属性
    else if (m_canvas->selectedLink())
    {
        Link *link = m_canvas->selectedLink();

        if (property == "传输带宽(Mbps)")
        {
            link->setBandwidth(value);
        }
        else if (property == "拥塞程度(0-1)")
        {
            link->setCongestion(qBound(0.0, value, 1.0));
        }
        else if (property == "链路距离(网格单位)")
        {
            link->setDistance(value);
        }
    }

    m_canvas->update(); // 重绘画布以反映变化
    m_isModified = true;
}

void MainWindow::updatePropertyView(ClientNode *node)
{
    clearPropertyView();
    m_propertyView->setHeaderLabel("节点属性");

    // 节点名称
    QTreeWidgetItem *nameItem = new QTreeWidgetItem();
    nameItem->setText(0, "节点名称");
    nameItem->setText(1, node->name());
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // 禁止编辑
    m_propertyView->addTopLevelItem(nameItem);

    // 添加节点属性
    QTreeWidgetItem *storageItem = new QTreeWidgetItem();
    storageItem->setText(0, "存储容量(GB)");
    storageItem->setText(1, QString::number(node->storageCapacity()));
    storageItem->setFlags(storageItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(storageItem);

    QTreeWidgetItem *computeItem = new QTreeWidgetItem();
    computeItem->setText(0, "计算能力(GFLOPS)");
    computeItem->setText(1, QString::number(node->computingPower()));
    computeItem->setFlags(computeItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(computeItem);

    QTreeWidgetItem *accessItem = new QTreeWidgetItem();
    accessItem->setText(0, "访问频率(次/秒)");
    accessItem->setText(1, QString::number(node->accessFrequency()));
    accessItem->setFlags(accessItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(accessItem);

    QTreeWidgetItem *processItem = new QTreeWidgetItem();
    processItem->setText(0, "收发处理能力(Mbps)");
    processItem->setText(1, QString::number(node->processingCapability()));
    processItem->setFlags(processItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(processItem);

    QTreeWidgetItem *loadItem = new QTreeWidgetItem();
    loadItem->setText(0, "负载状态(0-1)");
    loadItem->setText(1, QString::number(node->loadStatus()));
    loadItem->setFlags(loadItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(loadItem);

    QTreeWidgetItem *stableItem = new QTreeWidgetItem();
    stableItem->setText(0, "稳定性(0-1)");
    stableItem->setText(1, QString::number(node->stability()));
    stableItem->setFlags(stableItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(stableItem);

    m_propertyView->resizeColumnToContents(0);
}

void MainWindow::updatePropertyView(Link *link)
{
    clearPropertyView();
    m_propertyView->setHeaderLabel("链路属性");

    // 源节点信息
    QTreeWidgetItem *sourceItem = new QTreeWidgetItem();
    sourceItem->setText(0, "源节点");
    sourceItem->setText(1, link->node1()->name() + " (" + link->node1()->id().right(4) + ")");
    sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable); // 不可编辑
    m_propertyView->addTopLevelItem(sourceItem);

    // 终节点信息
    QTreeWidgetItem *destItem = new QTreeWidgetItem();
    destItem->setText(0, "终节点");
    destItem->setText(1, link->node2()->name() + " (" + link->node2()->id().right(4) + ")");
    destItem->setFlags(destItem->flags() & ~Qt::ItemIsEditable); // 不可编辑
    m_propertyView->addTopLevelItem(destItem);

    m_propertyView->addTopLevelItem(new QTreeWidgetItem()); // 添加空行分隔

    // 传输带宽
    QTreeWidgetItem *bandwidthItem = new QTreeWidgetItem();
    bandwidthItem->setText(0, "传输带宽(Mbps)");
    bandwidthItem->setText(1, QString::number(link->bandwidth()));
    bandwidthItem->setFlags(bandwidthItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(bandwidthItem);

    // 拥塞程度
    QTreeWidgetItem *congestionItem = new QTreeWidgetItem();
    congestionItem->setText(0, "拥塞程度(0-1)");
    congestionItem->setText(1, QString::number(link->congestion()));
    congestionItem->setFlags(congestionItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(congestionItem);

    // 链路距离
    QTreeWidgetItem *distanceItem = new QTreeWidgetItem();
    distanceItem->setText(0, "链路距离(网格单位)");
    distanceItem->setText(1, QString::number(link->distance()));
    distanceItem->setFlags(distanceItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(distanceItem);

    m_propertyView->resizeColumnToContents(0);
}

void MainWindow::clearPropertyView()
{
    m_propertyView->clear();
}
// 保存文件
void MainWindow::onSaveFile()
{
    if (m_currentFileName.isEmpty())
    {
        onSaveAsFile();
    }
    else
    {
        saveFile(m_currentFileName);
    }
#if 0
    // 获取可执行文件所在目录
    QString appPath = QCoreApplication::applicationDirPath();
    // 创建data文件夹（如果不存在）
    QString dataDir = appPath + "/data";
    QDir().mkpath(dataDir);

    // 生成时间戳作为文件名（例如：20231025153045.txt）
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString filePath = dataDir + "/" + timeStamp + ".txt";

    // 创建JSON文档
    QJsonObject root;

    // 保存节点数据
    QJsonArray nodesArray;
    foreach (ClientNode *node, m_canvas->getNodes())
    {
        QJsonObject nodeObj;
        nodeObj["id"] = node->id();
        nodeObj["name"] = node->name();
        nodeObj["x"] = node->position().x();
        nodeObj["y"] = node->position().y();
        nodeObj["storageCapacity"] = node->storageCapacity();
        nodeObj["computingPower"] = node->computingPower();
        nodeObj["accessFrequency"] = node->accessFrequency();
        nodeObj["processingCapability"] = node->transceiverCapability();
        nodeObj["loadStatus"] = node->loadStatus();
        nodeObj["stability"] = node->stability();
        nodesArray.append(nodeObj);
    }
    root["nodes"] = nodesArray;

    // 保存链路数据
    QJsonArray linksArray;
    foreach (Link *link, m_canvas->getLinks())
    {
        QJsonObject linkObj;
        linkObj["node1Id"] = link->node1()->id();
        linkObj["node2Id"] = link->node2()->id();
        linkObj["bandwidth"] = link->bandwidth();
        linkObj["congestion"] = link->congestion();
        linkObj["distance"] = link->distance();
        linksArray.append(linkObj);
    }
    root["links"] = linksArray;

    // 写入文件
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << QJsonDocument(root).toJson(QJsonDocument::Indented);
        file.close();
    }
#endif
}

// 另存为
void MainWindow::onSaveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存文件", "",
                                                    "网络拓扑文件 (*.net);;所有文件 (*)");

    if (!fileName.isEmpty())
    {
        saveFile(fileName);
    }
}

// 打开文件
void MainWindow::onOpenFile()
{
    if (maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this, "打开文件", "",
                                                        "网络拓扑文件 (*.net);;所有文件 (*)");

        if (!fileName.isEmpty())
        {
            loadFile(fileName);
        }
    }
}

// 实际保存逻辑
bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "错误", "无法保存文件: " + file.errorString());
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_13);

    // 保存节点数量
    const auto &nodes = m_canvas->getNodes(); // 需要在NetworkCanvas中添加getNodes()方法
    out << (quint32)nodes.size();

    // 保存每个节点的属性
    for (ClientNode *node : nodes)
    {
        out << node->id() << node->name() << node->position();
        out << node->processingCapability() << node->storageCapacity();
        out << node->accessFrequency() << node->transceiverCapability();
        out << node->loadStatus() << node->stability();
    }

    // 保存链路数量
    const auto &links = m_canvas->getLinks(); // 需要在NetworkCanvas中添加getLinks()方法
    out << (quint32)links.size();

    // 保存每个链路的属性
    for (Link *link : links)
    {
        // 保存节点ID用于恢复连接
        out << link->node1()->id() << link->node2()->id();
        out << link->bandwidth() << link->congestion() << link->distance();
    }

    m_currentFileName = fileName;
    m_isModified = false;
    setWindowTitle("分布式存储可视化系统 - " + fileName);
    return true;
}

// 加载文件
bool MainWindow::loadFile(const QString &fileName)
{
    // 清空现有数据
    m_canvas->clearAll(); // 需要在NetworkCanvas中添加clearAll()方法

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "错误", "无法打开文件: " + file.errorString());
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_13);

    // 加载节点
    quint32 nodeCount;
    in >> nodeCount;

    QMap<QString, ClientNode *> nodeMap; // 用于通过ID查找节点
    for (quint32 i = 0; i < nodeCount; ++i)
    {
        QString id, name;
        QPoint pos;
        double processing, storage, access, transceiver, load, stability;

        in >> id >> name >> pos;
        in >> processing >> storage >> access >> transceiver >> load >> stability;

        ClientNode *node = new ClientNode(pos.x(), pos.y());
        // 注意：需要修改ClientNode构造函数或添加setId()方法来设置ID
        node->setName(name);
        node->setProcessingCapability(processing);
        node->setStorageCapacity(storage);
        node->setAccessFrequency(access);
        node->setTransceiverCapability(transceiver);
        node->setLoadStatus(load);
        node->setStability(stability);

        m_canvas->addNodeFromFile(node); // 需要在NetworkCanvas中添加addNode()方法
        nodeMap[id] = node;
    }

    // 加载链路
    quint32 linkCount;
    in >> linkCount;

    for (quint32 i = 0; i < linkCount; ++i)
    {
        QString id1, id2;
        double bandwidth, congestion, distance;

        in >> id1 >> id2;
        in >> bandwidth >> congestion >> distance;

        if (nodeMap.contains(id1) && nodeMap.contains(id2))
        {
            Link *link = new Link(nodeMap[id1], nodeMap[id2]);
            link->setBandwidth(bandwidth);
            link->setCongestion(congestion);
            link->setDistance(distance);
            m_canvas->addLink(link); // 需要在NetworkCanvas中添加addLink()方法
        }
    }

    m_currentFileName = fileName;
    m_isModified = false;
    setWindowTitle("分布式存储可视化系统 - " + fileName);
    m_canvas->update();
    return true;
}

// 关闭前检查是否需要保存
bool MainWindow::maybeSave()
{
    if (!m_isModified)
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, "提示",
                               "文件已被修改，是否保存?",
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveFile(m_currentFileName.isEmpty() ? QFileDialog::getSaveFileName(this, "保存文件") : m_currentFileName);
    else if (ret == QMessageBox::Cancel)
        return false;

    return true; // 放弃保存
}

// 重写关闭事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}