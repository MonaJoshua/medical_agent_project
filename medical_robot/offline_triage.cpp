#include "offline_triage.h"
#include <QDebug>

OfflineTriage& OfflineTriage::instance()
{
    static OfflineTriage instance;
    return instance;
}

OfflineTriage::OfflineTriage()
{
    initKeywordMap();
    
    // 预置典型病例（用于演示）
    m_sampleSymptoms = {
        "头痛头晕",
        "胸痛胸闷",
        "发烧咳嗽",
        "胃痛腹痛",
        "视力模糊"
    };
    
    m_sampleResults = {
        "🏥 建议就诊科室：神经内科\n💡 建议：注意休息，避免劳累，监测血压。",
        "🏥 建议就诊科室：心内科\n💡 建议：请立即就医，避免剧烈运动。",
        "🏥 建议就诊科室：呼吸内科\n💡 建议：多喝水，注意体温变化。",
        "🏥 建议就诊科室：消化内科\n💡 建议：饮食清淡，避免辛辣刺激。",
        "🏥 建议就诊科室：眼科\n💡 建议：减少用眼，避免长时间看屏幕。"
    };
}

void OfflineTriage::initKeywordMap()
{
    m_keywordMap["头疼"] = "神经内科";
    m_keywordMap["头痛"] = "神经内科";
    m_keywordMap["头晕"] = "神经内科";
    m_keywordMap["胸痛"] = "心内科";
    m_keywordMap["胸闷"] = "心内科";
    m_keywordMap["发烧"] = "呼吸内科";
    m_keywordMap["发热"] = "呼吸内科";
    m_keywordMap["咳嗽"] = "呼吸内科";
    m_keywordMap["胃痛"] = "消化内科";
    m_keywordMap["腹痛"] = "消化内科";
    m_keywordMap["牙痛"] = "口腔科";
    m_keywordMap["视力"] = "眼科";
    m_keywordMap["皮疹"] = "皮肤科";
    m_keywordMap["关节"] = "骨科";
}

QString OfflineTriage::triage(const QString& symptoms)
{
    for (auto it = m_keywordMap.begin(); it != m_keywordMap.end(); ++it) {
        if (symptoms.contains(it.key())) {
            return QString("🏥 建议就诊科室：%1\n💡 建议：请及时就医，详细描述症状。").arg(it.value());
        }
    }
    return "🏥 建议就诊科室：全科门诊\n💡 建议：请详细描述症状以便更准确分诊。";
}

QStringList OfflineTriage::getSampleSymptoms() const
{
    return m_sampleSymptoms;
}

QStringList OfflineTriage::getSampleResult(int index) const
{
    if (index >= 0 && index < m_sampleResults.size()) {
        return {m_sampleResults[index]};
    }
    return {m_sampleResults[0]};
}