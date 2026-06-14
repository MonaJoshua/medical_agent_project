#include "smarttriage.h"
#include "network/voicemanager.h"
#include "core/offline_triage.h"
#include "core/config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QDebug>

SmartTriage::SmartTriage(QWidget *parent)
    : QWidget(parent)
    , voiceManager(nullptr)
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

    QPushButton *voiceBtn = new QPushButton("🎤 语音输入");
    voiceBtn->setFont(QFont("", 14));
    voiceBtn->setMinimumHeight(40);

    buttonLayout->addWidget(analyzeBtn);
    buttonLayout->addWidget(registerBtn);
    buttonLayout->addWidget(voiceBtn);
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
    connect(voiceBtn, &QPushButton::clicked, this, &SmartTriage::onVoiceInputClicked);

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &SmartTriage::onReplyFinished);

    // 初始化语音管理器，从环境变量读取密钥
    voiceManager = new VoiceManager(this);
    
    QString baiduApiKey = QString::fromLocal8Bit(qgetenv("BAIDU_API_KEY"));
    QString baiduSecretKey = QString::fromLocal8Bit(qgetenv("BAIDU_SECRET_KEY"));
    
    if (baiduApiKey.isEmpty() || baiduSecretKey.isEmpty()) {
        resultBrowser->append("<font color='orange'>⚠️ 语音功能未配置：请设置环境变量 BAIDU_API_KEY 和 BAIDU_SECRET_KEY</font>");
    } else {
        voiceManager->setApiKeys(baiduApiKey, baiduSecretKey);
        resultBrowser->append("<font color='green'>✅ 语音模块已就绪</font>");
    }
    
    connect(voiceManager, &VoiceManager::recognitionFinished, this, &SmartTriage::onVoiceRecognized);
    connect(voiceManager, &VoiceManager::recognitionError, this, [this](const QString &error) {
        resultBrowser->append("<font color='red'>❌ 语音识别失败: " + error + "</font>");
    });
}

SmartTriage::~SmartTriage()
{
}

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
        QString result = OfflineTriage::instance().triage(symptoms);
        currentTriageResult = result;
        resultBrowser->append("<b>📋 分诊结果：</b>");
        resultBrowser->append(result);
    } else {
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

void SmartTriage::onVoiceInputClicked()
{
    resultBrowser->append("<font color='blue'>🎤 正在识别语音...</font>");
    
    QString audioPath = QDir::homePath() + "/test_audio.wav";
    
    if (!QFile::exists(audioPath)) {
        resultBrowser->append("<font color='red'>❌ 测试音频文件不存在: " + audioPath + "</font>");
        resultBrowser->append("<font color='red'>💡 请将音频文件放到: " + audioPath + "</font>");
        return;
    }
    
    voiceManager->recognizeFromFile(audioPath);
}

void SmartTriage::onVoiceRecognized(const QString &text)
{
    resultBrowser->append("<font color='green'>🎤 识别结果: " + text + "</font>");
    symptomEdit->setText(text);
    onAnalyzeClicked();
}
