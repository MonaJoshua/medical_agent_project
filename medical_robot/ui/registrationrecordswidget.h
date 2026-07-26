#ifndef REGISTRATIONRECORDSWIDGET_H
#define REGISTRATIONRECORDSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QTimer>

class RegistrationRecordsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RegistrationRecordsWidget(QWidget *parent = nullptr);

signals:
    void backToGrid();   // 点击“返回”时通知父窗口切换到网格页面

private slots:
    void applyFilter();
    void resetFilter();
    void onDateChanged(const QDate &date);
    void refreshRecords();

private:
    void loadRecords();
    void displayRecords(const QJsonArray &records);
    void showRecordDetail(const QString &ticketId);
    void updateTableStyle();
    void showError(const QString &msg);
    void resizeEvent(QResizeEvent *event) override;

    QTableWidget *recordTable;
    QDateEdit *dateEdit;
    QPushButton *resetBtn;
    QPushButton *refreshBtn;
    QPushButton *backBtn;

    QNetworkAccessManager *networkManager;
    QJsonArray m_allRecords;
    bool m_dateFilterEnabled;
    bool isProcessing;
    QTimer *resizeTimer;
};

#endif // REGISTRATIONRECORDSWIDGET_H