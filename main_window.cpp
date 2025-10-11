#include "main_window.h"
#include "monte_carlo_path_finder.h"
#include "bidirectional_astar_path_finder.h"
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
#include <QStatusBar>
#include <QQueue>
#include <QSpinBox>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("分布式存储可视化系统");
    setMinimumSize(1000, 600);

    m_minNodeCount = 5;
    m_maxNodeCount = 15;

    // 创建中心部件和布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(5);

    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    leftLayout->setSpacing(5);

    // 创建属性视图
    m_propertyView = new QTreeWidget(this);
    m_propertyView->setHeaderLabels(QStringList() << "属性"
                                                  << "值");
    m_propertyView->setAlternatingRowColors(true);                     // 交替行颜色
    m_propertyView->setEditTriggers(QAbstractItemView::DoubleClicked); // 双击编辑
    leftLayout->addWidget(m_propertyView, 1);                          // 占1/4宽度

    QWidget *pathSearchWidget = new QWidget(this);
    leftLayout->addWidget(pathSearchWidget, 2);

    mainLayout->addWidget(leftWidget, 1);

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

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setMinimumHeight(20);
    m_statusLabel->setText("就绪");

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->setStyleSheet("QStatusBar::item { border: none; }");

    showMaximized();
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

    QAction *exportDataAction = new QAction("导出强化学习训练数据", this);
    connect(exportDataAction, &QAction::triggered, this, &MainWindow::onExportTrainingData);
    fileMenu->addAction(exportDataAction);

    fileMenu->addSeparator(); // 添加分隔线

    QAction *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);
    fileMenu->addAction(exitAction);

    // 创建视图菜单（原有代码）
    QMenu *viewMenu = menuBar()->addMenu("视图");
    m_gridSizeAction = new QAction("网格大小", this);
    connect(m_gridSizeAction, &QAction::triggered, this, &MainWindow::onGridSizeChanged);
    viewMenu->addAction(m_gridSizeAction);

    QAction *clearAction = new QAction("清空", this);
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearAll);
    viewMenu->addAction(clearAction);

    // 创建仿真菜单
    QMenu *simulationMenu = menuBar()->addMenu("仿真");

    // 仿真菜单选项
    QAction *dataTransAction = new QAction("数据传输", this);
    dataTransAction->setObjectName("dataTransAction");
    dataTransAction->setCheckable(true);
    dataTransAction->setShortcutContext(Qt::WindowShortcut);
    connect(dataTransAction, &QAction::triggered, this, &MainWindow::onRefreshGraph);
    simulationMenu->addAction(dataTransAction);

    QAction *faultHandleAction = new QAction("故障处理", this);
    simulationMenu->addAction(faultHandleAction);

    simulationMenu->addSeparator();

    QAction *settingsAction = new QAction("仿真设置", this);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSimulationSettings);
    simulationMenu->addAction(settingsAction);

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
        else if (property == "数据比例(0-1)")
        {
            node->setDataRatio(qBound(0.0, value, 1.0));
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

    QTreeWidgetItem *dataRatioItem = new QTreeWidgetItem();
    dataRatioItem->setText(0, "数据比例(0-1)");
    dataRatioItem->setText(1, QString::number(node->dataRatio()));
    dataRatioItem->setFlags(dataRatioItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(dataRatioItem);

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
    if (m_canvas->getNodes().isEmpty())
    {
        QMessageBox::warning(this, "保存失败", "当前没有节点，无法保存文件！");
        return;
    }

    QString defaultDir = defaultSaveDirectory();

    QString fileName = QFileDialog::getSaveFileName(this, "保存文件", defaultDir,
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
        QString defaultDir = defaultSaveDirectory();

        QString fileName = QFileDialog::getOpenFileName(this, "打开文件", defaultDir,
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
    if (m_canvas->getNodes().isEmpty())
    {
        QMessageBox::warning(this, "保存失败", "当前没有节点，无法保存文件！");
        return false;
    }

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

void MainWindow::onClearAll()
{
    // 确认对话框，防止误操作
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认清空",
                                  "确定要清除所有节点和链路吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        m_canvas->clearAll(); // 调用画布的清空方法
        clearPropertyView();  // 清空属性视图
        m_propertyView->setHeaderLabel("未选择任何对象");
        m_isModified = true; // 标记文件已修改
    }
}

void MainWindow::onRefreshGraph()
{
    // 获取数据传输动作（通过对象名查找，需先设置对象名）
    QAction *dataTransAction = findChild<QAction *>("dataTransAction");
    if (!dataTransAction)
        return;

    bool isChecked = dataTransAction->isChecked();
    m_canvas->setDataTransferEnabled(isChecked);

    // 仅在勾选状态下刷新
    if (dataTransAction->isChecked())
    {
        m_canvas->generateRandomConnectedGraph(m_minNodeCount, m_maxNodeCount); // 重新生成连通图
        m_statusLabel->setText("按下F5刷新"); // 状态栏提示
        m_isModified = true;                  // 标记为已修改
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 仅处理F5键
    if (event->key() == Qt::Key_F5)
    {
        // 查找数据传输动作
        QAction *dataTransAction = findChild<QAction *>("dataTransAction");
        if (!dataTransAction)
        {
            QMainWindow::keyPressEvent(event); // 未找到动作，按默认处理
            return;
        }

        // 仅在勾选状态下执行刷新
        if (dataTransAction->isChecked())
        {
            m_canvas->generateRandomConnectedGraph(m_minNodeCount, m_maxNodeCount);
            m_statusLabel->setText("已刷新数据传输连通图");
            m_isModified = true;
        }
        // 未勾选时不做任何操作（F5无效）
        return;
    }

    // 其他按键按默认逻辑处理
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onExportTrainingData()
{
    const auto &nodes = m_canvas->getNodes();
    if (nodes.isEmpty())
    {
        QMessageBox::warning(this, "导出失败", "无法导出数据：没有节点数据！");
        return;
    }

    if (!isGraphConnected())
    {
        QMessageBox::warning(this, "导出失败", "无法导出数据：图不连通，请确保所有节点都相互连接！");
        return;
    }

    QString defaultDir = defaultTrainDataDirectory();
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    QString defaultFileName = defaultDir + "/train_data_" + timeStamp + ".json";

    QString fileName = QFileDialog::getSaveFileName(this, "导出训练数据", defaultFileName,
                                                    "JSON文件 (*.json);;所有文件 (*)");
    if (fileName.isEmpty())
        return;

    if (exportDataToJson(fileName))
    {
        QMessageBox::information(this, "成功", "训练数据已导出到: " + fileName);
    }
    else
    {
        QMessageBox::warning(this, "失败", "无法导出训练数据");
    }
}

// 实现JSON导出功能
bool MainWindow::exportDataToJson(const QString &fileName)
{
    QJsonObject rlData;

    QJsonObject stateDesc;
    stateDesc["node_features"] = QJsonArray()
                                 << "storage_capacity_norm"  // 归一化存储容量
                                 << "computing_power_norm"   // 归一化计算能力
                                 << "load_status"            // 负载状态（已在0-1范围）
                                 << "stability"              // 稳定性（已在0-1范围）
                                 << "access_frequency_norm"; // 归一化访问频率
    stateDesc["link_features"] = QJsonArray()
                                 << "bandwidth_norm" // 归一化带宽
                                 << "congestion"     // 拥塞程度（已在0-1范围）
                                 << "distance_norm"; // 归一化距离
    rlData["state_description"] = stateDesc;

    QJsonArray nodeFeatures;
    // 先计算节点特征的最大值（用于归一化）
    double maxStorage = 0, maxComputing = 0, maxAccess = 0;
    for (ClientNode *node : m_canvas->getNodes())
    {
        maxStorage = qMax(maxStorage, node->storageCapacity());
        maxComputing = qMax(maxComputing, node->computingPower());
        maxAccess = qMax(maxAccess, node->accessFrequency());
    }
    // 收集每个节点的特征
    for (ClientNode *node : m_canvas->getNodes())
    {
        QJsonObject nodeObj;
        nodeObj["id"] = node->id();
        // 归一化到[0,1]（避免除以0）
        nodeObj["storage_capacity_norm"] = maxStorage > 0 ? node->storageCapacity() / maxStorage : 0;
        nodeObj["computing_power_norm"] = maxComputing > 0 ? node->computingPower() / maxComputing : 0;
        nodeObj["load_status"] = node->loadStatus(); // 已在0-1范围
        nodeObj["stability"] = node->stability();    // 已在0-1范围
        nodeObj["access_frequency_norm"] = maxAccess > 0 ? node->accessFrequency() / maxAccess : 0;
        nodeFeatures.append(nodeObj);
    }
    rlData["nodes"] = nodeFeatures;

    QJsonArray linkFeatures;
    const auto &links = m_canvas->getLinks();
    // 计算链路特征的最大值（用于归一化）
    double maxBandwidth = 0, maxDistance = 0;
    for (Link *link : links)
    {
        maxBandwidth = qMax(maxBandwidth, link->bandwidth());
        maxDistance = qMax(maxDistance, link->distance());
    }
    // 收集每个链路的特征
    for (Link *link : links)
    {
        QJsonObject linkObj;
        linkObj["node1_id"] = link->node1()->id();
        linkObj["node2_id"] = link->node2()->id();
        // 归一化到[0,1]
        linkObj["bandwidth_norm"] = maxBandwidth > 0 ? link->bandwidth() / maxBandwidth : 0;
        linkObj["congestion"] = link->congestion(); // 已在0-1范围
        linkObj["distance_norm"] = maxDistance > 0 ? link->distance() / maxDistance : 0;
        linkFeatures.append(linkObj);
    }
    rlData["links"] = linkFeatures;

    QJsonObject adjacencyList;
    for (ClientNode *node : m_canvas->getNodes())
    {
        QJsonArray neighbors;
        for (Link *link : links)
        {
            if (link->node1() == node)
            {
                neighbors.append(link->node2()->id());
            }
            else if (link->node2() == node)
            {
                neighbors.append(link->node1()->id());
            }
        }
        adjacencyList[node->id()] = neighbors;
    }
    rlData["adjacency_list"] = adjacencyList;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "导出失败", "无法写入文件：" + file.errorString());
        return false;
    }
    QTextStream out(&file);
    out << QJsonDocument(rlData).toJson(QJsonDocument::Indented);
    file.close();

    return true;
}

bool MainWindow::isGraphConnected()
{
    const auto &nodes = m_canvas->getNodes();
    int nodeCount = nodes.size();

    // 节点数为0或1时，默认视为连通
    if (nodeCount <= 1)
    {
        return true;
    }

    // 构建邻接表：记录每个节点的邻居
    QMap<ClientNode *, QList<ClientNode *>> adjacencyList;
    for (ClientNode *node : nodes)
    {
        adjacencyList[node] = QList<ClientNode *>();
    }

    // 根据链路填充邻接表
    const auto &links = m_canvas->getLinks();
    for (Link *link : links)
    {
        ClientNode *node1 = link->node1();
        ClientNode *node2 = link->node2();
        adjacencyList[node1].append(node2);
        adjacencyList[node2].append(node1); // 双向添加（无向图）
    }

    // BFS遍历标记可达节点
    QSet<ClientNode *> visited;
    QQueue<ClientNode *> queue;

    // 从第一个节点开始遍历
    ClientNode *startNode = nodes.first();
    queue.enqueue(startNode);
    visited.insert(startNode);

    while (!queue.isEmpty())
    {
        ClientNode *current = queue.dequeue();
        // 遍历当前节点的所有邻居
        for (ClientNode *neighbor : adjacencyList[current])
        {
            if (!visited.contains(neighbor))
            {
                visited.insert(neighbor);
                queue.enqueue(neighbor);
            }
        }
    }

    // 若所有节点都被访问，则图连通
    return visited.size() == nodeCount;
}

void MainWindow::onFindOptimalPath()
{
    MonteCarloPathFinder finder(m_canvas);
    QVector<ClientNode *> bestPath = finder.findBestPath();

    if (bestPath.isEmpty())
    {
        QMessageBox::information(this, "路径搜索", "没有找到有效路径或没有目标节点");
        return;
    }

    // 显示路径信息
    QString pathInfo;
    for (ClientNode *node : bestPath)
    {
        pathInfo += node->id().right(4) + " -> ";
    }
    pathInfo.chop(4); // 移除最后一个箭头

    QMessageBox::information(this, "最佳路径",
                             QString("找到最佳路径:\n%1\n节点数量: %2")
                                 .arg(pathInfo)
                                 .arg(bestPath.size()));
}

void MainWindow::onSimulationSettings()
{
    int canvasWidth = m_canvas->width();
    int canvasHeight = m_canvas->height();
    int gridSize = m_canvas->gridSize();

    if (gridSize <= 0)
    {
        QMessageBox::warning(this, "参数错误", "网格大小设置无效，请先设置合理的网格大小");
        return;
    }

    int horizontalPoints = (canvasWidth / gridSize) + 1;
    int verticalPoints = (canvasHeight / gridSize) + 1;
    int maxNodeCount = horizontalPoints * verticalPoints;
    // 创建设置对话框
    QDialog dialog(this);
    dialog.setWindowTitle("仿真设置");
    dialog.setFixedSize(300, 150);

    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // 最小节点数量设置
    QHBoxLayout *minLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("最小节点数量:");
    QSpinBox *minSpin = new QSpinBox();
    minSpin->setRange(5, maxNodeCount);
    minSpin->setValue(m_minNodeCount);
    minLayout->addWidget(minLabel);
    minLayout->addWidget(minSpin);

    // 最大节点数量设置
    QHBoxLayout *maxLayout = new QHBoxLayout();
    QLabel *maxLabel = new QLabel("最大节点数量:");
    QSpinBox *maxSpin = new QSpinBox();
    maxSpin->setRange(5, maxNodeCount);
    maxSpin->setValue(m_maxNodeCount);
    maxLayout->addWidget(maxLabel);
    maxLayout->addWidget(maxSpin);

    // 确保最大节点数不小于最小节点数
    connect(minSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [maxSpin](int minVal)
            {
        if (maxSpin->value() < minVal) {
            maxSpin->setValue(minVal);
        } });

    // 添加按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("确定");
    QPushButton *cancelBtn = new QPushButton("取消");
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    // 添加到主布局
    mainLayout->addLayout(minLayout);
    mainLayout->addLayout(maxLayout);
    mainLayout->addLayout(btnLayout);

    // 显示对话框并处理结果
    if (dialog.exec() == QDialog::Accepted)
    {
        m_minNodeCount = minSpin->value();
        m_maxNodeCount = maxSpin->value();

        // 在状态栏显示设置结果
        m_statusLabel->setText(QString("仿真设置已更新 - 节点数量范围: %1-%2")
                                   .arg(m_minNodeCount)
                                   .arg(m_maxNodeCount));
    }
}

QString MainWindow::defaultSaveDirectory() const
{
    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    // 构建net文件夹路径
    QString netDir = appDir + "/net";

    // 确保net文件夹存在，不存在则创建
    QDir dir;
    if (!dir.exists(netDir))
    {
        dir.mkpath(netDir); // 递归创建目录（支持多级目录）
    }

    return netDir;
}

QString MainWindow::defaultTrainDataDirectory() const
{
    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    // 构建train_data文件夹路径
    QString trainDir = appDir + "/train_data";

    // 确保train_data文件夹存在，不存在则创建
    QDir dir;
    if (!dir.exists(trainDir))
    {
        dir.mkpath(trainDir); // 递归创建目录
    }

    return trainDir;
}