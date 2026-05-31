#ifndef OFFLINE_TRIAGE_H
#define OFFLINE_TRIAGE_H

#include <QString>
#include <QMap>

class OfflineTriage
{
public:
    static OfflineTriage& instance();
    QString triage(const QString& symptoms);

private:
    OfflineTriage();
    void initKeywordMap();
    QMap<QString, QString> keywordMap;
};

#endif // OFFLINE_TRIAGE_H
