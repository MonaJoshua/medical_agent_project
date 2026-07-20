#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QComboBox>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QIODevice>

class VoiceManager;

class SmartTriage : public QWidget
{
    Q_OBJECT

public:
    explicit SmartTriage(QWidget *parent = nullptr);
    ~SmartTriage();

    void setMultiRoundMode(bool enable);   // 设置分诊模式

private slots:
    // ===== 核心功能 =====
    void onAnalyzeClicked();
    void onRegisterClicked();
    void onReplyFinished(QNetworkReply *reply);
    void onVoiceInputClicked();
    void onVoiceRecognized(const QString &text);

    // ===== 科室管理 =====
    void loadDepartments();
    void onDepartmentRefreshClicked();
    void populateDepartmentCombo();

    // ===== 语音交互 =====
    void startRecording();
    void stopRecording();
    void processAudioData();

    void resetTriage();   // 重置状态机

private:
    // ===== UI 控件 =====
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
    QComboBox *departmentCombo;
    QPushButton *voiceBtn;          // 语音输入按钮
    QNetworkAccessManager *networkManager;
    VoiceManager *voiceManager;
    QStringList m_departments;
    QString currentTriageResult;

    // ===== 多轮预诊状态机 =====
    enum TriageState {
        STATE_IDLE,            // 空闲状态
        STATE_ASK_SYMPTOM,     // 询问症状
        STATE_ASK_DURATION,    // 询问持续时间
        STATE_ASK_HISTORY,     // 询问既往病史
        STATE_SUMMARY_DONE     // 摘要生成完成
    };

    TriageState m_state;
    QString m_collectedSymptoms;
    QString m_collectedDuration;
    QString m_collectedHistory;
    bool m_isMultiRoundMode;   // true=多轮预诊，false=快速分诊

    void generatePreDiagnosis();   // 生成预诊摘要

    // ===== 本地语音录音 =====
    QAudioInput *m_audioInput;
    QIODevice *m_audioDevice;
    QByteArray m_audioBuffer;
    bool m_isRecording;

    void recognizeAudio();         // 识别录音
};

#endif // SMARTTRIAGE_H