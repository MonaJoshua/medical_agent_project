#ifndef MONITORDATAWIDGET_H
#define MONITORDATAWIDGET_H

#include "trendwidget.h"
#include <QWidget>
#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QPushButton>
#include <QPointer>

class MonitorDataWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorDataWidget(QWidget *parent = nullptr);
    void refreshData();

signals:
    void backToGrid();

private slots:
    void onTrendButtonClicked();   // 新增：趋势图按钮槽

private:
    void loadData();
    void updateTableStyle();
    void showTrendWindow(const QString &paramName, const QString &displayName);
    QString getButtonStyleSheet() const;
    
    QTableWidget *recordTable;
    QNetworkAccessManager *networkManager;
    QJsonArray m_allRecords;
    QPointer<TrendWidget> m_trendWindow;
};

#endif // MONITORDATAWIDGET_H