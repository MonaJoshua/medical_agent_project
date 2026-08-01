#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QPushButton> 
#include <QButtonGroup>
#include <QGridLayout>
#include <QNetworkAccessManager>
#include <QTextBrowser>

class SmartTriage : public QWidget
{
    Q_OBJECT
public:
    explicit SmartTriage(QWidget *parent = nullptr);
    ~SmartTriage();

private slots:
    void onDeptButtonClicked(QAbstractButton *button);
    void onRegisterClicked();
    void onReplyFinished(QNetworkReply *reply);
    void onDepartmentRefreshClicked();

private:
    void loadDepartments();
    void populateDepartmentButtons();
    QString buildSuccessHtml(const QString &dept, const QString &ticketId, int queueNum);
    QString buildErrorHtml(const QString &error);

    QNetworkAccessManager *networkManager;
    QTextBrowser *resultBrowser;
    QWidget *deptButtonContainer;
    QGridLayout *deptGridLayout;
    QButtonGroup *deptButtonGroup;
    QPushButton *registerBtn;

    QStringList m_departments;
    QString m_selectedDept;
    bool isRefreshing;
};

#endif // SMARTTRIAGE_H