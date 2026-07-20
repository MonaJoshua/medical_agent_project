#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QString>
#include <QDateTime>

struct TriageResult {
    QString department;
    QString suggestion;
    QString rawResult;
    bool isSuccess;
    
    TriageResult() : isSuccess(false) {}
    TriageResult(const QString& dept, const QString& sugg, const QString& raw)
        : department(dept), suggestion(sugg), rawResult(raw), isSuccess(true) {}
};

struct RegistrationInfo {
    QString ticketId;
    QString department;
    int queueNumber;
    QString patientName;
    QString symptoms;
    QDateTime timestamp;
    
    RegistrationInfo() : queueNumber(0) {}
};

#endif // DATAMODEL_H
