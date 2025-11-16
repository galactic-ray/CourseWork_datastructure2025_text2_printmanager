#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>
#include "printmanager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddJob();
    void onCancelJob();
    void onSetSpeed();
    void onTick();
    void onRunToEnd();
    void onRandomJobs();
    void onAutoTick(); // 自动推进
    void updateDisplay();

private:
    void setupUI();
    void refreshWaitingTable();
    void refreshRunningInfo();
    void refreshDoneTable();
    void refreshStatistics();
    void refreshStatus();

    PrintManager pm;
    QTimer *autoTimer;  // 用于自动刷新显示
    QTimer *autoTickTimer;  // 用于自动推进时间

    // UI组件
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    // 状态显示区域
    QGroupBox *statusGroup;
    QLabel *timeLabel;
    QLabel *speedLabel;
    QLabel *printerStatusLabel;
    QLabel *currentJobLabel;
    
    // 控制区域
    QGroupBox *controlGroup;
    QDoubleSpinBox *speedSpinBox;
    QPushButton *setSpeedBtn;
    QPushButton *addJobBtn;
    QPushButton *cancelJobBtn;
    QPushButton *tickBtn;
    QPushButton *runToEndBtn;
    QPushButton *randomJobsBtn;
    QPushButton *autoTickBtn;
    QSpinBox *tickSecondsSpinBox;
    
    // 添加任务对话框组件（内嵌）
    QGroupBox *addJobGroup;
    QLineEdit *userEdit;
    QLineEdit *docEdit;
    QSpinBox *pagesSpinBox;
    
    // 取消任务
    QGroupBox *cancelJobGroup;
    QSpinBox *cancelIdSpinBox;
    
    // 等待队列表格
    QGroupBox *waitingGroup;
    QTableWidget *waitingTable;
    
    // 进行中任务显示
    QGroupBox *runningGroup;
    QTextEdit *runningText;
    
    // 已完成任务表格
    QGroupBox *doneGroup;
    QTableWidget *doneTable;
    
    // 统计信息
    QGroupBox *statsGroup;
    QTextEdit *statsText;
};

#endif // MAINWINDOW_H

