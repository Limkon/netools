#pragma once

#include <QMainWindow>
#include <QTreeWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QRadioButton>
#include <QVector>
#include "WorkerThreads.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 拖拽事件支持
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    // 窗口关闭事件
    void closeEvent(QCloseEvent *event) override;

private slots:
    // UI 交互槽函数
    void onInputMethodChanged();
    void onBrowseFile();
    void onExportCsv();
    void onFilterTextChanged(const QString &text);
    
    // 任务控制槽函数
    void startPingTask();
    void startScanTask();
    void startExtractTask();
    void startSingleScanTask();
    void stopCurrentTask();
    void toggleSystemProxy();

    // 线程回调槽函数
    void onProgressUpdated(int percentage, QString statusText);
    void onResultReady(QStringList rowData);
    void onSetupColumns(QStringList headers);
    void onClearResults();
    void onTaskFinished(QString message);

    // 上下文菜单
    void showTreeContextMenu(const QPoint &pos);

private:
    void createWidgets();
    void setupLayout();
    void setControlsState(bool processing);
    
    // 辅助函数：解析输入
    QStringList getInputHosts(); 
    QString getInputContent();
    QVector<int> parsePortString(const QString& portStr);

    // UI 组件指针
    QWidget *centralWidget;
    
    // 输入区
    QRadioButton *radioFile;
    QRadioButton *radioText;
    QLineEdit *editFilePath;
    QPushButton *btnBrowse;
    QTextEdit *txtInput;
    QLineEdit *editTimeout;
    QLineEdit *editCount;
    QLineEdit *editPorts;

    // 单个扫描区
    QLineEdit *editSingleIp;
    QLineEdit *editSinglePorts;
    QPushButton *btnSingleScan;

    // 控制区
    QPushButton *btnPing;
    QPushButton *btnScan;
    QPushButton *btnExtract;
    QPushButton *btnProxy;
    QPushButton *btnStop;

    // 结果区
    QLineEdit *editFilter;
    QTreeWidget *treeWidget;
    
    // 底部状态
    QProgressBar *progressBar;
    QLabel *lblStatus;
    QPushButton *btnExport;

    // 数据与状态
    WorkerThread *currentTask = nullptr;
    QVector<QStringList> originalData; // 用于过滤的原始数据备份
    bool isProxySet = false;
};
