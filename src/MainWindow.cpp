#include "MainWindow.h"
#include "ProxyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QMenu>
#include <QTextStream>
#include <QCloseEvent>
// --- 修复点：添加以下两个头文件 ---
#include <QApplication>
#include <QClipboard>
// -------------------------------

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("多功能网络工具 (v3.7 C++重构版)");
    resize(900, 720);
    setAcceptDrops(true); // 启用主窗口拖拽

    createWidgets();
    setupLayout();

    // 初始化状态
    onInputMethodChanged();
    lblStatus->setText("请选择输入方式并开始操作...");
}

MainWindow::~MainWindow() {
    if (currentTask && currentTask->isRunning()) {
        currentTask->requestInterruption();
        currentTask->quit();
        currentTask->wait();
    }
}

void MainWindow::createWidgets() {
    // --- 输入区 ---
    radioFile = new QRadioButton("从文件");
    radioText = new QRadioButton("从文本框粘贴");
    radioFile->setChecked(true);
    
    connect(radioFile, &QRadioButton::toggled, this, &MainWindow::onInputMethodChanged);

    editFilePath = new QLineEdit();
    editFilePath->setPlaceholderText("拖拽文件到此处或点击浏览");
    editFilePath->setReadOnly(true); // 文件路径只读，通过浏览或拖拽设置

    btnBrowse = new QPushButton("浏览...");
    connect(btnBrowse, &QPushButton::clicked, this, &MainWindow::onBrowseFile);

    txtInput = new QTextEdit();
    txtInput->setPlaceholderText("在此处粘贴IP或文本内容...");

    editTimeout = new QLineEdit("1000");
    editCount = new QLineEdit("5");
    editPorts = new QLineEdit("80,443,8080,1433,3306,3389");

    // --- 单个扫描区 ---
    editSingleIp = new QLineEdit("127.0.0.1");
    editSinglePorts = new QLineEdit("1-1024, 3306, 8080-8090");
    btnSingleScan = new QPushButton("扫描指定目标");
    connect(btnSingleScan, &QPushButton::clicked, this, &MainWindow::startSingleScanTask);

    // --- 控制区 ---
    btnPing = new QPushButton("开始批量 Ping");
    btnScan = new QPushButton("开始批量端口扫描");
    btnExtract = new QPushButton("从输入源提取IP");
    btnProxy = new QPushButton("设置系统代理");
    btnStop = new QPushButton("停止当前任务");
    btnStop->setEnabled(false);

    connect(btnPing, &QPushButton::clicked, this, &MainWindow::startPingTask);
    connect(btnScan, &QPushButton::clicked, this, &MainWindow::startScanTask);
    connect(btnExtract, &QPushButton::clicked, this, &MainWindow::startExtractTask);
    connect(btnProxy, &QPushButton::clicked, this, &MainWindow::toggleSystemProxy);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::stopCurrentTask);

    // --- 结果区 ---
    editFilter = new QLineEdit();
    editFilter->setPlaceholderText("输入关键词过滤结果...");
    connect(editFilter, &QLineEdit::textChanged, this, &MainWindow::onFilterTextChanged);

    treeWidget = new QTreeWidget();
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);

    // --- 底部 ---
    progressBar = new QProgressBar();
    progressBar->setTextVisible(true);
    progressBar->setAlignment(Qt::AlignCenter);
    
    lblStatus = new QLabel("就绪");
    btnExport = new QPushButton("将结果导出为CSV");
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExportCsv);
}

void MainWindow::setupLayout() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 1. 批量任务输入组
    QGroupBox *grpInput = new QGroupBox("批量任务: 输入源与选项");
    QGridLayout *inputLayout = new QGridLayout(grpInput);
    
    inputLayout->addWidget(radioFile, 0, 0);
    inputLayout->addWidget(radioText, 0, 2);
    
    inputLayout->addWidget(editFilePath, 1, 0, 1, 2);
    inputLayout->addWidget(btnBrowse, 1, 2);
    
    inputLayout->addWidget(new QLabel("Ping超时(ms):"), 2, 0);
    inputLayout->addWidget(editTimeout, 2, 1);
    
    inputLayout->addWidget(new QLabel("Ping次数:"), 3, 0);
    inputLayout->addWidget(editCount, 3, 1);
    
    inputLayout->addWidget(new QLabel("批量扫描端口:"), 4, 0);
    inputLayout->addWidget(editPorts, 4, 1, 1, 2);
    
    inputLayout->addWidget(txtInput, 1, 3, 4, 1); // 文本框跨行放在右侧
    inputLayout->setColumnStretch(3, 1); // 让文本框拉伸
    
    mainLayout->addWidget(grpInput);

    // 2. 单个扫描组
    QGroupBox *grpSingle = new QGroupBox("单个目标端口扫描");
    QHBoxLayout *singleLayout = new QHBoxLayout(grpSingle);
    singleLayout->addWidget(new QLabel("目标IP:"));
    singleLayout->addWidget(editSingleIp);
    singleLayout->addWidget(new QLabel("端口范围:"));
    singleLayout->addWidget(editSinglePorts);
    singleLayout->addWidget(btnSingleScan);
    mainLayout->addWidget(grpSingle);

    // 3. 控制按钮
    QGroupBox *grpControl = new QGroupBox("任务控制");
    QHBoxLayout *controlLayout = new QHBoxLayout(grpControl);
    controlLayout->addWidget(btnPing);
    controlLayout->addWidget(btnScan);
    controlLayout->addWidget(btnExtract);
    controlLayout->addWidget(btnProxy);
    controlLayout->addStretch();
    controlLayout->addWidget(btnStop);
    mainLayout->addWidget(grpControl);

    // 4. 结果显示
    QGroupBox *grpResult = new QGroupBox("结果显示");
    QVBoxLayout *resultLayout = new QVBoxLayout(grpResult);
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("过滤结果:"));
    filterLayout->addWidget(editFilter);
    resultLayout->addLayout(filterLayout);
    resultLayout->addWidget(treeWidget);
    mainLayout->addWidget(grpResult, 1); // 这里的 1 表示占用多余空间

    // 5. 底部状态
    mainLayout->addWidget(progressBar);
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(lblStatus, 1);
    bottomLayout->addWidget(btnExport);
    mainLayout->addLayout(bottomLayout);
}

// --- 事件处理 ---

void MainWindow::onInputMethodChanged() {
    bool isFile = radioFile->isChecked();
    editFilePath->setEnabled(isFile);
    btnBrowse->setEnabled(isFile);
    txtInput->setEnabled(!isFile);
    
    if (isFile) lblStatus->setText("请拖拽文件或点击浏览...");
    else lblStatus->setText("请在文本框粘贴内容...");
}

void MainWindow::onBrowseFile() {
    QString path = QFileDialog::getOpenFileName(this, "选择文件", "", "Text Files (*.txt);;All Files (*.*)");
    if (!path.isEmpty()) {
        editFilePath->setText(path);
        radioFile->setChecked(true);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    
    QString path = urls.first().toLocalFile();
    if (!path.isEmpty()) {
        editFilePath->setText(path);
        radioFile->setChecked(true);
        onInputMethodChanged();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (isProxySet) {
        auto reply = QMessageBox::question(this, "退出确认", 
            "检测到系统代理已设置，是否在退出前自动取消代理？", 
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ProxyManager::unsetSystemProxy();
        }
    }
    event->accept();
}

// --- 任务逻辑 ---

void MainWindow::setControlsState(bool processing) {
    btnPing->setEnabled(!processing);
    btnScan->setEnabled(!processing);
    btnExtract->setEnabled(!processing);
    btnSingleScan->setEnabled(!processing);
    btnProxy->setEnabled(!processing);
    btnBrowse->setEnabled(!processing);
    btnStop->setEnabled(processing);
}

QString MainWindow::getInputContent() {
    if (radioText->isChecked()) {
        return txtInput->toPlainText();
    } else {
        QString path = editFilePath->text();
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        } else {
            QMessageBox::warning(this, "错误", "无法读取文件，请检查路径。");
            return "";
        }
    }
}

QStringList MainWindow::getInputHosts() {
    QString content = getInputContent();
    if (content.isEmpty()) return {};

    // 简单的正则匹配提取域名和IP (简化版)
    QRegularExpression re("[a-zA-Z0-9\\-\\.]+\\.[a-zA-Z]{2,}|\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
    QRegularExpressionMatchIterator i = re.globalMatch(content);
    
    QStringList hosts;
    while (i.hasNext()) {
        hosts << i.next().captured(0);
    }
    hosts.removeDuplicates();
    hosts.sort();
    return hosts;
}

QVector<int> MainWindow::parsePortString(const QString& portStr) {
    QVector<int> ports;
    QStringList parts = portStr.split(',');
    for (const QString& part : parts) {
        QString p = part.trimmed();
        if (p.isEmpty()) continue;
        if (p.contains('-')) {
            QStringList range = p.split('-');
            if (range.size() == 2) {
                int start = range[0].toInt();
                int end = range[1].toInt();
                for (int i = start; i <= end; ++i) ports.append(i);
            }
        } else {
            ports.append(p.toInt());
        }
    }
    // 简单验证
    if (ports.isEmpty()) QMessageBox::warning(this, "错误", "端口格式错误");
    return ports;
}

void MainWindow::startPingTask() {
    QStringList hosts = getInputHosts();
    if (hosts.isEmpty()) return;
    
    setControlsState(true);
    int count = editCount->text().toInt();
    int timeout = editTimeout->text().toInt();
    
    currentTask = new PingTask(hosts, count, timeout, this);
    // 连接通用信号
    connect(currentTask, &WorkerThread::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(currentTask, &WorkerThread::resultReady, this, &MainWindow::onResultReady);
    connect(currentTask, &WorkerThread::setupColumns, this, &MainWindow::onSetupColumns);
    connect(currentTask, &WorkerThread::clearResults, this, &MainWindow::onClearResults);
    connect(currentTask, &WorkerThread::taskFinished, this, &MainWindow::onTaskFinished);
    
    currentTask->start();
}

void MainWindow::startScanTask() {
    QStringList hosts = getInputHosts();
    if (hosts.isEmpty()) return;
    
    QVector<int> ports = parsePortString(editPorts->text());
    if (ports.isEmpty()) return;

    setControlsState(true);
    currentTask = new PortScanTask(hosts, ports, this);
    connect(currentTask, &WorkerThread::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(currentTask, &WorkerThread::resultReady, this, &MainWindow::onResultReady);
    connect(currentTask, &WorkerThread::setupColumns, this, &MainWindow::onSetupColumns);
    connect(currentTask, &WorkerThread::clearResults, this, &MainWindow::onClearResults);
    connect(currentTask, &WorkerThread::taskFinished, this, &MainWindow::onTaskFinished);
    currentTask->start();
}

void MainWindow::startExtractTask() {
    QString content = getInputContent();
    if (content.isEmpty()) return;

    setControlsState(true);
    currentTask = new ExtractIpTask(content, this);
    connect(currentTask, &WorkerThread::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(currentTask, &WorkerThread::resultReady, this, &MainWindow::onResultReady);
    connect(currentTask, &WorkerThread::setupColumns, this, &MainWindow::onSetupColumns);
    connect(currentTask, &WorkerThread::clearResults, this, &MainWindow::onClearResults);
    connect(currentTask, &WorkerThread::taskFinished, this, &MainWindow::onTaskFinished);
    currentTask->start();
}

void MainWindow::startSingleScanTask() {
    QString host = editSingleIp->text().trimmed();
    if (host.isEmpty()) return;
    QVector<int> ports = parsePortString(editSinglePorts->text());
    if (ports.isEmpty()) return;

    setControlsState(true);
    currentTask = new SingleScanTask(host, ports, this);
    connect(currentTask, &WorkerThread::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(currentTask, &WorkerThread::resultReady, this, &MainWindow::onResultReady);
    connect(currentTask, &WorkerThread::setupColumns, this, &MainWindow::onSetupColumns);
    connect(currentTask, &WorkerThread::clearResults, this, &MainWindow::onClearResults);
    connect(currentTask, &WorkerThread::taskFinished, this, &MainWindow::onTaskFinished);
    currentTask->start();
}

void MainWindow::stopCurrentTask() {
    if (currentTask && currentTask->isRunning()) {
        lblStatus->setText("正在停止任务...");
        currentTask->requestInterruption();
        // 禁用停止按钮防止重复点击
        btnStop->setEnabled(false);
    }
}

// --- 线程回调 ---

void MainWindow::onProgressUpdated(int percentage, QString statusText) {
    progressBar->setValue(percentage);
    lblStatus->setText(statusText);
}

void MainWindow::onSetupColumns(QStringList headers) {
    treeWidget->clear();
    treeWidget->setHeaderLabels(headers);
    // 自动调整列宽
    treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    originalData.clear();
}

void MainWindow::onResultReady(QStringList rowData) {
    QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
    for (int i = 0; i < rowData.size(); ++i) {
        item->setText(i, rowData[i]);
    }
    originalData.append(rowData); // 保存副本用于过滤
}

void MainWindow::onClearResults() {
    treeWidget->clear();
    originalData.clear();
}

void MainWindow::onTaskFinished(QString message) {
    setControlsState(false);
    lblStatus->setText(message);
    progressBar->setValue(100);
    // 确保线程对象被安全删除
    if (currentTask) {
        currentTask->deleteLater();
        currentTask = nullptr;
    }
}

// --- 其他功能 ---

void MainWindow::onFilterTextChanged(const QString &text) {
    treeWidget->clear();
    if (text.isEmpty()) {
        // 恢复所有
        for (const auto &row : originalData) {
            QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
            for (int i = 0; i < row.size(); ++i) item->setText(i, row[i]);
        }
    } else {
        // 过滤
        for (const auto &row : originalData) {
            bool match = false;
            for (const auto &col : row) {
                if (col.contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
            if (match) {
                QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
                for (int i = 0; i < row.size(); ++i) item->setText(i, row[i]);
            }
        }
    }
}

void MainWindow::onExportCsv() {
    if (treeWidget->topLevelItemCount() == 0) {
        QMessageBox::information(this, "提示", "没有数据可导出");
        return;
    }
    
    QString path = QFileDialog::getSaveFileName(this, "保存CSV", "", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        // 写表头
        QStringList headers;
        for(int i=0; i<treeWidget->columnCount(); ++i) 
            headers << treeWidget->headerItem()->text(i);
        out << headers.join(",") << "\n";

        // 写数据
        QTreeWidgetItemIterator it(treeWidget);
        while (*it) {
            QStringList row;
            for(int i=0; i<treeWidget->columnCount(); ++i)
                row << (*it)->text(i);
            out << row.join(",") << "\n";
            ++it;
        }
        file.close();
        QMessageBox::information(this, "成功", "文件导出成功！");
    }
}

void MainWindow::toggleSystemProxy() {
    if (isProxySet) {
        if (ProxyManager::unsetSystemProxy()) {
            isProxySet = false;
            btnProxy->setText("设置系统代理");
            lblStatus->setText("系统代理已恢复。");
            QMessageBox::information(this, "成功", "系统代理已取消。");
        }
    } else {
        // 从 TreeWidget 获取选中行
        QList<QTreeWidgetItem*> selected = treeWidget->selectedItems();
        if (selected.size() != 1) {
            QMessageBox::warning(this, "选择错误", "请先选中一行包含IP和端口的结果。");
            return;
        }
        
        // 简单的逻辑：假设第0列是IP，第1列是端口 (需要对应 PortScan 的结果)
        // 为了稳健性，可以检查表头名称，这里简化处理
        QString ip = selected[0]->text(0);
        bool ok;
        int port = selected[0]->text(1).toInt(&ok);
        
        if (!ok || port <= 0 || port > 65535) {
            QMessageBox::warning(this, "错误", "选中的行端口数据无效。");
            return;
        }
        
        if (ProxyManager::setSystemProxy(ip, port)) {
            isProxySet = true;
            btnProxy->setText("取消系统代理");
            lblStatus->setText(QString("系统代理已设置: %1:%2").arg(ip).arg(port));
            QMessageBox::information(this, "成功", "系统代理设置成功！");
        } else {
            QMessageBox::critical(this, "失败", "设置代理失败，请检查注册表权限。");
        }
    }
}

void MainWindow::showTreeContextMenu(const QPoint &pos) {
    QMenu menu(this);
    menu.addAction("复制选中行", [this](){
        // 简单的复制逻辑
        QList<QTreeWidgetItem*> selected = treeWidget->selectedItems();
        if(selected.isEmpty()) return;
        QStringList texts;
        for(auto item : selected) {
            QStringList row;
            for(int i=0; i<treeWidget->columnCount(); ++i) row << item->text(i);
            texts << row.join("\t");
        }
        QApplication::clipboard()->setText(texts.join("\n"));
    });
    // 可以继续添加删除行等功能
    menu.exec(treeWidget->mapToGlobal(pos));
}
