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
    void refreshRecords();

signals:
    void backToGrid();

private slots:
    void resetFilter();
    void onDateChanged(const QDate &date);

private:
    void loadRecords();
    void fetchWithCurl();
    void displayRecords(const QJsonArray &records);
    void applyFilter();
    void showError(const QString &msg);
    void showRecordDetail(const QString &ticketId);
    void updateTableStyle();
    QTableWidgetItem* makeTableItem(const QString &text);

    void resizeEvent(QResizeEvent *event) override;

    QTableWidget *recordTable;
    QNetworkAccessManager *networkManager;
    QDateEdit *dateEdit;
    QPushButton *backBtn;
    QPushButton *resetBtn;
    QPushButton *refreshBtn;
    QTimer *resizeTimer;

    QJsonArray m_allRecords;
    bool m_dateFilterEnabled;
    bool isProcessing;
};

#endif // REGISTRATIONRECORDSWIDGET_H