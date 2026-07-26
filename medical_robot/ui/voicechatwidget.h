#ifndef VOICECHATWIDGET_H
#define VOICECHATWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAudioInput>
#include <QIODevice>
#include <QDateTime>
#include <QJsonObject>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>

class VoiceManager;

enum class ChatMessageType {
    User,
    System,
    Card
};

struct ChatMessage {
    ChatMessageType type;
    QString text;
    QString detail;
    QDateTime timestamp;
    ChatMessage() : type(ChatMessageType::System) {}
    ChatMessage(ChatMessageType t, const QString &txt)
        : type(t), text(txt), timestamp(QDateTime::currentDateTime()) {}
    ChatMessage(ChatMessageType t, const QString &txt, const QString &det)
        : type(t), text(txt), detail(det), timestamp(QDateTime::currentDateTime()) {}
};

enum class ChatState {
    IDLE,
    COLLECTING_SYMPTOM,
    COLLECTING_DEPT,
    TRIAGING,
    REGISTERING,
    QUERYING_SLOTS,
    QUERYING_RECORDS,
    CONFIRM_REGISTER,   // 等待用户确认挂号
    WAITING_DEPT        // 等待用户说出科室名
};

class VoiceChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VoiceChatWidget(QWidget *parent = nullptr);
    ~VoiceChatWidget();
    void appendSystemMessage(const QString &text);
    void appendCardMessage(const QString &title, const QString &detail);
    void setOnlineMode(bool online);  // 供外部切换
    void setDarkMode(bool dark);   // 切换深色模式

signals:
    void requestMonitorPage();
    void requestSwitchToMoreTools();
    
private slots:
    void onVoiceBtnPressed();
    void onVoiceBtnReleased();
    void onVoiceRecognized(const QString &text);
    void onVoiceError(const QString &error);
    void onSendTextClicked();
    void onTextReturnPressed();
    void onHelpClicked();
    void onResetClicked();

private:
    void setupStyles();
    void setupUI();
    void addChatMessage(const ChatMessage &msg);
    void addUserBubble(const QString &text);
    void addSystemBubble(const QString &text);
    void addCardBubble(const QString &title, const QString &detail);
    QString buildChatHtml(const ChatMessage &msg);
    void startRecording();
    void stopRecording();
    void processAudioData();
    void processUserInput(const QString &input);
    QString detectIntent(const QString &input);
    void executeTriage(const QString &input);
    void executeRegister(const QString &input);
    void executeQuerySlots(const QString &department);
    void executeQueryRecords();
    void executeHelp();
    void speakText(const QString &text);
    void resetConversation();
    void executeChat(const QString &input); 
    void doRegister(const QString &department); 
    void showDepartmentCard();
    void loadDepartments(std::function<void()> callback = nullptr);
    QString extractDepartment(const QString& raw);
    void onLinkActivated(const QString &link);
    void displayRecords(const QJsonArray &records);
    void displayLatestRecord(const QJsonArray &records);

    QListWidget *chatList;

    // 控件
    QPushButton *voiceBtn;
    QPushButton *helpBtn;
    QPushButton *resetBtn;
    QLineEdit *textInput;
    QPushButton *sendBtn;

    VoiceManager *voiceManager;
    QAudioInput *m_audioInput;
    QIODevice *m_audioDevice;
    QByteArray m_audioBuffer;
    bool m_isRecording;
    QNetworkAccessManager *m_networkManager;
    ChatState m_state;
    QString m_collectedSymptom;
    QString m_collectedDepartment;
    QString m_lastTriageResult;
    QString m_patientName;
    QStringList m_departments;
    QString m_lastSymptom;   // 存储最近一次分诊的用户输入
    QString m_lastSymptomSummary;   // 存储AI摘要
    bool m_welcomeShown = false;
    QStringList m_availableDepts;  // 动态科室列表
    bool m_isOnline;   // true=在线模式, false=离线模式
    QString m_lastQueryDept;   //保存最近查询的科室
    bool m_isDarkMode = false;     // 当前是否为深色模式

};

#endif