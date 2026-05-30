#ifndef OFFLINE_TRIAGE_H
#define OFFLINE_TRIAGE_H

#include <QString>
#include <QMap>

class OfflineTriage
{
public:
    static OfflineTriage& instance();
    
    // 离线分诊（关键词匹配）
    QString triage(const QString& symptoms);
    
    // 获取预置的典型病例（用于演示）
    QStringList getSampleSymptoms() const;
    QString getSampleResult(int index) const;
    
private:
    OfflineTriage();
    void initKeywordMap();
    
    QMap<QString, QString> m_keywordMap;  // 关键词 -> 科室建议
    QStringList m_sampleSymptoms;
    QStringList m_sampleResults;
};

#endif // OFFLINE_TRIAGE_H