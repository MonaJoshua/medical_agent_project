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
}

void OfflineTriage::initKeywordMap()
{
    keywordMap["头疼"] = "神经内科";
    keywordMap["头痛"] = "神经内科";
    keywordMap["头晕"] = "神经内科";
    keywordMap["胸痛"] = "心内科";
    keywordMap["胸闷"] = "心内科";
    keywordMap["咳嗽"] = "呼吸内科";
    keywordMap["发烧"] = "呼吸内科";
    keywordMap["发热"] = "呼吸内科";
    keywordMap["胃痛"] = "消化内科";
    keywordMap["腹痛"] = "消化内科";
    keywordMap["牙痛"] = "口腔科";
    keywordMap["眼睛"] = "眼科";
    keywordMap["视力"] = "眼科";
    keywordMap["皮疹"] = "皮肤科";
    keywordMap["关节"] = "骨科";
    keywordMap["腰痛"] = "骨科";
}

QString OfflineTriage::triage(const QString& symptoms)
{
    for (auto it = keywordMap.begin(); it != keywordMap.end(); ++it) {
        if (symptoms.contains(it.key())) {
            return QString("🏥 建议就诊科室：%1\n💡 建议：请及时就医，详细描述症状。").arg(it.value());
        }
    }
    return "🏥 建议就诊科室：全科门诊\n💡 建议：请详细描述症状以便更准确分诊。";
}
