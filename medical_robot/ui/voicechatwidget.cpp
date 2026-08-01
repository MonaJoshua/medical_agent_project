// voicechatwidget.cpp
// 语音助手组件：语音识别 + 意图理解 + 多轮对话

#include "voicechatwidget.h"
#include "thememanager.h"
#include "network/voicemanager.h"
#include "network/apiclient.h"
#include "core/config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QRegularExpression>
#include <QApplication>
#include <QScrollBar>
#include <QProcess>
#include <QDebug>
#include <QSizePolicy>
#include <QDate>

// ============================================================
// 构造 / 析构
// ============================================================

VoiceChatWidget::VoiceChatWidget(QWidget *parent)
    : QWidget(parent)
    , voiceManager(nullptr)
    , m_isRecording(false)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_state(ChatState::IDLE)
    , m_isDarkMode(false)
    , m_voiceEnabled(false)
    , m_recordingProcess(nullptr)
{
    voiceManager = new VoiceManager(this);

    // 从环境变量读取百度密钥
    QString apiKey = qgetenv("BAIDU_API_KEY");
    QString secretKey = qgetenv("BAIDU_SECRET_KEY");
    if (!apiKey.isEmpty() && !secretKey.isEmpty()) {
        voiceManager->setApiKeys(apiKey, secretKey);
        qDebug() << "✅ 百度语音密钥已加载";
    } else {
        qWarning() << "⚠️ 未设置 BAIDU_API_KEY 或 BAIDU_SECRET_KEY";
    }

    connect(voiceManager, &VoiceManager::recognitionFinished,
            this, &VoiceChatWidget::onVoiceRecognized);
    connect(voiceManager, &VoiceManager::recognitionError,
            this, &VoiceChatWidget::onVoiceError);

    setupUI();
    updateVoiceUI();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme theme) {
                m_isDarkMode = (theme == ThemeManager::Dark);
            });

    // 欢迎消息
    QString welcome = QStringLiteral(
        "<div style='font-size:16px;'>"
        "👋 你好！我是你的医疗助手。<br><br>"
        "💬 你可以这样与我互动：<br>"
        "• <b>健康咨询</b>：直接描述你的健康问题（如“熬夜有什么危害？”）<br>"
        "• <b>挂号</b>：说“我要挂号”或“预约”，我会引导你完成<br>"
        "• <b>查询号源</b>：说“ 某科室有没有号源”<br>"
        "• <b>查看记录</b>：说“我的记录”<br>"
        "• <b>帮助</b>：说“帮助”<br><br>"
        "🎤 点击下方麦克风可语音输入</div>"
    );
    addSystemBubble(welcome);
    loadDepartments();
}

VoiceChatWidget::~VoiceChatWidget()
{
    if (m_isRecording) stopRecording();
    if (m_recordingProcess) {
        if (m_recordingProcess->state() == QProcess::Running)
            m_recordingProcess->terminate();
        m_recordingProcess->waitForFinished(1000);
        delete m_recordingProcess;
        m_recordingProcess = nullptr;
    }
}

void VoiceChatWidget::setDarkMode(bool dark)
{
    m_isDarkMode = dark;
}

// ============================================================
// UI 初始化
// ============================================================

void VoiceChatWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 6, 8, 6);

    // 顶部栏
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);

    QLabel *titleLabel = new QLabel("语音助手");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setFont(QFont("", 20, QFont::Bold));
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    m_voiceToggleBtn = new QPushButton("🔇");
    m_voiceToggleBtn->setObjectName("voiceToggleBtn");
    m_voiceToggleBtn->setCheckable(true);
    m_voiceToggleBtn->setChecked(false);
    m_voiceToggleBtn->setFixedHeight(36);
    m_voiceToggleBtn->setFont(QFont("", 12));
    connect(m_voiceToggleBtn, &QPushButton::clicked, this, &VoiceChatWidget::onVoiceToggleClicked);
    topLayout->addWidget(m_voiceToggleBtn);

    helpBtn = new QPushButton("帮助");
    helpBtn->setObjectName("helpBtn");
    helpBtn->setFixedHeight(36);
    connect(helpBtn, &QPushButton::clicked, this, &VoiceChatWidget::onHelpClicked);
    topLayout->addWidget(helpBtn);

    resetBtn = new QPushButton("重置");
    resetBtn->setObjectName("resetBtn");
    resetBtn->setFixedHeight(36);
    connect(resetBtn, &QPushButton::clicked, this, &VoiceChatWidget::onResetClicked);
    topLayout->addWidget(resetBtn);

    mainLayout->addLayout(topLayout);

    // 聊天列表
    chatList = new QListWidget();
    chatList->setFont(QFont("", 14));
    chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chatList->setSelectionMode(QAbstractItemView::NoSelection);
    chatList->setFrameShape(QFrame::NoFrame);
    chatList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chatList->setUniformItemSizes(false);
    mainLayout->addWidget(chatList, 1);

    // 底部输入区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);

    voiceBtn = new QPushButton("🎤");
    voiceBtn->setObjectName("voiceBtn");
    connect(voiceBtn, &QPushButton::pressed, this, &VoiceChatWidget::onVoiceBtnPressed);
    connect(voiceBtn, &QPushButton::released, this, &VoiceChatWidget::onVoiceBtnReleased);
    inputLayout->addWidget(voiceBtn);

    textInput = new QLineEdit();
    textInput->setPlaceholderText("描述你的症状或说出指令...");
    textInput->setFont(QFont("", 15));
    connect(textInput, &QLineEdit::returnPressed, this, &VoiceChatWidget::onTextReturnPressed);
    inputLayout->addWidget(textInput, 1);

    sendBtn = new QPushButton("发送");
    sendBtn->setObjectName("sendBtn");
    connect(sendBtn, &QPushButton::clicked, this, &VoiceChatWidget::onSendTextClicked);
    inputLayout->addWidget(sendBtn);

    mainLayout->addLayout(inputLayout);
}

// ============================================================
// 语音控制
// ============================================================

void VoiceChatWidget::onVoiceToggleClicked()
{
    m_voiceEnabled = m_voiceToggleBtn->isChecked();
    updateVoiceUI();

    if (m_voiceEnabled) {
        m_voiceToggleBtn->setText("🔊");
        addSystemBubble("🔊 语音已开启");
    } else {
        m_voiceToggleBtn->setText("🔇");
        addSystemBubble("🔇 语音已关闭");
        if (m_isRecording) {
            stopRecording();
            voiceBtn->setText("🎤");
        }
    }
}

void VoiceChatWidget::updateVoiceUI()
{
    voiceBtn->setEnabled(m_voiceEnabled);
    voiceBtn->setStyleSheet(m_voiceEnabled ? "" : "QPushButton { opacity: 0.5; }");
}

// ----- 录音控制 -----

void VoiceChatWidget::onVoiceBtnPressed()
{
    if (!m_voiceEnabled) return;
    voiceBtn->setText("🔴");
    startRecording();
}

void VoiceChatWidget::onVoiceBtnReleased()
{
    if (!m_voiceEnabled) {
        voiceBtn->setText("🎤");
        return;
    }
    voiceBtn->setText("🎤");
    stopRecording();
}

void VoiceChatWidget::startRecording()
{
    if (m_isRecording) return;

    m_tempAudioFile = QDir::tempPath() + "/voice_record_" +
                      QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    if (!QDir(QDir::tempPath()).exists()) {
        m_tempAudioFile = "/tmp/voice_record_" +
                          QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    }

    QString cmd = QString("arecord -f S16_LE -r 16000 -c 1 %1").arg(m_tempAudioFile);
    m_recordingProcess = new QProcess(this);
    connect(m_recordingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VoiceChatWidget::onRecordingFinished);

    m_recordingProcess->start("bash", QStringList() << "-c" << cmd);
    if (!m_recordingProcess->waitForStarted(2000)) {
        qWarning() << "启动 arecord 失败，请检查是否安装 arecord";
        delete m_recordingProcess;
        m_recordingProcess = nullptr;
        return;
    }
    m_isRecording = true;
    voiceBtn->setText("🔴");
}

void VoiceChatWidget::stopRecording()
{
    if (!m_isRecording || !m_recordingProcess) return;

    if (m_recordingProcess->state() == QProcess::Running) {
        m_recordingProcess->terminate();
        if (!m_recordingProcess->waitForFinished(2000)) {
            m_recordingProcess->kill();
            m_recordingProcess->waitForFinished(1000);
        }
    }
    m_isRecording = false;
    voiceBtn->setText("🎤");
}

void VoiceChatWidget::onRecordingFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);

    QFile file(m_tempAudioFile);
    if (!file.exists() || file.size() == 0) {
        addSystemBubble("录音文件无效或未录制内容，请重试");
        if (m_recordingProcess) {
            m_recordingProcess->deleteLater();
            m_recordingProcess = nullptr;
        }
        return;
    }

    qDebug() << "录音文件生成，大小：" << file.size() << "字节";

    // 尝试用 ffmpeg 增益
    QString gainFile = QDir::tempPath() + "/voice_gain_" +
                       QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    QString cmd = QString("ffmpeg -i %1 -af \"volume=2\" %2 -y")
                  .arg(m_tempAudioFile, gainFile);
    int ret = system(cmd.toUtf8().constData());
    if (ret == 0 && QFile::exists(gainFile) && QFileInfo(gainFile).size() > 0) {
        voiceManager->recognizeFromFile(gainFile);
        QTimer::singleShot(5000, [gainFile]() { QFile::remove(gainFile); });
    } else {
        voiceManager->recognizeFromFile(m_tempAudioFile);
    }

    QTimer::singleShot(5000, [path = m_tempAudioFile]() { QFile::remove(path); });

    if (m_recordingProcess) {
        m_recordingProcess->deleteLater();
        m_recordingProcess = nullptr;
    }
}

// ----- 语音识别回调 -----

void VoiceChatWidget::onVoiceRecognized(const QString &text)
{
    if (!text.trimmed().isEmpty()) {
        addUserBubble(text.trimmed());
        processUserInput(text.trimmed());
    }
}

void VoiceChatWidget::onVoiceError(const QString &error)
{
    addSystemBubble("语音识别失败：" + error);
}

// ============================================================
// 文本交互
// ============================================================

void VoiceChatWidget::onSendTextClicked()
{
    QString text = textInput->text().trimmed();
    if (!text.isEmpty()) {
        addUserBubble(text);
        textInput->clear();
        processUserInput(text);
    }
}

void VoiceChatWidget::onTextReturnPressed()
{
    onSendTextClicked();
}

void VoiceChatWidget::onHelpClicked()
{
    executeHelp();
}

void VoiceChatWidget::onResetClicked()
{
    resetConversation();
}

// ============================================================
// 聊天 UI 辅助
// ============================================================

void VoiceChatWidget::addChatMessage(const ChatMessage &msg)
{
    QListWidgetItem *item = new QListWidgetItem();
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(0);

    QLabel *label = new QLabel(buildChatHtml(msg));
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    label->setMaximumWidth(750);
    label->setMinimumWidth(500);

    connect(label, &QLabel::linkActivated, this, &VoiceChatWidget::onLinkActivated);

    if (msg.type == ChatMessageType::User) {
        QString bg = m_isDarkMode ? "#1a73e8" : "#d2e3fc";
        QString color = m_isDarkMode ? "#ffffff" : "#1a1a1a";
        label->setStyleSheet(QString("background-color: %1; border-radius: 12px; padding: 8px 12px; color: %2;").arg(bg, color));
        layout->addStretch();
        layout->addWidget(label);
    } else if (msg.type == ChatMessageType::Card) {
        QString bg = m_isDarkMode ? "#1e3a2a" : "#e6f4ea";
        QString color = m_isDarkMode ? "#e8e8e8" : "#2c3e50";
        label->setStyleSheet(QString("background-color: %1; border: 2px solid #34a853; border-radius: 12px; padding: 12px 16px; color: %2;").arg(bg, color));
        layout->addWidget(label);
        layout->addStretch();
    } else {
        QString bg = m_isDarkMode ? "#2d2d2d" : "#ffffff";
        QString border = m_isDarkMode ? "#444444" : "#e0e0e0";
        QString color = m_isDarkMode ? "#e8e8e8" : "#2c3e50";
        label->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: 12px; padding: 8px 12px; color: %3;").arg(bg, border, color));
        layout->addWidget(label);
        layout->addStretch();
    }

    container->setLayout(layout);
    chatList->addItem(item);
    chatList->setItemWidget(item, container);

    // 高度计算
    container->adjustSize();
    container->updateGeometry();

    int labelWidth = label->width();
    if (labelWidth <= 0) labelWidth = label->maximumWidth();

    int exactHeight = label->heightForWidth(labelWidth);
    if (exactHeight < 0) exactHeight = label->sizeHint().height();

    int totalHeight = exactHeight + 6;
    container->setFixedHeight(totalHeight);
    item->setSizeHint(QSize(labelWidth, totalHeight));

    container->updateGeometry();
    chatList->doItemsLayout();
    chatList->scrollToBottom();
}

void VoiceChatWidget::addUserBubble(const QString &text)
{
    addChatMessage(ChatMessage(ChatMessageType::User, text));
}

void VoiceChatWidget::addSystemBubble(const QString &text)
{
    addChatMessage(ChatMessage(ChatMessageType::System, text));
}

void VoiceChatWidget::addCardBubble(const QString &title, const QString &detail)
{
    addChatMessage(ChatMessage(ChatMessageType::Card, title, detail));
}

void VoiceChatWidget::appendSystemMessage(const QString &text)
{
    addSystemBubble(text);
}

void VoiceChatWidget::appendCardMessage(const QString &title, const QString &detail)
{
    addCardBubble(title, detail);
}

QString VoiceChatWidget::buildChatHtml(const ChatMessage &msg)
{
    switch (msg.type) {
    case ChatMessageType::User:
        return msg.text.toHtmlEscaped();
    case ChatMessageType::Card:
        return QString("<b>%1</b><br>%2").arg(msg.text.toHtmlEscaped(), msg.detail.toHtmlEscaped());
    default:
        return msg.text;
    }
}

void VoiceChatWidget::onLinkActivated(const QString &link)
{
    if (link == "refresh_slots" && !m_lastQueryDept.isEmpty()) {
        executeQuerySlots(m_lastQueryDept);
    } else if (link == "switch_to_more_tools") {
        emit requestSwitchToMoreTools();
    }
}

// ============================================================
// 核心业务逻辑
// ============================================================

void VoiceChatWidget::processUserInput(const QString &input)
{
    // 挂号确认状态
    if (m_state == ChatState::CONFIRM_REGISTER) {
        if (input.contains("确认挂号") || input.contains("确认")) {
            m_state = ChatState::WAITING_DEPT;
            addSystemBubble("正在获取最新科室列表...");
            loadDepartments([this]() {
                showDepartmentCard();
                speakText("请说出您要挂的科室名");
            });
        } else if (input.contains("取消")) {
            m_state = ChatState::IDLE;
            addSystemBubble("已取消挂号");
            speakText("已取消挂号");
        } else {
            addSystemBubble("请回复“确认挂号”或“取消”");
        }
        return;
    }

    // 等待科室状态
    if (m_state == ChatState::WAITING_DEPT) {
        if (input.contains("取消")) {
            m_state = ChatState::IDLE;
            addSystemBubble("已取消挂号");
            speakText("已取消挂号");
            return;
        }

        QString matchedDept;
        QString spokenDept = input.trimmed();
        for (const QString& dept : m_departments) {
            if (spokenDept.contains(dept, Qt::CaseInsensitive) || dept.contains(spokenDept, Qt::CaseInsensitive)) {
                matchedDept = dept;
                break;
            }
        }

        if (!matchedDept.isEmpty()) {
            // 查询号源
            QString backendUrl = Config::instance().getBackendUrl();
            QUrl url(backendUrl + "/api/slot/" + matchedDept);
            QNetworkRequest request(url);
            QNetworkReply* reply = m_networkManager->get(request);
            connect(reply, &QNetworkReply::finished, [this, reply, matchedDept]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    QJsonObject obj = doc.object();
                    if (obj["success"].toBool()) {
                        int remaining = obj["remaining_slots"].toInt();
                        if (remaining > 0) {
                            doRegister(matchedDept);
                            m_state = ChatState::IDLE;
                        } else {
                            addSystemBubble(QString("科室“%1”当前暂无号源，请选择其他科室或稍后再试").arg(matchedDept));
                            speakText("该科室暂无号源");
                            showDepartmentCard();
                        }
                    } else {
                        addSystemBubble(QString("科室“%1”不存在，请重新选择").arg(matchedDept));
                    }
                } else {
                    addSystemBubble("网络错误，请稍后重试");
                }
                reply->deleteLater();
            });
        } else {
            addSystemBubble("未识别到科室名，请重新说出科室名（如：全科门诊、内科）");
            speakText("未识别到科室名，请重新说出");
            showDepartmentCard();
        }
        return;
    }

    // 收集科室（号源查询）
    if (m_state == ChatState::COLLECTING_DEPT) {
        QString matchedDept;
        for (const QString& dept : m_departments) {
            if (input.contains(dept, Qt::CaseInsensitive) || dept.contains(input, Qt::CaseInsensitive)) {
                matchedDept = dept;
                break;
            }
        }
        if (!matchedDept.isEmpty()) {
            executeQuerySlots(matchedDept);
            m_state = ChatState::IDLE;
        } else {
            addSystemBubble("未识别到科室名，请重新说出科室名（如：全科门诊、内科）");
            speakText("未识别到科室名，请重新说出");
        }
        return;
    }

    // 意图识别
    QString intent = detectIntent(input);
    if (intent == "register") {
        m_state = ChatState::CONFIRM_REGISTER;
        addSystemBubble("确认要挂号吗？请说“确认挂号”或“取消”");
        return;
    }
    if (intent == "slots") {
        executeQuerySlots(QString());
        return;
    }
    if (intent == "records") {
        executeQueryRecords();
        return;
    }
    if (intent == "help") {
        executeHelp();
        return;
    }
    if (intent == "reset") {
        resetConversation();
        return;
    }

    // 默认：健康咨询
    executeChat(input);
}

// ============================================================
// 意图检测
// ============================================================

QString VoiceChatWidget::detectIntent(const QString &input)
{
    QString lower = input.trimmed().toLower();

    if (lower.contains("什么科室") || lower.contains("哪个科室") || lower.contains("哪些科室") ||
        lower.contains("挂哪个") || (lower.contains("推荐") && lower.contains("科"))) {
        return "chat";
    }

    if (lower.contains("我要挂号") || lower.contains("确认挂号") || lower.contains("预约挂号") ||
        lower.contains("帮我挂号") || (lower.contains("挂号") && !lower.contains("什么") && !lower.contains("哪个"))) {
        return "register";
    }

    if (lower.contains("号源") || lower.contains("剩余号源") || lower.contains("挂号名额") ||
        lower.contains("查询号源") || lower.contains("查看号源") || lower.contains("还有号") ||
        lower.contains("有没有号") || lower.contains("有无号源") || lower.contains("号源查询") ||
        lower.contains("余号") || lower.contains("剩余号") || lower.contains("可用") || lower.contains("还有")) {
        return "slots";
    }

    if (lower.contains("记录") || lower.contains("历史") || lower.contains("我的")) return "records";
    if (lower.contains("帮助") || lower.contains("help")) return "help";
    if (lower.contains("重置") || lower.contains("重启") || lower.contains("清空")) return "reset";

    return "chat";
}

// ============================================================
// 挂号
// ============================================================

void VoiceChatWidget::doRegister(const QString &department)
{
    m_state = ChatState::REGISTERING;
    addSystemBubble(QString("正在为 %1 挂号，请稍候...").arg(department));
    speakText(QString("正在为 %1 挂号。").arg(department));

    // 使用症状摘要（如有）
    QString symptomText = m_lastSymptomSummary.isEmpty() ? "无具体症状描述" : m_lastSymptomSummary;
    if (symptomText.length() > 30) symptomText = symptomText.left(30) + "...";

    // 使用 ApiClient 统一管理 HTTP 请求
    ApiClient::instance().registerPatient(department, "用户", symptomText,
        [this, department](bool ok, const QJsonObject& obj) {
            if (ok) {
                QString ticketId = obj["ticket_id"].toString();
                int queueNum = obj["queue_number"].toInt();
                QString deptName = obj["department"].toString();

                QString response = buildRegisterSuccessHtml(deptName, ticketId, queueNum);
                addSystemBubble(response);
                speakText(QString("挂号成功，科室 %1，票号 %2，排队号 %3。")
                          .arg(deptName, ticketId).arg(queueNum));
            } else {
                QString msg = obj["message"].toString("挂号失败");
                addSystemBubble("挂号失败：" + msg);
                speakText("挂号失败");
            }
            m_state = ChatState::IDLE;
        }
    );
}

QString VoiceChatWidget::buildRegisterSuccessHtml(const QString &dept, const QString &ticketId, int queueNum)
{
    return QString(
        "<div style='background-color: #e6f4ea; border: 2px solid #34a853; border-radius: 12px; padding: 16px; margin: 4px 0;'>"
        "<div style='font-size:18px; font-weight:bold; color:#1e7e34; margin-bottom:10px;'>✅ 挂号成功</div>"
        "<table style='font-size:15px; color:#2c3e50; line-height:2;'>"
        "<tr><td style='padding-right:16px; color:#5f6b7a;'>科室</td><td style='font-weight:bold;'>%1</td></tr>"
        "<tr><td style='padding-right:16px; color:#5f6b7a;'>票号</td><td style='font-weight:bold; color:#1a73e8;'>%2</td></tr>"
        "<tr><td style='padding-right:16px; color:#5f6b7a;'>排队号</td><td style='font-weight:bold; font-size:20px; color:#d93025;'>%3</td></tr>"
        "</table></div>"
    ).arg(dept, ticketId).arg(queueNum);
}

// ============================================================
// 查询号源
// ============================================================

void VoiceChatWidget::executeQuerySlots(const QString &department)
{
    if (!department.isEmpty()) {
        m_lastQueryDept = department;
        m_state = ChatState::QUERYING_SLOTS;
        addSystemBubble(QString("正在查询 %1 的号源...").arg(department));
        speakText(QString("正在查询 %1 的号源。").arg(department));

        QString backendUrl = Config::instance().getBackendUrl();
        QUrl url(backendUrl + "/api/slot/" + department);
        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, [this, reply, department]() {
            if (reply->error() != QNetworkReply::NoError) {
                QString errorMsg = (reply->error() == QNetworkReply::ContentNotFoundError) ?
                    QString("未找到科室“%1”的号源信息，请确认科室名称是否正确。").arg(department) :
                    QString("查询号源失败：" + reply->errorString());
                addSystemBubble(errorMsg);
                speakText("查询号源失败。");
            } else {
                QByteArray data = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj["success"].toBool()) {
                        int remaining = obj["remaining_slots"].toInt();
                        QString date = QDate::currentDate().toString("yyyy-MM-dd");

                        QString result = (remaining > 0) ?
                            QString(
                                "<div style='background-color: #f0f8ff; border-radius: 12px; padding: 16px; border: 1px solid #c0d4e6;'>"
                                "<div style='font-size:16px; font-weight:bold; margin-bottom:8px;'>🏥 %1 号源信息</div>"
                                "<div style='line-height:1.8;'>"
                                "<b>📅 日期：</b> %2<br>"
                                "<b style='color:#1a73e8; font-size:18px;'>✅ 剩余号源：%3</b><br>"
                                "你可以说 <b>\"挂号\"</b> 立即预约。"
                                "</div>"
                                "<div style='margin-top:12px; border-top:1px dashed #ccc; padding-top:8px;'>"
                                "<a href='refresh_slots' style='color:#1a73e8; text-decoration:none; font-weight:bold;'>🔄 刷新号源</a>"
                                "</div>"
                                "</div>"
                            ).arg(department, date).arg(remaining) :
                            QString(
                                "<div style='background-color: #fff0f0; border-radius: 12px; padding: 16px; border: 1px solid #e6b0b0;'>"
                                "<div style='font-size:16px; font-weight:bold; margin-bottom:8px;'>🏥 %1 号源信息</div>"
                                "<div style='line-height:1.8;'>"
                                "<span style='color:#d93025; font-size:16px;'>❌ 无可用号源</span><br><br>"
                                "建议选择其他科室或稍后再试。"
                                "</div>"
                                "<div style='margin-top:12px; border-top:1px dashed #ccc; padding-top:8px;'>"
                                "<a href='refresh_slots' style='color:#1a73e8; text-decoration:none; font-weight:bold;'>🔄 刷新号源</a>"
                                "</div>"
                                "</div>"
                            ).arg(department);

                        addSystemBubble(result);
                        speakText(remaining > 0 ?
                                  QString("%1 剩余 %2 个号源。").arg(department).arg(remaining) :
                                  QString("%1 没有可用号源。").arg(department));
                    } else {
                        addSystemBubble(obj["message"].toString());
                    }
                } else {
                    addSystemBubble("返回数据格式错误，请稍后重试。");
                }
            }
            m_state = ChatState::IDLE;
            reply->deleteLater();
        });
    } else {
        addSystemBubble("你想查询哪个科室的号源？例如：全科门诊、心血管内科...");
        speakText("你想查询哪个科室的号源？");
        m_state = ChatState::COLLECTING_DEPT;
    }
}

// ============================================================
// 查询记录
// ============================================================

void VoiceChatWidget::executeQueryRecords()
{
    m_state = ChatState::QUERYING_RECORDS;
    addSystemBubble("正在获取你的挂号记录...");
    speakText("正在获取你的挂号记录。");

    QString backendUrl = Config::instance().getBackendUrl();
    QUrl url(backendUrl + "/api/registrations");
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            addSystemBubble("获取记录失败：" + reply->errorString());
            m_state = ChatState::IDLE;
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        reply->deleteLater();

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj["success"].toBool()) {
                displayLatestRecord(obj["data"].toArray());
            } else {
                addSystemBubble("获取记录失败：" + obj["message"].toString());
            }
        } else {
            addSystemBubble("返回数据格式错误，请稍后重试。");
        }
        m_state = ChatState::IDLE;
    });
}

void VoiceChatWidget::displayLatestRecord(const QJsonArray &records)
{
    if (records.isEmpty()) {
        addSystemBubble("暂无挂号记录。你可以说“挂号”来预约。");
        return;
    }

    QJsonObject obj = records.first().toObject();
    QString ticket = obj["ticket_id"].toString();
    QString dept = obj["department"].toString();
    QString doctor = obj["doctor"].toString();
    QString symptoms = obj["symptoms"].toString();
    QString status = obj["status"].toString();
    QString created = obj["created_at"].toString();

    QString statusText, statusColor;
    if (status == "waiting") { statusText = "待就诊"; statusColor = "#d93025"; }
    else if (status == "completed") { statusText = "已完成"; statusColor = "#34a853"; }
    else if (status == "cancelled") { statusText = "已取消"; statusColor = "#8899aa"; }
    else { statusText = status; statusColor = "#8899aa"; }

    QString html = QString(
        "<div style='background-color: #f8f9fa; border: 1px solid #d5e4ed; border-radius: 8px; padding: 10px 14px; margin: 6px 0;'>"
        "<div style='font-weight: bold; color: #1a73e8;'>%1 - %2</div>"
        "<div style='font-size: 14px; color: #2c3e50;'>"
        "医生：%3<br>"
        "症状：%4<br>"
        "状态：<span style='font-weight: bold; color: %5;'>%6</span><br>"
        "挂号时间：%7<br><br>"
        "<span style='color: #1a73e8; font-weight: bold;'>💡 您共有 %8 条历史记录，</span>"
        "<a href='switch_to_more_tools' style='color: #1a73e8; text-decoration: underline;'>点击查看全部</a>"
        "</div>"
        "</div>"
    ).arg(ticket, dept, doctor, symptoms, statusColor, statusText, created)
     .arg(records.size());

    addSystemBubble(html);
}

// ============================================================
// 帮助
// ============================================================

void VoiceChatWidget::executeHelp()
{
    QString bgColor = m_isDarkMode ? "#252535" : "#f8f9fa";
    QString borderColor = m_isDarkMode ? "#3a4a6a" : "#d5e4ed";
    QString textColor = m_isDarkMode ? "#f0f4f8" : "#2c3e50";
    QString titleColor = m_isDarkMode ? "#ffffff" : "#2c5f8a";
    QString exampleColor = m_isDarkMode ? "#b0c0d0" : "#5f6b7a";
    QString dividerColor = m_isDarkMode ? "#3a4a5a" : "#e0e0e0";
    QString footerColor = m_isDarkMode ? "#7ab8d9" : "#7ab8d9";

    QString helpText = QString(
        "<div style='background-color: %1; border: 1px solid %2; border-radius: 12px; padding: 16px 20px; margin: 4px 0; line-height: 1.6;'>"
        "<div style='font-size: 18px; font-weight: bold; color: %3; margin-bottom: 8px;'>功能介绍</div>"
        "<div style='font-size: 14px; color: %4;'>"
        "你可以使用 <b>语音</b> 或 <b>文字</b> 交互。<br><br>"
        "<b>📋 挂号：</b> 预约挂号<br>"
        "<span style='padding-left: 20px; color: %5;'>例如：\"挂神经内科\"</span><br>"
        "<b>🔍 查询号源：</b> 查看可用号源<br>"
        "<span style='padding-left: 20px; color: %5;'>例如：\"某科室有没有号源\"</span><br>"
        "<b>📄 记录：</b> 查看历史挂号记录<br>"
        "<span style='padding-left: 20px; color: %5;'>例如：\"我的记录\"</span><br>"
        "<b>🔄 重置：</b> 重新开始<br>"
        "</div>"
        "<div style='margin-top: 10px; padding-top: 8px; border-top: 1px solid %6; color: %7; font-size: 13px;'>"
        "👇 按下麦克风按钮或在输入框中输入"
        "</div>"
        "</div>"
    ).arg(bgColor, borderColor, titleColor, textColor, exampleColor, dividerColor, footerColor);

    addSystemBubble(helpText);

    QString speechText =
        "帮助。你可以使用语音或文字交互。"
        "挂号，预约挂号，例如挂神经内科。"
        "查询号源，查看可用号源，例如有没有号源。"
        "记录，查看历史挂号记录，例如我的记录。"
        "重置，重新开始。";
    speakText(speechText);
    m_state = ChatState::IDLE;
}

// ============================================================
// 健康咨询
// ============================================================

void VoiceChatWidget::executeChat(const QString &input)
{
    addSystemBubble("正在思考...");

    QString backendUrl = Config::instance().getBackendUrl();
    QUrl url(backendUrl + "/api/chat");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["question"] = input;
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(obj).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg;
            switch (reply->error()) {
                case QNetworkReply::ConnectionRefusedError:
                    errorMsg = "连接被拒绝，请检查后端服务是否已启动（端口5000）。";
                    break;
                case QNetworkReply::HostNotFoundError:
                    errorMsg = "无法找到后端服务器，请确认IP地址和端口是否正确。";
                    break;
                case QNetworkReply::TimeoutError:
                    errorMsg = "请求超时，请检查网络连接或后端服务状态。";
                    break;
                default:
                    errorMsg = "网络错误：" + reply->errorString();
                    break;
            }
            addSystemBubble(errorMsg);
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            if (obj["success"].toBool()) {
                QString answer = obj["answer"].toString();
                addSystemBubble(answer);
                speakText(answer);
            } else {
                addSystemBubble("咨询失败：" + obj["message"].toString());
            }
        }
        reply->deleteLater();
    });
}

// ============================================================
// 科室列表
// ============================================================

void VoiceChatWidget::loadDepartments(std::function<void()> callback)
{
    QString backendUrl = Config::instance().getBackendUrl();
    QUrl url(backendUrl + "/api/departments");
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, callback]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.object()["success"].toBool()) {
                QJsonArray arr = doc.object()["departments"].toArray();
                m_departments.clear();
                for (auto val : arr) {
                    m_departments.append(val.toString());
                }
            }
        }
        if (m_departments.isEmpty()) {
            m_departments << "全科门诊" << "外科" << "内科" << "皮肤科" << "儿科" << "心血管内科";
        }
        reply->deleteLater();
        if (callback) callback();
    });
}

void VoiceChatWidget::showDepartmentCard()
{
    if (m_departments.isEmpty()) {
        addSystemBubble("暂无科室数据，请稍后再试");
        return;
    }
    QString deptList;
    for (int i = 0; i < m_departments.size(); ++i) {
        deptList += QString("%1. %2").arg(i+1).arg(m_departments[i]);
        if (i < m_departments.size()-1) deptList += "\n";
    }
    addCardBubble("🏥 可选科室", deptList);
}

// ============================================================
// TTS 语音输出
// ============================================================

void VoiceChatWidget::speakText(const QString &text)
{
    if (!m_voiceEnabled) return;

    QString plainText = text;
    plainText.replace(QRegularExpression("<[^>]*>"), "");
    voiceManager->speak(plainText);
}

// ============================================================
// 重置对话
// ============================================================

void VoiceChatWidget::resetConversation()
{
    m_state = ChatState::IDLE;

    if (chatList) {
        chatList->clear();
    }

    addSystemBubble("对话已重置，有什么可以帮助你的吗？");
    speakText("对话已重置，有什么可以帮助你的吗？");
}