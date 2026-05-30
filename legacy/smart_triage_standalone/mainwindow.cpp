#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

// 通义千问 API Key（已填入）
const QString API_KEY = "YOUR_API_KEY_HERE";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置窗口
    resize(900, 700);
    setWindowTitle("智能医疗助手 - 关怀模式");

    // 创建中心控件
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("🏥 智能医疗导诊助手");
    titleLabel->setFont(QFont("", 24, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 症状输入区域
    QLabel *symptomLabel = new QLabel("📝 请描述您的症状：");
    symptomLabel->setFont(QFont("", 16));
    mainLayout->addWidget(symptomLabel);

    symptomEdit = new QTextEdit();
    symptomEdit->setFont(QFont("", 14));
    symptomEdit->setPlaceholderText("例如：头疼、发烧38度、咳嗽三天...");
    symptomEdit->setMinimumHeight(150);
    mainLayout->addWidget(symptomEdit);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);

    QPushButton *analyzeBtn = new QPushButton("🔍 智能分诊");
    analyzeBtn->setFont(QFont("", 16));
    analyzeBtn->setMinimumHeight(50);

    QPushButton *registerBtn = new QPushButton("🏥 一键挂号");
    registerBtn->setFont(QFont("", 16));
    registerBtn->setMinimumHeight(50);

    buttonLayout->addWidget(analyzeBtn);
    buttonLayout->addWidget(registerBtn);
    mainLayout->addLayout(buttonLayout);

    // 结果显示区域
    QLabel *resultLabel = new QLabel("📋 分诊结果：");
    resultLabel->setFont(QFont("", 16));
    mainLayout->addWidget(resultLabel);

    resultBrowser = new QTextBrowser();
    resultBrowser->setFont(QFont("", 14));
    resultBrowser->setMinimumHeight(200);
    mainLayout->addWidget(resultBrowser);

    // 连接信号槽
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onReplyFinished);
}

MainWindow::~MainWindow() {}

void MainWindow::onAnalyzeClicked()
{
    QString symptoms = symptomEdit->toPlainText().trimmed();
    if (symptoms.isEmpty()) {
        resultBrowser->append("<font color='red'>⚠️ 请输入症状描述！</font>");
        return;
    }

    resultBrowser->append("<font color='blue'>🤖 正在分析中...</font>");

    // 通义千问 API 地址（兼容 OpenAI 格式）
    QUrl url("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + API_KEY).toUtf8());

    QJsonObject message;
    message["role"] = "user";
    message["content"] = QString("根据以下症状给出可能的科室分诊建议，以简短分诊结果形式返回：%1").arg(symptoms);
    
    QJsonArray messages;
    messages.append(message);
    
    QJsonObject jsonBody;
    jsonBody["model"] = "qwen-turbo";
    jsonBody["messages"] = messages;
    jsonBody["max_tokens"] = 200;

    networkManager->post(request, QJsonDocument(jsonBody).toJson());
}

void MainWindow::onRegisterClicked()
{
    if (currentTriageResult.isEmpty()) {
        resultBrowser->append("<font color='red'>⚠️ 请先进行智能分诊！</font>");
        return;
    }

    resultBrowser->append("<font color='green'>🏥 正在为您挂号...</font>");
    // TODO: 对接你的模拟挂号系统
    resultBrowser->append("<font color='green'>✅ 挂号成功！请前往科室等待叫号。</font>");
}

void MainWindow::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        resultBrowser->append(QString("<font color='red'>❌ 网络错误: %1</font>").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull()) {
        resultBrowser->append("<font color='red'>❌ 解析响应失败</font>");
        reply->deleteLater();
        return;
    }

    // 解析通义千问返回（OpenAI 兼容格式）
    QJsonObject obj = doc.object();
    
    // 检查是否有错误
    if (obj.contains("error")) {
        QJsonObject errorObj = obj["error"].toObject();
        QString errorMsg = errorObj["message"].toString();
        resultBrowser->append(QString("<font color='red'>❌ API错误: %1</font>").arg(errorMsg));
        reply->deleteLater();
        return;
    }
    
    QJsonArray choices = obj["choices"].toArray();
    if (!choices.isEmpty()) {
        QJsonObject firstChoice = choices[0].toObject();
        QJsonObject message = firstChoice["message"].toObject();
        currentTriageResult = message["content"].toString();
        
        resultBrowser->append("<b>📋 分诊结果：</b>");
        resultBrowser->append(currentTriageResult);
    } else {
        resultBrowser->append("<font color='red'>❌ 未获取到有效结果</font>");
    }

    reply->deleteLater();
}
