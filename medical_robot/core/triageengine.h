#ifndef TRIAGEENGINE_H
#define TRIAGEENGINE_H

#include <QObject>
#include <functional>
#include "datamodel.h"

class TriageEngine : public QObject
{
    Q_OBJECT
public:
    static TriageEngine& instance();
    
    TriageResult triageSync(const QString& symptoms);
    void triageAsync(const QString& symptoms, 
                     std::function<void(const TriageResult&)> callback);
    
    void setProductionMode(bool isProduction);
    
private:
    TriageEngine() = default;
    
    TriageResult processWithOffline(const QString& symptoms);
    void processWithAI(const QString& symptoms, 
                       std::function<void(const TriageResult&)> callback);
    
    // 辅助函数
    QString extractDepartment(const QString& rawResult);
    TriageResult parseResult(const QString& resultStr);
    
    bool m_isProduction = false;
};

#endif // TRIAGEENGINE_H
