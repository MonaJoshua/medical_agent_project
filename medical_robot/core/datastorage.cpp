#include "datastorage.h"
#include "datamanager.h" 
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDir>

DataStorage& DataStorage::instance()
{
    static DataStorage instance;
    return instance;
}

DataStorage::DataStorage(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

DataStorage::~DataStorage()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DataStorage::initDatabase(const QString &dbPath)
{
    if (m_initialized) return true;

    QFileInfo info(dbPath);
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "[DataStorage] 无法打开数据库:" << m_db.lastError().text();
        return false;
    }

    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS vital_signs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            heart_rate REAL,
            spo2 REAL,
            temperature REAL
        );
        CREATE INDEX IF NOT EXISTS idx_timestamp ON vital_signs(timestamp);
    )";

    QSqlQuery query;
    if (!query.exec(createTable)) {
        qDebug() << "[DataStorage] 创建表失败:" << query.lastError().text();
        return false;
    }

    m_initialized = true;
    qDebug() << "[DataStorage] 数据库初始化成功:" << dbPath;
    return true;
}

void DataStorage::saveData(const VitalSigns &data)
{
    if (!m_initialized) return;

    QSqlQuery query;
    query.prepare("INSERT INTO vital_signs (timestamp, heart_rate, spo2, temperature) "
                  "VALUES (:timestamp, :heart_rate, :spo2, :temperature)");
    query.bindValue(":timestamp", data.timestamp);
    query.bindValue(":heart_rate", data.heartRate);
    query.bindValue(":spo2", data.spo2);
    query.bindValue(":temperature", data.temperature);

    if (!query.exec()) {
        qDebug() << "[DataStorage] 插入数据失败:" << query.lastError().text();
    }
}

void DataStorage::saveDataBatch(const QList<VitalSigns> &dataList)
{
    if (dataList.isEmpty() || !m_initialized) return;

    QSqlQuery query;
    query.prepare("INSERT INTO vital_signs (timestamp, heart_rate, spo2, temperature) "
                  "VALUES (:timestamp, :heart_rate, :spo2, :temperature)");

    m_db.transaction();
    bool hasError = false;

    for (const VitalSigns &data : dataList) {
        query.bindValue(":timestamp", data.timestamp);
        query.bindValue(":heart_rate", data.heartRate);
        query.bindValue(":spo2", data.spo2);
        query.bindValue(":temperature", data.temperature);

        if (!query.exec()) {
            qDebug() << "[DataStorage] 批量插入失败:" << query.lastError().text();
            hasError = true;
            break;
        }
    }

    if (hasError) {
        m_db.rollback();
        qDebug() << "[DataStorage] 批量插入已回滚";
    } else {
        m_db.commit();
    }
}

QList<VitalSigns> DataStorage::fetchData(qint64 startTime, qint64 endTime)
{
    QList<VitalSigns> list;
    if (!m_initialized) return list;

    QSqlQuery query;
    query.prepare("SELECT timestamp, heart_rate, spo2, temperature FROM vital_signs "
                  "WHERE timestamp >= :start AND timestamp <= :end ORDER BY timestamp ASC");
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);

    if (!query.exec()) {
        qDebug() << "[DataStorage] 查询数据失败:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        VitalSigns data;
        data.timestamp = query.value(0).toLongLong();
        data.heartRate = query.value(1).toDouble();
        data.spo2 = query.value(2).toDouble();
        data.temperature = query.value(3).toDouble();
        list.append(data);
    }
    return list;
}

qint64 DataStorage::getEarliestTimestamp()
{
    if (!m_initialized) return 0;

    QSqlQuery query("SELECT MIN(timestamp) FROM vital_signs");
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

void DataStorage::cleanOldData(int daysToKeep)
{
    if (!m_initialized) return;

    qint64 cutoff = QDateTime::currentDateTime().addDays(-daysToKeep).toMSecsSinceEpoch();
    QSqlQuery query;
    query.prepare("DELETE FROM vital_signs WHERE timestamp < :cutoff");
    query.bindValue(":cutoff", cutoff);

    if (!query.exec()) {
        qDebug() << "[DataStorage] 清理旧数据失败:" << query.lastError().text();
        return;
    }

    int deleted = query.numRowsAffected();
    qDebug() << "[DataStorage] 清理了" << deleted << "条旧数据";

    // VACUUM 释放空间（静默执行，忽略错误）
    query.exec("VACUUM");
}