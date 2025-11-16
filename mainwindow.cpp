#include "mainwindow.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <random>
#include <ctime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    pm.saveAll();
    setupUI();
    updateDisplay();
    
    // 设置自动刷新定时器（每秒更新一次显示）
    autoTimer = new QTimer(this);
    connect(autoTimer, &QTimer::timeout, this, &MainWindow::updateDisplay);
    autoTimer->start(1000); // 每秒更新
    
    // 初始化自动推进定时器
    autoTickTimer = nullptr;
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("操作系统打印机管理器");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);
    
    // ========== 状态显示区域 ==========
    statusGroup = new QGroupBox("系统状态", this);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusGroup);
    
    timeLabel = new QLabel("当前时间: 00:00", this);
    timeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    speedLabel = new QLabel("打印速度: 2.000 秒/页", this);
    speedLabel->setStyleSheet("font-size: 14px;");
    printerStatusLabel = new QLabel("打印机状态: 空闲", this);
    printerStatusLabel->setStyleSheet("font-size: 14px; color: green;");
    currentJobLabel = new QLabel("", this);
    currentJobLabel->setStyleSheet("font-size: 12px;");
    
    statusLayout->addWidget(timeLabel);
    statusLayout->addWidget(speedLabel);
    statusLayout->addWidget(printerStatusLabel);
    statusLayout->addWidget(currentJobLabel);
    statusLayout->addStretch();
    
    mainLayout->addWidget(statusGroup);
    
    // ========== 控制区域 ==========
    controlGroup = new QGroupBox("控制面板", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    
    // 设置速度
    controlLayout->addWidget(new QLabel("打印速度(秒/页):", this));
    speedSpinBox = new QDoubleSpinBox(this);
    speedSpinBox->setRange(0.001, 100.0);
    speedSpinBox->setValue(2.0);
    speedSpinBox->setDecimals(3);
    speedSpinBox->setSingleStep(0.1);
    controlLayout->addWidget(speedSpinBox);
    
    setSpeedBtn = new QPushButton("设置速度", this);
    connect(setSpeedBtn, &QPushButton::clicked, this, &MainWindow::onSetSpeed);
    controlLayout->addWidget(setSpeedBtn);
    
    controlLayout->addSpacing(20);
    
    // 模拟推进
    controlLayout->addWidget(new QLabel("推进秒数:", this));
    tickSecondsSpinBox = new QSpinBox(this);
    tickSecondsSpinBox->setRange(1, 3600);
    tickSecondsSpinBox->setValue(1);
    controlLayout->addWidget(tickSecondsSpinBox);
    
    tickBtn = new QPushButton("推进时间", this);
    connect(tickBtn, &QPushButton::clicked, this, &MainWindow::onTick);
    controlLayout->addWidget(tickBtn);
    
    runToEndBtn = new QPushButton("运行至完成", this);
    connect(runToEndBtn, &QPushButton::clicked, this, &MainWindow::onRunToEnd);
    controlLayout->addWidget(runToEndBtn);
    
    autoTickBtn = new QPushButton("自动推进", this);
    autoTickBtn->setCheckable(true);
    connect(autoTickBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            autoTickBtn->setText("停止自动");
            // 每秒自动推进1秒
            if (!autoTickTimer) {
                autoTickTimer = new QTimer(this);
                connect(autoTickTimer, &QTimer::timeout, this, [this]() {
                    pm.tick(1);
                    updateDisplay();
                });
            }
            autoTickTimer->start(1000);
        } else {
            autoTickBtn->setText("自动推进");
            if (autoTickTimer) {
                autoTickTimer->stop();
            }
        }
    });
    controlLayout->addWidget(autoTickBtn);
    
    controlLayout->addStretch();
    
    mainLayout->addWidget(controlGroup);
    
    // ========== 添加任务区域 ==========
    addJobGroup = new QGroupBox("添加打印任务", this);
    QHBoxLayout *addJobLayout = new QHBoxLayout(addJobGroup);
    
    addJobLayout->addWidget(new QLabel("用户:", this));
    userEdit = new QLineEdit(this);
    userEdit->setPlaceholderText("输入用户名");
    addJobLayout->addWidget(userEdit);
    
    addJobLayout->addWidget(new QLabel("文档名:", this));
    docEdit = new QLineEdit(this);
    docEdit->setPlaceholderText("输入文档名");
    addJobLayout->addWidget(docEdit);
    
    addJobLayout->addWidget(new QLabel("页数:", this));
    pagesSpinBox = new QSpinBox(this);
    pagesSpinBox->setRange(1, 10000);
    pagesSpinBox->setValue(10);
    addJobLayout->addWidget(pagesSpinBox);
    
    addJobBtn = new QPushButton("添加任务", this);
    connect(addJobBtn, &QPushButton::clicked, this, &MainWindow::onAddJob);
    addJobLayout->addWidget(addJobBtn);
    
    randomJobsBtn = new QPushButton("随机生成任务", this);
    connect(randomJobsBtn, &QPushButton::clicked, this, &MainWindow::onRandomJobs);
    addJobLayout->addWidget(randomJobsBtn);
    
    mainLayout->addWidget(addJobGroup);
    
    // ========== 取消任务区域 ==========
    cancelJobGroup = new QGroupBox("取消任务", this);
    QHBoxLayout *cancelLayout = new QHBoxLayout(cancelJobGroup);
    
    cancelLayout->addWidget(new QLabel("任务ID:", this));
    cancelIdSpinBox = new QSpinBox(this);
    cancelIdSpinBox->setRange(1, 100000);
    cancelLayout->addWidget(cancelIdSpinBox);
    
    cancelJobBtn = new QPushButton("取消任务", this);
    connect(cancelJobBtn, &QPushButton::clicked, this, &MainWindow::onCancelJob);
    cancelLayout->addWidget(cancelJobBtn);
    
    cancelLayout->addStretch();
    
    mainLayout->addWidget(cancelJobGroup);
    
    // ========== 内容区域（左右分栏） ==========
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // 左侧：等待队列和进行中
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    // 等待队列表格
    waitingGroup = new QGroupBox("等待队列", this);
    QVBoxLayout *waitingLayout = new QVBoxLayout(waitingGroup);
    waitingTable = new QTableWidget(this);
    waitingTable->setColumnCount(5);
    QStringList waitingHeaders = {"ID", "用户", "文档名", "页数", "提交时间"};
    waitingTable->setHorizontalHeaderLabels(waitingHeaders);
    waitingTable->horizontalHeader()->setStretchLastSection(true);
    waitingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    waitingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    waitingLayout->addWidget(waitingTable);
    leftLayout->addWidget(waitingGroup);
    
    // 进行中任务
    runningGroup = new QGroupBox("正在打印", this);
    QVBoxLayout *runningLayout = new QVBoxLayout(runningGroup);
    runningText = new QTextEdit(this);
    runningText->setReadOnly(true);
    runningText->setMaximumHeight(120);
    runningLayout->addWidget(runningText);
    leftLayout->addWidget(runningGroup);
    
    contentLayout->addLayout(leftLayout, 1);
    
    // 右侧：已完成和统计
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // 已完成任务表格
    doneGroup = new QGroupBox("已完成任务", this);
    QVBoxLayout *doneLayout = new QVBoxLayout(doneGroup);
    doneTable = new QTableWidget(this);
    doneTable->setColumnCount(8);
    QStringList doneHeaders = {"ID", "用户", "文档名", "页数", "提交时间", "开始时间", "结束时间", "等待/耗时(秒)"};
    doneTable->setHorizontalHeaderLabels(doneHeaders);
    doneTable->horizontalHeader()->setStretchLastSection(true);
    doneTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    doneTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    doneLayout->addWidget(doneTable);
    rightLayout->addWidget(doneGroup);
    
    // 统计信息
    statsGroup = new QGroupBox("统计信息", this);
    QVBoxLayout *statsLayout = new QVBoxLayout(statsGroup);
    statsText = new QTextEdit(this);
    statsText->setReadOnly(true);
    statsText->setMaximumHeight(100);
    statsLayout->addWidget(statsText);
    rightLayout->addWidget(statsGroup);
    
    contentLayout->addLayout(rightLayout, 1);
    
    mainLayout->addLayout(contentLayout, 1);
}

void MainWindow::onAddJob()
{
    QString user = userEdit->text().trimmed();
    QString doc = docEdit->text().trimmed();
    int pages = pagesSpinBox->value();
    
    if (user.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入用户名！");
        return;
    }
    if (doc.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入文档名！");
        return;
    }
    
    int id = pm.addJob(user.toStdString(), doc.toStdString(), pages);
    QMessageBox::information(this, "成功", 
        QString("任务已添加！\nID: %1\n当前时间: %2")
        .arg(id).arg(QString::fromStdString(PrintManager::fmt(pm.currentTime))));
    
    userEdit->clear();
    docEdit->clear();
    pagesSpinBox->setValue(10);
    updateDisplay();
}

void MainWindow::onCancelJob()
{
    int id = cancelIdSpinBox->value();
    bool ok = pm.cancelJob(id);
    if (ok) {
        QMessageBox::information(this, "成功", QString("任务 #%1 已取消").arg(id));
    } else {
        QMessageBox::warning(this, "失败", QString("未找到任务 #%1（可能不在等待队列中）").arg(id));
    }
    updateDisplay();
}

void MainWindow::onSetSpeed()
{
    double speed = speedSpinBox->value();
    pm.setSpeed(speed);
    QMessageBox::information(this, "成功", 
        QString("打印速度已设置为 %1 秒/页").arg(speed, 0, 'f', 3));
    updateDisplay();
}

void MainWindow::onTick()
{
    int dt = tickSecondsSpinBox->value();
    pm.tick(dt);
    updateDisplay();
}

void MainWindow::onRunToEnd()
{
    pm.runToEnd();
    QMessageBox::information(this, "完成", "所有任务已完成！");
    updateDisplay();
}

void MainWindow::onRandomJobs()
{
    bool ok;
    int count = QInputDialog::getInt(this, "随机生成任务", "任务数量:", 5, 1, 100, 1, &ok);
    if (!ok) return;
    
    int minPages = QInputDialog::getInt(this, "随机生成任务", "最小页数:", 1, 1, 1000, 1, &ok);
    if (!ok) return;
    
    int maxPages = QInputDialog::getInt(this, "随机生成任务", "最大页数:", 10, minPages, 1000, 1, &ok);
    if (!ok) return;
    
    std::mt19937 rng((unsigned)time(nullptr));
    std::uniform_int_distribution<int> pagesDist(minPages, maxPages);
    
    for (int i = 1; i <= count; ++i) {
        std::string user = "User" + std::to_string(i);
        std::string doc = "Doc" + std::to_string(i);
        int p = pagesDist(rng);
        pm.addJob(user, doc, p);
    }
    
    QMessageBox::information(this, "成功", QString("已生成 %1 个随机任务").arg(count));
    updateDisplay();
}

void MainWindow::onAutoTick()
{
    // 已在setupUI中实现
}

void MainWindow::updateDisplay()
{
    refreshStatus();
    refreshWaitingTable();
    refreshRunningInfo();
    refreshDoneTable();
    refreshStatistics();
}

void MainWindow::refreshStatus()
{
    timeLabel->setText(QString("当前时间: %1").arg(QString::fromStdString(PrintManager::fmt(pm.currentTime))));
    
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(3) << pm.secPerPage;
    speedLabel->setText(QString("打印速度: %1 秒/页").arg(QString::fromStdString(ss.str())));
    
    if (pm.busy) {
        printerStatusLabel->setText("打印机状态: 打印中");
        printerStatusLabel->setStyleSheet("font-size: 14px; color: red;");
        currentJobLabel->setText(QString("任务 #%1 (%2/%3) - 剩余约 %4 秒")
            .arg(pm.current.id)
            .arg(QString::fromStdString(pm.current.user))
            .arg(QString::fromStdString(pm.current.doc))
            .arg(pm.remainSec));
    } else {
        printerStatusLabel->setText("打印机状态: 空闲");
        printerStatusLabel->setStyleSheet("font-size: 14px; color: green;");
        currentJobLabel->setText("");
    }
}

void MainWindow::refreshWaitingTable()
{
    auto jobs = pm.getWaitingJobs();
    waitingTable->setRowCount(jobs.size());
    
    for (size_t i = 0; i < jobs.size(); ++i) {
        const auto& j = jobs[i];
        waitingTable->setItem(i, 0, new QTableWidgetItem(QString::number(j.id)));
        waitingTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(j.user)));
        waitingTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(j.doc)));
        waitingTable->setItem(i, 3, new QTableWidgetItem(QString::number(j.pages)));
        waitingTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(PrintManager::fmt(j.submitTime))));
    }
}

void MainWindow::refreshRunningInfo()
{
    if (pm.busy) {
        runningText->setText(QString(
            "任务ID: %1\n"
            "用户: %2\n"
            "文档: %3\n"
            "页数: %4\n"
            "开始时间: %5\n"
            "剩余时间: 约 %6 秒"
        ).arg(pm.current.id)
        .arg(QString::fromStdString(pm.current.user))
        .arg(QString::fromStdString(pm.current.doc))
        .arg(pm.current.pages)
        .arg(QString::fromStdString(PrintManager::fmt(pm.current.startTime)))
        .arg(pm.remainSec));
    } else {
        runningText->setText("当前无正在打印的任务");
    }
}

void MainWindow::refreshDoneTable()
{
    doneTable->setRowCount(pm.done.size());
    
    for (size_t i = 0; i < pm.done.size(); ++i) {
        const auto& j = pm.done[i];
        doneTable->setItem(i, 0, new QTableWidgetItem(QString::number(j.id)));
        doneTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(j.user)));
        doneTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(j.doc)));
        doneTable->setItem(i, 3, new QTableWidgetItem(QString::number(j.pages)));
        doneTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(PrintManager::fmt(j.submitTime))));
        doneTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(PrintManager::fmt(j.startTime))));
        doneTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(PrintManager::fmt(j.finishTime))));
        doneTable->setItem(i, 7, new QTableWidgetItem(
            QString("%1 / %2").arg(j.waitTime()).arg(j.duration())));
    }
}

void MainWindow::refreshStatistics()
{
    auto stats = pm.getStatistics();
    statsText->setText(QString(
        "完成任务数: %1\n"
        "平均等待时间: %2 秒\n"
        "平均打印耗时: %3 秒"
    ).arg(stats.totalCompleted)
    .arg(stats.avgWaitTime, 0, 'f', 2)
    .arg(stats.avgDuration, 0, 'f', 2));
}

