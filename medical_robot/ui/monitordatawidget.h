#ifndef MONITORDATAWIDGET_H
#define MONITORDATAWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPointer>

class TrendWidget;

class MonitorDataWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorDataWidget(QWidget *parent = nullptr);
    void refreshData();

signals:
    void backToGrid();

private slots:
    void onTrendButtonClicked();

private:
    void loadData();
    void updateTableStyle();
    void showTrendWindow(const QString &paramName, const QString &displayName);
    QString getButtonStyleSheet() const;

    void showError(const QString &msg);
    void showMessage(const QString &msg);
    QTableWidgetItem* makeTableItem(const QString &text);

    QTableWidget *recordTable;
    QPointer<TrendWidget> m_trendWindow;
};

#endif // MONITORDATAWIDGET_H