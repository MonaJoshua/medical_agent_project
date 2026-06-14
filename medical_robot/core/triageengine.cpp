#include "triageengine.h"
#include "offline_triage.h"
#include "network/apiclient.h"
#include <QDebug>

TriageEngine& TriageEngine::instance()
{
    static TriageEngine instance;
    return instance;
}

void TriageEngine::setProductionMode(bool isProduction)
{
    m_isProduction = isProduction;
    qDebug() << "分诊引擎模式:" << (m_isProduction ? "生产模式" : "演示模式");
}

TriageResult TriageEngine::triageSync(const QString& symptoms)
{
    return processWithOffline(symptoms);
}

void TriageEngine::triageAsync(const QString& symptoms, 
                                std::function<void(const TriageResult&)> callback)
{
    if (m_isProduction) {
        processWithAI(symptoms, callback);
    } else {
        TriageResult result = processWithOffline(symptoms);
        if (callback) callback(result);
    }
}

QString TriageEngine::extractDepartment(const QString& rawResult)
{
    if (rawResult.contains("神经内科")) return "神经内科";
    if (rawResult.contains("呼吸内科")) return "呼吸内科";
    if (rawResult.contains("消化内科")) return "消化内科";
    if (rawResult.contains("眼科")) return "眼科";
    if (rawResult.contains("心内科")) return "心内科";
    if (rawResult.contains("内科")) return "内科";
    return "全科门诊";
}

TriageResult TriageEngine::parseResult(const QString& resultStr)
{
    TriageResult result;
    result.rawResult = resultStr;
    result.department = extractDepartment(resultStr);
    result.suggestion = resultStr;
    result.isSuccess = true;
    return result;
}

TriageResult TriageEngine::processWithOffline(const QString& symptoms)
{
    QString rawResult = OfflineTriage::instance().triage(symptoms);
    QString department = extractDepartment(rawResult);
    return TriageResult(department, rawResult, rawResult);
}

void TriageEngine::processWithAI(const QString& symptoms,
                                  std::function<void(const TriageResult&)> callback)
{
    ApiClient::instance().triage(symptoms, [this, symptoms, callback](bool success, const QString& result) {
        if (success) {
            TriageResult triageResult = parseResult(result);
            if (callback) callback(triageResult);
        } else {
            qDebug() << "⚠️ AI 分诊失败 (" << result << ")，切换到演示模式";
            auto offlineResult = processWithOffline(symptoms);
            if (callback) callback(offlineResult);
        }
    });
}
