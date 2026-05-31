#include "smarttriage.h"
#include "config.h"
#include "offline_triage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

SmartTriage::SmartTriage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    QLabel *titleLabel = new QLabel("🤖 AI 智能导诊助手");
    titleLabel->setFont(QFont("", 20, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QLabel *symptomLabel = new QLabel("📝 请描述您的症状：");
    symptomLabel->setFont(QFont("", 14));
    mainLayout->addWidget(symptomLabel);

    symptomEdit = new QTextEdit();
    symptomEdit->setFont(QFont("", 14));
    symptomEdit->setPlaceholderText("例如：头疼、发烧38度、咳嗽三天...");
    symptomEdit->setMinimumHeight(120);
    mainLayout->addWidget(symptomEdit);

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

    QLabel *resultLabel = new QLabel("📋 分诊结果：");
    resultLabel->setFont(QFont("", 14));
    mainLayout->addWidget(resultLabel);

    resultBrowser = new QTextBrowser();
    resultBrowser->setFont(QFont("", 13));
    resultBrowser->setMinimumHeight(200);
    mainLayout->addWidget(resultBrowser);

    connect(analyzeBtn, &QPushButton::clicked, this, &SmartTriage::onAnalyzeClicked);
    connect(registerBtn, &QPushButton::clicked, this, &SmartTriage::onRegisterClicked);

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &SmartTriage::onReplyFinished);
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

    Config::Mode mode = Config::instance().getMode();

    if (mode == Config::MODE_DEMO) {
        // 演示模式：离线分诊
        QString result = OfflineTriage::instance().triage(symptoms);
        currentTriageResult = result;
        resultBrowser->append("<b>📋 分诊结果：</b>");
        resultBrowser->append(result);
    } else {
        // 生产模式：调用后端 AI
        QUrl url(Config::instance().getBackendUrl() + "/api/triage");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject jsonBody;
        jsonBody["symptoms"] = symptoms;

        networkManager->post(request, QJsonDocument(jsonBody).toJson());
    }
}

void SmartTriage::onRegisterClicked()
{
    if (currentTriageResult.isEmpty()) {
        resultBrowser->append("<font color='red'>⚠️ 请先进行智能分诊！</font>");
        return;
    }

    resultBrowser->append("<font color='blue'>🏥 正在为您挂号...</font>");

    // 简单提取科室
    QString department = "内科";
    if (currentTriageResult.contains("神经内科")) department = "神经内科";
    else if (currentTriageResult.contains("眼科")) department = "眼科";
    else if (currentTriageResult.contains("呼吸内科")) department = "呼吸内科";
    else if (currentTriageResult.contains("消化内科")) department = "消化内科";
    else if (currentTriageResult.contains("心内科")) department = "心内科";

    static int demoQueueNumber = 0;
    demoQueueNumber++;
    QString ticketId = QString("DEMO%1").arg(demoQueueNumber, 4, 10, QChar('0'));

    resultBrowser->append("<b><font color='green'>✅ 挂号成功！</font></b>");
    resultBrowser->append(QString("🏥 科室: <b>%1</b>").arg(department));
    resultBrowser->append(QString("🎫 挂号单号: <b>%1</b>").arg(ticketId));
    resultBrowser->append(QString("👥 当前排队人数: <b>%1</b>").arg(demoQueueNumber));
}

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
