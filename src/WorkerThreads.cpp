#include "WorkerThreads.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QRegularExpression>
#include <QHostInfo>
#include <QElapsedTimer>

// Windows ICMP API
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>

// --- Ping Task Implementation ---

PingTask::PingTask(const QStringList& hosts, int count, int timeoutMs, QObject* parent)
    : WorkerThread(parent), m_hosts(hosts), m_count(count), m_timeoutMs(timeoutMs) {}

void PingTask::run() {
    emit clearResults();
    emit setupColumns({"目标地址", "状态", "平均延迟(ms)", "丢包率(%)", "TTL"});

    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        emit taskFinished("错误: 无法打开 ICMP 句柄");
        return;
    }

    // 分配缓冲区: 请求数据 + 响应头 + 响应数据
    char sendData[] = "NetToolPingPayload";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    LPVOID replyBuffer = (VOID*)malloc(replySize);

    int total = m_hosts.size();
    for (int i = 0; i < total; ++i) {
        if (isInterruptionRequested()) break;

        QString hostStr = m_hosts[i];
        emit progressUpdated((i + 1) * 100 / total, QString("正在 Ping (%1/%2): %3...").arg(i + 1).arg(total).arg(hostStr));

        // 解析域名到 IP
        QHostInfo info = QHostInfo::fromName(hostStr);
        if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
            emit resultReady({hostStr, "无效地址", "N/A", "100", "N/A"});
            continue;
        }

        QHostAddress ipAddr = info.addresses().first();
        IPAddr destIp = htonl(ipAddr.toIPv4Address()); // Qt返回宿主序，API需要网络序

        int successCount = 0;
        long totalRtt = 0;
        int lastTtl = 0;
        bool anySuccess = false;

        for (int j = 0; j < m_count; ++j) {
            if (isInterruptionRequested()) break;
            
            DWORD dwRetVal = IcmpSendEcho(
                hIcmpFile,
                destIp,
                sendData,
                sizeof(sendData),
                NULL,
                replyBuffer,
                replySize,
                m_timeoutMs
            );

            if (dwRetVal != 0) {
                PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
                if (pEchoReply->Status == IP_SUCCESS) {
                    successCount++;
                    totalRtt += pEchoReply->RoundTripTime;
                    lastTtl = pEchoReply->Options.Ttl;
                    anySuccess = true;
                }
            }
            // 简单延时避免发包太快
            if(j < m_count - 1) QThread::msleep(100); 
        }

        if (anySuccess) {
            double avgRtt = (double)totalRtt / successCount;
            double loss = ((m_count - successCount) / (double)m_count) * 100.0;
            emit resultReady({
                hostStr, 
                "在线", 
                QString::number(avgRtt, 'f', 2), 
                QString::number(loss, 'f', 0), 
                QString::number(lastTtl)
            });
        } else {
            emit resultReady({hostStr, "超时", "N/A", "100", "N/A"});
        }
    }

    if(replyBuffer) free(replyBuffer);
    IcmpCloseHandle(hIcmpFile);
    emit taskFinished(isInterruptionRequested() ? "任务已被用户停止。" : "批量Ping测试完成。");
}

// --- Port Scan Task Implementation ---

PortScanTask::PortScanTask(const QStringList& hosts, const QVector<int>& ports, QObject* parent)
    : WorkerThread(parent), m_hosts(hosts), m_ports(ports) {}

void PortScanTask::run() {
    emit clearResults();
    emit setupColumns({"目标地址", "端口", "状态"});

    int totalChecks = m_hosts.size() * m_ports.size();
    int checksDone = 0;

    for (const QString& host : m_hosts) {
        if (isInterruptionRequested()) break;

        for (int port : m_ports) {
            if (isInterruptionRequested()) break;
            
            checksDone++;
            int progress = (totalChecks > 0) ? (checksDone * 100 / totalChecks) : 0;
            emit progressUpdated(progress, QString("批量扫描 (%1/%2): %3:%4").arg(checksDone).arg(totalChecks).arg(host).arg(port));

            QTcpSocket socket;
            // 尝试连接
            socket.connectToHost(host, port);
            
            QString status;
            if (socket.waitForConnected(2000)) { // 2秒超时
                status = "开放 (Open)";
                socket.disconnectFromHost();
            } else {
                // 判断是否是超时还是拒绝
                if (socket.error() == QAbstractSocket::ConnectionRefusedError)
                    status = "关闭 (Closed)";
                else if (socket.error() == QAbstractSocket::SocketTimeoutError)
                    status = "超时 (Timeout)";
                else
                    status = "拒绝/错误";
            }
            
            // 为了避免 UI 刷屏，只显示开放的端口，或者符合特定条件的
            // 这里遵循原 Python 逻辑：开放的才存入，或者显示错误
            if (status.contains("开放")) {
                emit resultReady({host, QString::number(port), status});
            }
        }
    }
    emit taskFinished(isInterruptionRequested() ? "任务已被用户停止。" : "批量端口扫描完成。");
}

// --- Single Target Scan Task Implementation ---

SingleScanTask::SingleScanTask(const QString& host, const QVector<int>& ports, QObject* parent)
    : WorkerThread(parent), m_host(host), m_ports(ports) {}

void SingleScanTask::run() {
    emit clearResults();
    emit setupColumns({"目标地址", "开放端口", "服务/备注"});

    int totalChecks = m_ports.size();
    int openFound = 0;

    for (int i = 0; i < totalChecks; ++i) {
        if (isInterruptionRequested()) break;
        int port = m_ports[i];

        emit progressUpdated((i + 1) * 100 / totalChecks, QString("正在扫描 %1:%2").arg(m_host).arg(port));

        QTcpSocket socket;
        socket.connectToHost(m_host, port);
        
        if (socket.waitForConnected(1000)) {
            // 尝试简单的服务识别 (Qt 没有 getservbyport，这里简单显示未知或通过已知端口映射)
            // 这是一个简化版本，原 Python 用 socket.getservbyport
            QString service = "未知"; 
            // 这里可以用一个简易的 switch case 模拟常见端口服务，或者省略
            
            emit resultReady({m_host, QString::number(port), service});
            openFound++;
            socket.disconnectFromHost();
        }
    }
    emit taskFinished(QString("扫描完成。共找到 %1 个开放端口。").arg(openFound));
}

// --- Extract IP Task Implementation ---

ExtractIpTask::ExtractIpTask(const QString& content, QObject* parent)
    : WorkerThread(parent), m_content(content) {}

void ExtractIpTask::run() {
    emit clearResults();
    emit setupColumns({"提取到的IP地址"});

    QRegularExpression ipRegex("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})");
    QRegularExpressionMatchIterator i = ipRegex.globalMatch(m_content);
    
    QStringList foundIps;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString ip = match.captured(1);
        
        // 验证 IP 有效性 (0-255)
        bool valid = true;
        QStringList parts = ip.split('.');
        for (const QString& part : parts) {
            int octet = part.toInt();
            if (octet < 0 || octet > 255) {
                valid = false; 
                break;
            }
        }
        if (valid) foundIps.append(ip);
    }

    // 去重并排序
    foundIps.removeDuplicates();
    foundIps.sort();

    int total = foundIps.size();
    if (total == 0) {
        emit taskFinished("未在文本中找到有效的IP地址。");
        return;
    }

    for (int j = 0; j < total; ++j) {
        if (isInterruptionRequested()) break;
        emit progressUpdated((j + 1) * 100 / total, "正在提取IP...");
        emit resultReady({foundIps[j]});
        QThread::msleep(10); // 稍微延时展示效果
    }

    emit taskFinished(QString("提取完成，共找到 %1 个唯一IP。").arg(total));
}
