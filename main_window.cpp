#include "main_window.h"
#include <QMenuBar>
#include <QMenu>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>

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
}

MainWindow::~MainWindow()
{
    // 资源由Qt父对象机制自动管理
}

void MainWindow::createMenus()
{
    QMenu *viewMenu = menuBar()->addMenu("视图");

    // 网格大小设置动作
    m_gridSizeAction = new QAction("网格大小", this);
    connect(m_gridSizeAction, &QAction::triggered, this, &MainWindow::onGridSizeChanged);
    viewMenu->addAction(m_gridSizeAction);
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
}

void MainWindow::updatePropertyView(ClientNode *node)
{
    clearPropertyView();
    m_propertyView->setHeaderLabel("节点属性");

     // 节点名称
    QTreeWidgetItem *nameItem = new QTreeWidgetItem();
    nameItem->setText(0, "节点名称");
    nameItem->setText(1, node->name());
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);  // 禁止编辑
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

    // 添加链路属性
    QTreeWidgetItem *bandwidthItem = new QTreeWidgetItem();
    bandwidthItem->setText(0, "传输带宽(Mbps)");
    bandwidthItem->setText(1, QString::number(link->bandwidth()));
    bandwidthItem->setFlags(bandwidthItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(bandwidthItem);

    QTreeWidgetItem *congestionItem = new QTreeWidgetItem();
    congestionItem->setText(0, "拥塞程度(0-1)");
    congestionItem->setText(1, QString::number(link->congestion()));
    congestionItem->setFlags(congestionItem->flags() | Qt::ItemIsEditable);
    m_propertyView->addTopLevelItem(congestionItem);

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
