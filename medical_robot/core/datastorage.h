#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>

struct VitalSigns;

class DataStorage : public QObject
{
    Q_OBJECT
public:
    static DataStorage& instance();

    bool initDatabase(const QString &dbPath = "medical_data.db");
    void saveData(const VitalSigns &data);
    void saveDataBatch(const QList<VitalSigns> &dataList);

    QList<VitalSigns> fetchData(qint64 startTime, qint64 endTime);
    qint64 getEarliestTimestamp();
    void cleanOldData(int daysToKeep = 7);

private:
    DataStorage(QObject *parent = nullptr);
    ~DataStorage();

    QSqlDatabase m_db;
    bool m_initialized;
};

#endif // DATASTORAGE_H