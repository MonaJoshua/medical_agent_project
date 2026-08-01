#ifndef VOICECHATWIDGET_H
#define VOICECHATWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QProcess>
#include <functional>

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

    ChatMessage() : type(ChatMessageType::System) {}
    ChatMessage(ChatMessageType t, const QString &txt)
        : type(t), text(txt) {}
    ChatMessage(ChatMessageType t, const QString &txt, const QString &det)
        : type(t), text(txt), detail(det) {}
};

enum class ChatState {
    IDLE,
    REGISTERING,
    QUERYING_SLOTS,
    QUERYING_RECORDS,
    CONFIRM_REGISTER,
    WAITING_DEPT,
    COLLECTING_DEPT
};

class VoiceChatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VoiceChatWidget(QWidget *parent = nullptr);
    ~VoiceChatWidget();

    void appendSystemMessage(const QString &text);
    void appendCardMessage(const QString &title, const QString &detail);
    void setDarkMode(bool dark);

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
    void onRecordingFinished(int exitCode, QProcess::ExitStatus status);
    void onVoiceToggleClicked();
    void onLinkActivated(const QString &link);

private:
    void setupUI();
    void addChatMessage(const ChatMessage &msg);
    void addUserBubble(const QString &text);
    void addSystemBubble(const QString &text);
    void addCardBubble(const QString &title, const QString &detail);
    QString buildChatHtml(const ChatMessage &msg);
    void startRecording();
    void stopRecording();
    void processUserInput(const QString &input);
    QString detectIntent(const QString &input);
    void executeQuerySlots(const QString &department);
    void executeQueryRecords();
    void displayRecords(const QJsonArray &records);
    void displayLatestRecord(const QJsonArray &records);
    void executeHelp();
    void executeChat(const QString &input);
    void doRegister(const QString &department);
    QString buildRegisterSuccessHtml(const QString &dept, const QString &ticketId, int queueNum);
    void showDepartmentCard();
    void loadDepartments(std::function<void()> callback = nullptr);
    void speakText(const QString &text);
    void resetConversation();
    void updateVoiceUI();

    // UI 控件
    QListWidget *chatList;
    QPushButton *voiceBtn;
    QPushButton *helpBtn;
    QPushButton *resetBtn;
    QPushButton *m_voiceToggleBtn;
    QLineEdit *textInput;
    QPushButton *sendBtn;

    // 核心成员
    VoiceManager *voiceManager;
    QNetworkAccessManager *m_networkManager;
    ChatState m_state;
    QStringList m_departments;
    QString m_lastSymptomSummary;
    QString m_lastQueryDept;
    bool m_isDarkMode;
    bool m_voiceEnabled;

    // 录音相关
    QString m_tempAudioFile;
    QProcess *m_recordingProcess;
    bool m_isRecording;
};

#endif // VOICECHATWIDGET_H