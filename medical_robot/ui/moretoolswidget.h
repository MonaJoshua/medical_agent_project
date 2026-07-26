#ifndef MORETOOLSWIDGET_H
#define MORETOOLSWIDGET_H

#include <QWidget>
#include <QStackedWidget>

class RegistrationRecordsWidget;

class MoreToolsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MoreToolsWidget(QWidget *parent = nullptr);
    void showRecordsPage();
    
private slots:
    void showGridPage();

private:
    void createGridPage();

    QStackedWidget *stackedWidget;
    QWidget *gridPage;
    RegistrationRecordsWidget *recordsPage;
};

#endif // MORETOOLSWIDGET_H