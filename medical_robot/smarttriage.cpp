#include "smarttriage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

const QString HOSPITAL_API = "http://127.0.0.1:5000/api/register";

SmartTriage::SmartTriage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel("🤖 AI 智能导诊助手");
    titleLabel->setFont(QFont("", 20, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 症状输入区域
    QLabel *symptomLabel = new QLabel("📝 请描述您的症状：");
    symptomLabel->setFont(QFont("", 14));
    mainLayout->addWidget(symptomLabel);

    symptomEdit = new QTextEdit();
    symptomEdit->setFont(QFont("", 14));
    symptomEdit->setPlaceholderText("例如：头疼、发烧38度、咳嗽三天...");
    symptomEdit->setMinimumHeight(120);
    mainLayout->addWidget(symptomEdit);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);

    QPushButton *analyzeBtn = new QPushButton("🔍 智能分诊");
    analyzeBtn->setFont(QFont("", 14));
    analyzeBtn->setMinimumHeight(40);

    QPushButton *registerBtn = new QPushButton("🏥 一键挂号");
    registerBtn->setFont(QFont("", 14));
    registerBtn->setMinimumHeight(40);

    buttonLayout->addWidget(analyzeBtn);
    buttonLayout->addWidget(registerBtn);
    mainLayout->addLayout(buttonLayout);

    // 结果显示区域
    QLabel *resultLabel = new QLabel("📋 分诊结果：");
    resultLabel->setFont(QFont("", 14));
    mainLayout->addWidget(resultLabel);

    resultBrowser = new QTextBrowser();
    resultBrowser->setFont(QFont("", 13));
    resultBrowser->setMinimumHeight(200);
    mainLayout->addWidget(resultBrowser);

    // 连接信号槽
    connect(analyzeBtn, &QPushButton::clicked, this, &SmartTriage::onAnalyzeClicked);
    connect(registerBtn, &QPushButton::clicked, this, &SmartTriage::onRegisterClicked);

    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &SmartTriage::onReplyFinished);

    registerManager = new QNetworkAccessManager(this);
    connect(registerManager, &QNetworkAccessManager::finished, this, &SmartTriage::onRegisterReplyFinished);
}

SmartTriage::~SmartTriage() {}

void SmartTriage::onAnalyzeClicked()
{
    QString symptoms = symptomEdit->toPlainText().trimmed();
    if (symptoms.isEmpty()) {
        resultBrowser->append("<font color='red'>⚠️ 请输入症状描述！</font>");
        return;
    }

    resultBrowser->append("<font color='blue'>🤖 正在分析中...</font>");

    // 调用本地 Flask 后端（安全，不暴露 API Key）
    QUrl url("http://127.0.0.1:5000/api/triage");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonBody;
    jsonBody["symptoms"] = symptoms;

    networkManager->post(request, QJsonDocument(jsonBody).toJson());
}

void SmartTriage::onRegisterClicked()
{
    if (currentTriageResult.isEmpty()) {
        resultBrowser->append("<font color='red'>⚠️ 请先进行智能分诊！</font>");
        return;
    }

    resultBrowser->append("<font color='blue'>🏥 正在为您挂号...</font>");

    // 从分诊结果中提取科室名
    QString department = "内科";
    if (currentTriageResult.contains("内科")) department = "内科";
    else if (currentTriageResult.contains("外科")) department = "外科";
    else if (currentTriageResult.contains("神经内科")) department = "神经内科";
    else if (currentTriageResult.contains("骨科")) department = "骨科";
    else if (currentTriageResult.contains("儿科")) department = "儿科";
    else if (currentTriageResult.contains("皮肤科")) department = "皮肤科";

    QUrl url(HOSPITAL_API);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonBody;
    jsonBody["department"] = department;
    jsonBody["patient_name"] = "测试患者";
    jsonBody["symptoms"] = symptomEdit->toPlainText();

    registerManager->post(request, QJsonDocument(jsonBody).toJson());
}

// 处理 AI 分诊响应（来自 Flask 后端）
void SmartTriage::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        resultBrowser->append(QString("<font color='red'>❌ 网络错误: %1</font>").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj["success"].toBool()) {
        currentTriageResult = obj["result"].toString();
        resultBrowser->append("<b>📋 分诊结果：</b>");
        resultBrowser->append(currentTriageResult);
    } else {
        resultBrowser->append("<font color='red'>❌ " + obj["message"].toString() + "</font>");
    }
    reply->deleteLater();
}

// 处理挂号响应
void SmartTriage::onRegisterReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        resultBrowser->append(QString("<font color='red'>❌ 挂号失败: %1</font>").arg(reply->errorString()));
        resultBrowser->append("<font color='red'>💡 请确保后端服务已启动: python app.py</font>");
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        resultBrowser->append("<font color='red'>❌ 解析挂号响应失败</font>");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    if (obj["success"].toBool()) {
        QString ticketId = obj["ticket_id"].toString();
        QString department = obj["department"].toString();
        int queueNumber = obj["queue_number"].toInt();

        resultBrowser->append("<b><font color='green'>✅ 挂号成功！</font></b>");
        resultBrowser->append(QString("🏥 科室: <b>%1</b>").arg(department));
        resultBrowser->append(QString("🎫 挂号单号: <b>%1</b>").arg(ticketId));
        resultBrowser->append(QString("👥 当前排队人数: <b>%1</b>").arg(queueNumber));
    } else {
        resultBrowser->append("<font color='red'>❌ 挂号失败: " + obj["message"].toString() + "</font>");
    }

    reply->deleteLater();
}
