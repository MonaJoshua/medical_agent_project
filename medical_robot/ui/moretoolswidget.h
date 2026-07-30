#ifndef MORETOOLSWIDGET_H
#define MORETOOLSWIDGET_H

#include <QWidget>
#include <QStackedWidget>

class RegistrationRecordsWidget;
class MonitorDataWidget;

class MoreToolsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MoreToolsWidget(QWidget *parent = nullptr);
    void showRecordsPage();
    void showMonitorData();

private slots:
    void showGridPage();

private:
    void createGridPage();

    QStackedWidget *stackedWidget;
    QWidget *gridPage;
    RegistrationRecordsWidget *recordsPage;
    MonitorDataWidget *m_monitorDataPage;
};

#endif // MORETOOLSWIDGET_H