#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QTextBrowser>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QComboBox>

class SmartTriage : public QWidget
{
    Q_OBJECT
public:
    explicit SmartTriage(QWidget *parent = nullptr);
    ~SmartTriage();

private slots:
    void onDepartmentRefreshClicked();
    void onRegisterClicked();
    void onDeptButtonClicked(QAbstractButton *button);
    void onReplyFinished(QNetworkReply *reply);

private:
    void loadDepartments();
    void populateDepartmentButtons();
    bool isRefreshing;

    QNetworkAccessManager *networkManager;
    QTextBrowser *resultBrowser;
    QPushButton *registerBtn;
    QStringList m_departments;
    QString m_selectedDept;

    QWidget *deptButtonContainer;
    QGridLayout *deptGridLayout;
    QButtonGroup *deptButtonGroup;
    QPushButton *refreshDeptBtn; 
};

#endif // SMARTTRIAGE_H