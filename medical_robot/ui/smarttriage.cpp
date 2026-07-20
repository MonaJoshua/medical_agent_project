#include "smarttriage.h"
#include "network/voicemanager.h"
#include "core/offline_triage.h"
#include "core/config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QProcess>

SmartTriage::SmartTriage(QWidget *parent)
    : QWidget(parent)
    , voiceManager(nullptr)
    , m_audioInput(nullptr)
    , m_audioDevice(nullptr)
    , m_isRecording(false)
{
    m_isMultiRoundMode = true;   // 默认多轮预诊
    m_state = STATE_IDLE;
    m_collectedSymptoms.clear();
    m_collectedDuration.clear();
    m_collectedHistory.clear();

    // ===== 全局 QSS 美化 =====
    setStyleSheet(R"(
        SmartTriage {
            background-color: #f5f9fc;
        }
        QLabel {
            color: #2c3e50;
        }
        QTextEdit, QTextBrowser {
            background-color: #ffffff;
            border: 2px solid #d5e4ed;
            border-radius: 10px;
            padding: 10px;
            color: #2c3e50;
            selection-background-color: #b3d9f2;
        }
        QTextEdit:focus, QTextBrowser:focus {
            border: 2px solid #7ab8d9;
        }
        QPushButton {
            background-color: #1a73e8;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1a73e8;
        }
        QPushButton:pressed {
            background-color: #1a73e8;
        }
        QPushButton#refreshDeptBtn {
            background-color: #a8c8e0;
            border-radius: 6px;
            padding: 6px 12px;
            min-width: 36px;
        }
        QPushButton#refreshDeptBtn:hover {
            background-color: #8db8d4;
        }
        QComboBox {
            background-color: #ffffff;
            border: 2px solid #d5e4ed;
            border-radius: 8px;
            padding: 6px 12px;
            color: #2c3e50;
            min-width: 180px;
            min-height: 28px;
        }
        QComboBox:focus {
            border: 2px solid #7ab8d9;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 24px;
            border-left: 1px solid #d5e4ed;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
        }
        QComboBox::down-arrow {
            width: 12px;
            height: 12px;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            border: 1px solid #d5e4ed;
            border-radius: 6px;
            selection-background-color: #b3d9f2;
            color: #2c3e50;
            padding: 4px;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 20, 24, 20);

    // ----- 标题 -----
    QLabel *titleLabel = new QLabel("🤖 AI 智能导诊助手");
    titleLabel->setFont(QFont("", 22, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #2c5f8a; margin-bottom: 4px;");
    mainLayout->addWidget(titleLabel);

    // ----- 症状输入 -----
    QLabel *symptomLabel = new QLabel("📝 请描述您的症状：");
    symptomLabel->setFont(QFont("", 15));
    mainLayout->addWidget(symptomLabel);

    symptomEdit = new QTextEdit();
    symptomEdit->setFont(QFont("", 14));
    symptomEdit->setPlaceholderText("例如：头疼、发烧38度、咳嗽三天...");
    symptomEdit->setMinimumHeight(110);
    mainLayout->addWidget(symptomEdit);

    // ----- 三个操作按钮 -----
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(16);

    QPushButton *analyzeBtn = new QPushButton("🔍 智能分诊");
    analyzeBtn->setFont(QFont("", 14));
    analyzeBtn->setMinimumHeight(42);

    QPushButton *registerBtn = new QPushButton(" 一键挂号");
    registerBtn->setFont(QFont("", 14));
    registerBtn->setMinimumHeight(42);

    voiceBtn = new QPushButton("🎤 语音输入");
    voiceBtn->setFont(QFont("", 14));
    voiceBtn->setMinimumHeight(42);

    buttonLayout->addWidget(analyzeBtn);
    buttonLayout->addWidget(registerBtn);
    buttonLayout->addWidget(voiceBtn);
    mainLayout->addLayout(buttonLayout);

    // ----- 科室选择 -----
    QHBoxLayout *deptLayout = new QHBoxLayout();
    deptLayout->setSpacing(10);

    QLabel *deptLabel = new QLabel("🏢 选择科室：");
    deptLabel->setFont(QFont("", 14));
    deptLayout->addWidget(deptLabel);

    departmentCombo = new QComboBox();
    departmentCombo->setFont(QFont("", 13));
    departmentCombo->addItem("自动（根据分诊）");
    departmentCombo->setMinimumHeight(36);
    deptLayout->addWidget(departmentCombo);

    // 刷新按钮
    QPushButton *refreshDeptBtn = new QPushButton();
    refreshDeptBtn->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refreshDeptBtn->setToolTip("刷新科室列表");
    refreshDeptBtn->setObjectName("refreshDeptBtn");
    refreshDeptBtn->setMinimumSize(36, 36);
    deptLayout->addWidget(refreshDeptBtn);

    deptLayout->addStretch();
    mainLayout->addLayout(deptLayout);

    // 连接刷新按钮
    connect(refreshDeptBtn, &QPushButton::clicked, this, &SmartTriage::onDepartmentRefreshClicked);

    //重置按钮
    QPushButton *resetBtn = new QPushButton("🔄 重新导诊");
    resetBtn->setFont(QFont("", 12));
    resetBtn->setFixedHeight(36);
    connect(resetBtn, &QPushButton::clicked, [this]() {
        m_state = STATE_IDLE;
        m_collectedSymptoms.clear();
        m_collectedDuration.clear();
        m_collectedHistory.clear();
        resultBrowser->append("<font color='blue'>🔄 已重置，请重新描述症状</font>");
    });
    mainLayout->addWidget(resetBtn);

    // ----- 分诊结果 -----
    QLabel *resultLabel = new QLabel("📋 分诊结果：");
    resultLabel->setFont(QFont("", 15));
    mainLayout->addWidget(resultLabel);

    resultBrowser = new QTextBrowser();
    resultBrowser->setFont(QFont("", 13));
    resultBrowser->setMinimumHeight(180);
    mainLayout->addWidget(resultBrowser);

    // ----- 信号槽连接 -----
    connect(analyzeBtn, &QPushButton::clicked, this, &SmartTriage::onAnalyzeClicked);
    connect(registerBtn, &QPushButton::clicked, this, &SmartTriage::onRegisterClicked);

    connect(voiceBtn, &QPushButton::clicked, [this]() {
        if (m_isRecording) {
            stopRecording();
        } else {
            startRecording();
        }
    });

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &SmartTriage::onReplyFinished);

    // ----- 语音模块初始化 -----
    voiceManager = new VoiceManager(this);

    QString baiduApiKey = QString::fromLocal8Bit(qgetenv("BAIDU_API_KEY"));
    QString baiduSecretKey = QString::fromLocal8Bit(qgetenv("BAIDU_SECRET_KEY"));

    if (baiduApiKey.isEmpty() || baiduSecretKey.isEmpty()) {
        resultBrowser->append("<font color=\"orange\">⚠️ 语音功能未配置：请设置环境变量 BAIDU_API_KEY 和 BAIDU_SECRET_KEY</font>");
    } else {
        voiceManager->setApiKeys(baiduApiKey, baiduSecretKey);
        resultBrowser->append("<font color=\"green\">✅ 语音模块已就绪</font>");
    }

    connect(voiceManager, &VoiceManager::recognitionFinished, this, &SmartTriage::onVoiceRecognized);
    connect(voiceManager, &VoiceManager::recognitionError, this, [this](const QString &error) {
        resultBrowser->append("<font color=\"red\">❌ 语音识别失败: " + error + "</font>");
    });

    // ----- 加载科室列表 -----
    loadDepartments();
}

SmartTriage::~SmartTriage()
{
}

// ========== 智能分诊 ==========
void SmartTriage::onAnalyzeClicked()
{
    QString input = symptomEdit->toPlainText().trimmed();
    if (input.isEmpty()) {
        resultBrowser->append("<font color='red'>请描述您的症状</font>");
        return;
    }

    if (m_isMultiRoundMode) {
        // ===== 多轮预诊模式 =====
        switch (m_state) {
        case STATE_IDLE:
        case STATE_ASK_SYMPTOM:
            m_collectedSymptoms = input;
            m_state = STATE_ASK_DURATION;
            resultBrowser->append("<font color='blue'>📋 已记录症状，请回答：这个症状持续多久了？（如：3天、一周）</font>");
            symptomEdit->clear();
            break;

        case STATE_ASK_DURATION:
            m_collectedDuration = input;
            m_state = STATE_ASK_HISTORY;
            resultBrowser->append("<font color='blue'>📋 已记录持续时间，请回答：您是否有高血压、糖尿病等既往病史？</font>");
            symptomEdit->clear();
            break;

        case STATE_ASK_HISTORY:
            m_collectedHistory = input;
            m_state = STATE_SUMMARY_DONE;
            generatePreDiagnosis();
            break;

        default:
            break;
        }
    } else {
        // ===== 快速分诊模式 =====
        resultBrowser->append("<font color='blue'>🤖 正在分析中...</font>");
        Config::Mode mode = Config::instance().getMode();
        if (mode == Config::MODE_DEMO) {
            QString result = OfflineTriage::instance().triage(input);
            currentTriageResult = result;
            resultBrowser->append("<b>📋 分诊结果：</b>");
            resultBrowser->append(result);
        } else {
            QUrl url(Config::instance().getBackendUrl() + "/api/triage");
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QJsonObject jsonBody;
            jsonBody["symptoms"] = input;
            networkManager->post(request, QJsonDocument(jsonBody).toJson());
        }
        // 重置状态机（防止状态残留）
        resetTriage();
    }
}

void SmartTriage::generatePreDiagnosis()
{
    QString fullPrompt = QString(
        "请根据以下信息生成一份预诊摘要：\n"
        "症状：%1\n"
        "持续时间：%2\n"
        "既往病史：%3\n"
        "格式要求：分条列出初步判断、可能病因、建议就诊科室"
    ).arg(m_collectedSymptoms, m_collectedDuration, m_collectedHistory);

    if (Config::instance().getMode() == Config::MODE_PRODUCTION) {
        // ===== 生产模式：调用 AI API =====
        QNetworkAccessManager *preDiagManager = new QNetworkAccessManager(this);
        QUrl url(Config::instance().getBackendUrl() + "/api/triage");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject jsonBody;
        jsonBody["symptoms"] = fullPrompt;

        QNetworkReply *reply = preDiagManager->post(request, QJsonDocument(jsonBody).toJson());
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                QJsonObject obj = doc.object();
                if (obj["success"].toBool()) {
                    QString aiResult = obj["result"].toString();
                    // 组合完整摘要
                    QString summary = QString(
                        "📋 预诊摘要：\n"
                        "%1\n"
                        "🕐 持续时间：%2\n"
                        "📋 既往病史：%3"
                    ).arg(aiResult, m_collectedDuration, m_collectedHistory);
                    resultBrowser->append(summary);
                } else {
                    resultBrowser->append("<font color='red'>❌ AI 生成预诊摘要失败</font>");
                }
            } else {
                resultBrowser->append("<font color='red'>❌ 网络错误，请检查后端服务</font>");
            }
            reply->deleteLater();
            sender()->deleteLater();  // 释放预诊管理器
            resetTriage();            // 重置状态机
        });
    } else {
        // ===== 演示模式（离线）：基于规则生成摘要 =====
        QString offlineResult = OfflineTriage::instance().triage(m_collectedSymptoms);
        QString summary = QString(
            "📋 预诊摘要：\n"
            "%1\n"
            "🕐 持续时间：%2\n"
            "📋 既往病史：%3"
        ).arg(offlineResult, m_collectedDuration, m_collectedHistory);
        resultBrowser->append(summary);
        resetTriage();
    }
}

// ========== 一键挂号 ==========
void SmartTriage::onRegisterClicked()
{
    QString department;
    bool manualSelected = false;

    // 检查用户是否手动选择了科室
    if (departmentCombo->currentIndex() > 0) {
        department = departmentCombo->currentText();
        manualSelected = true;
    } else if (!currentTriageResult.isEmpty()) {
        // 从分诊结果中提取科室
        if (currentTriageResult.contains("神经内科")) department = "神经内科";
        else if (currentTriageResult.contains("眼科")) department = "眼科";
        else if (currentTriageResult.contains("呼吸内科")) department = "呼吸内科";
        else if (currentTriageResult.contains("消化内科")) department = "消化内科";
        else if (currentTriageResult.contains("心内科")) department = "心内科";
        else department = "内科";
    }

    if (department.isEmpty()) {
        resultBrowser->append("<font color=\"red\">⚠️ 请先进行智能分诊或选择科室</font>");
        return;
    }

    resultBrowser->append("<font color=\"blue\">🏢 正在为您挂号...</font>");

    if (manualSelected) {
        resultBrowser->append(QString("👉 已手动选择科室: <b>%1</b>").arg(department));
    }

    // 调用后端 API 挂号
    QUrl url(Config::instance().getBackendUrl() + "/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // API Key 验证（从环境变量读取）
    QString apiKey = QString::fromLocal8Bit(qgetenv("MEDICAL_API_KEY"));
    if (apiKey.isEmpty()) {
        qWarning() << "未设置 MEDICAL_API_KEY 环境变量，使用默认 Key（仅用于测试）";
        apiKey = "secure_medical_api_key_2026";
    }
    request.setRawHeader("X-API-Key", apiKey.toUtf8());

    QJsonObject jsonBody;
    jsonBody["department"] = department;
    jsonBody["patient_name"] = "患者";
    jsonBody["symptoms"] = symptomEdit->toPlainText();

    // 使用独立的 NetworkManager 发送挂号请求
    QNetworkAccessManager *regManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = regManager->post(request, QJsonDocument(jsonBody).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, regManager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            if (data.isEmpty()) {
                resultBrowser->append("<font color='red'>❌ 后端返回空数据</font>");
            } else {
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isNull()) {
                    resultBrowser->append("<font color='red'>❌ 解析挂号响应失败</font>");
                } else {
                    QJsonObject obj = doc.object();
                    if (obj["success"].toBool()) {
                        resultBrowser->append("<b><font color=\"green\">✅ 挂号成功！</font></b>");
                        resultBrowser->append(QString("🏥 科室: <b>%1</b>").arg(obj["department"].toString()));
                        resultBrowser->append(QString("🎫 挂号单号: <b>%1</b>").arg(obj["ticket_id"].toString()));
                        resultBrowser->append(QString("👥 当前排队人数: <b>%1</b>").arg(obj["queue_number"].toInt()));
                    } else {
                        resultBrowser->append("<font color=\"red\">❌ 挂号失败: " + obj["message"].toString() + "</font>");
                    }
                }
            }
        } else {
            resultBrowser->append("<font color=\"red\">❌ 网络错误: " + reply->errorString() + "</font>");
        }
        reply->deleteLater();
        regManager->deleteLater();
    });
}

// ========== 网络响应处理 ==========
void SmartTriage::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        resultBrowser->append(QString("<font color=\"red\">❌ 网络错误: %1</font>").arg(reply->errorString()));
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
        resultBrowser->append("<font color=\"red\">❌ " + obj["message"].toString() + "</font>");
    }
    reply->deleteLater();
}

// ========== 语音输入 ==========
void SmartTriage::onVoiceInputClicked()
{
    resultBrowser->append("<font color=\"blue\">🎤 正在识别语音...</font>");

    QString audioPath = QDir::homePath() + "/test_audio.wav";

    if (!QFile::exists(audioPath)) {
        resultBrowser->append("<font color=\"red\">❌ 测试音频文件不存在: " + audioPath + "</font>");
        resultBrowser->append("<font color=\"red\">💡 请将音频文件放到: " + audioPath + "</font>");
        return;
    }

    voiceManager->recognizeFromFile(audioPath);
}

void SmartTriage::onVoiceRecognized(const QString &text)
{
    symptomEdit->setText(text);
    resultBrowser->append(QString("<font color='green'>🎤 识别结果：%1</font>").arg(text));
    onAnalyzeClicked(); // 自动触发分诊
}

//========== 开始/停止录音 ==========
void SmartTriage::startRecording()
{
    // Qt 5 音频 API
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (info.isNull()) {
        resultBrowser->append("<font color='red'>未找到麦克风设备</font>");
        return;
    }

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    if (!info.isFormatSupported(format)) {
        resultBrowser->append("<font color='red'>音频格式不支持</font>");
        return;
    }

    m_audioInput = new QAudioInput(info, format, this);
    m_audioBuffer.clear();
    m_audioDevice = m_audioInput->start();
    m_isRecording = true;

    connect(m_audioDevice, &QIODevice::readyRead, this, &SmartTriage::processAudioData);

    voiceBtn->setText("🔴 停止录音");
    resultBrowser->append("<font color='blue'>🎤 正在录音，请说话...</font>");
}

void SmartTriage::stopRecording()
{
    if (m_audioInput) {
        m_audioInput->stop();
        m_isRecording = false;
        voiceBtn->setText("🎤 语音输入");
        resultBrowser->append("<font color='blue'>⏹️ 录音结束，正在识别...</font>");
        recognizeAudio();
    }
}

void SmartTriage::processAudioData()
{
    if (m_audioDevice) {
        m_audioBuffer.append(m_audioDevice->readAll());
    }
}

// ========== 语音识别 ==========
void SmartTriage::recognizeAudio()
{
    // 保存录音为 WAV 格式
    QString tempFile = "/tmp/voice_input.wav";
    QFile file(tempFile);
    if (file.open(QIODevice::WriteOnly)) {
        // 添加 WAV 头（需要实现，或使用已有函数）
        file.write(m_audioBuffer);
        file.close();
    }

    // 调用 VoiceManager 识别
    voiceManager->recognizeFromFile(tempFile);
}

// ========== 填充科室下拉框 ==========
void SmartTriage::populateDepartmentCombo()
{
    if (!departmentCombo) return;

    QString currentSelection = departmentCombo->currentText();
    departmentCombo->blockSignals(true);
    departmentCombo->clear();
    departmentCombo->addItem("自动（根据分诊）");
    for (const QString &dept : m_departments) {
        departmentCombo->addItem(dept);
    }
    // 恢复之前的选择（如果存在）
    int idx = departmentCombo->findText(currentSelection);
    if (idx >= 0) {
        departmentCombo->setCurrentIndex(idx);
    }
    departmentCombo->blockSignals(false);
    departmentCombo->update();
}

// ========== 从后端加载科室列表（使用 curl）==========
void SmartTriage::loadDepartments()
{
    // 检查 curl 是否可用
    QProcess process;
    process.start("curl", QStringList() << "--version");
    if (!process.waitForFinished(1000) || process.exitCode() != 0) {
        resultBrowser->append("<font color='orange'>⚠️ 系统缺少 curl 命令，无法获取科室列表，使用默认科室</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    // 执行 curl 获取科室数据
    process.start("curl", QStringList() << "-s" << "http://127.0.0.1:5000/api/departments");
    if (!process.waitForFinished(5000)) {
        resultBrowser->append("<font color='orange'>⚠️ 请求科室列表超时，请检查网络或后端服务是否运行</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    if (process.exitCode() != 0) {
        resultBrowser->append("<font color='orange'>⚠️ 获取科室列表失败，请检查后端服务是否运行</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    QByteArray data = process.readAllStandardOutput();
    if (data.isEmpty()) {
        resultBrowser->append("<font color='orange'>⚠️ 后端返回数据为空，使用默认科室列表</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        resultBrowser->append("<font color='orange'>⚠️ 解析科室数据失败，使用默认科室列表</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    QJsonObject obj = doc.object();
    QJsonArray depts = obj["departments"].toArray();
    if (depts.isEmpty()) {
        resultBrowser->append("<font color='orange'>⚠️ 科室列表为空，使用默认科室</font>");
        m_departments.clear();
        m_departments << "全科门诊" << "内科" << "外科" << "儿科" << "皮肤科";
        populateDepartmentCombo();
        return;
    }

    m_departments.clear();
    for (const auto &item : depts) {
        m_departments.append(item.toString());
    }
    populateDepartmentCombo();
}

// ========== 刷新科室列表 ==========
void SmartTriage::onDepartmentRefreshClicked()
{
    resultBrowser->append("<font color=\"blue\">🔄 正在刷新科室列表...</font>");
    loadDepartments();
}

// ========== 设置分诊模式 ==========
void SmartTriage::setMultiRoundMode(bool enable)
{
    m_isMultiRoundMode = enable;
    // 如果切换到快速分诊，重置状态机
    if (!enable) {
        m_state = STATE_IDLE;
        m_collectedSymptoms.clear();
        m_collectedDuration.clear();
        m_collectedHistory.clear();
        resultBrowser->append("<font color='blue'>⚡ 已切换到快速分诊模式</font>");
    } else {
        resultBrowser->append("<font color='blue'>🧠 已切换到多轮预诊模式</font>");
    }
}

// ========== 重置导诊状态 ==========
void SmartTriage::resetTriage()
{
    m_state = STATE_IDLE;
    m_collectedSymptoms.clear();
    m_collectedDuration.clear();
    m_collectedHistory.clear();
    resultBrowser->append("<font color='blue'>🔄 已重置，请重新描述症状</font>");
}