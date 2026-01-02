#pragma once
#include <QThread>
#include <QStringList>
#include <QVector>
#include <QVariant>
#include <QMutex>

// 定义数据结构，方便传递给 UI
struct ScanResult {
    QStringList columns; // 对应 Treeview 的一行
};

// --- 基类：定义通用信号 ---
class WorkerThread : public QThread {
    Q_OBJECT
public:
    explicit WorkerThread(QObject* parent = nullptr) : QThread(parent) {}
    
signals:
    // 进度信号：百分比 (0-100)，状态文本
    void progressUpdated(int percentage, QString statusText);
    
    // 结果信号：一行数据（对应 Python 的 treeview_row）
    void resultReady(QStringList rowData);
    
    // 设置表头信号（对应 Python 的 treeview_setup）
    void setupColumns(QStringList headers);
    
    // 清空结果信号
    void clearResults();
    
    // 完成信号（对应 Python 的 complete）
    void taskFinished(QString message);
};

// --- 1. Ping 任务 ---
class PingTask : public WorkerThread {
    Q_OBJECT
public:
    PingTask(const QStringList& hosts, int count, int timeoutMs, QObject* parent = nullptr);
    void run() override;

private:
    QStringList m_hosts;
    int m_count;
    int m_timeoutMs;
};

// --- 2. 批量端口扫描任务 ---
class PortScanTask : public WorkerThread {
    Q_OBJECT
public:
    PortScanTask(const QStringList& hosts, const QVector<int>& ports, QObject* parent = nullptr);
    void run() override;

private:
    QStringList m_hosts;
    QVector<int> m_ports;
};

// --- 3. 单个目标扫描任务 ---
class SingleScanTask : public WorkerThread {
    Q_OBJECT
public:
    SingleScanTask(const QString& host, const QVector<int>& ports, QObject* parent = nullptr);
    void run() override;

private:
    QString m_host;
    QVector<int> m_ports;
};

// --- 4. 提取 IP 任务 ---
class ExtractIpTask : public WorkerThread {
    Q_OBJECT
public:
    ExtractIpTask(const QString& content, QObject* parent = nullptr);
    void run() override;

private:
    QString m_content;
};
