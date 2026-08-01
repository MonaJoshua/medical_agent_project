// registrationrecordswidget.cpp
// 挂号记录列表组件

#include "registrationrecordswidget.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <algorithm>

class TableUpdateGuard {
public:
    TableUpdateGuard(QTableWidget *table) : m_table(table) {
        if (m_table) m_table->setUpdatesEnabled(false);
    }
    ~TableUpdateGuard() {
        if (m_table) m_table->setUpdatesEnabled(true);
    }
private:
    QTableWidget *m_table;
};

RegistrationRecordsWidget::RegistrationRecordsWidget(QWidget *parent)
    : QWidget(parent)
    , networkManager(new QNetworkAccessManager(this))
    , m_dateFilterEnabled(false)
    , isProcessing(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 头部布局
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(12);

    QString baseBtnStyle = R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid;
            border-radius: 6px;
            padding: 6px 16px;
            font-size: 14px;
            font-weight: 500;
            min-width: 60px;
        }
    )";

    backBtn = new QPushButton("返回");
    backBtn->setFixedHeight(36);
    backBtn->setStyleSheet(baseBtnStyle);
    connect(backBtn, &QPushButton::clicked, this, &RegistrationRecordsWidget::backToGrid);
    headerLayout->addWidget(backBtn);

    headerLayout->addStretch();

    QLabel *recordsTitle = new QLabel("挂号记录");
    recordsTitle->setFont(QFont("", 18, QFont::Bold));
    headerLayout->addWidget(recordsTitle);

    headerLayout->addStretch();

    dateEdit = new QDateEdit();
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setFont(QFont("", 14));
    dateEdit->setSpecialValueText("选择日期");
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setFixedHeight(36);

    QCalendarWidget *calendar = dateEdit->calendarWidget();
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar->setStyleSheet(
        "QCalendarWidget { background-color: #ffffff; border: 1px solid #d5e4ed; border-radius: 6px; }"
        "QCalendarWidget QToolButton { color: #1a73e8; font-weight: bold; font-size: 13px; }"
        "QCalendarWidget QSpinBox { color: #1a73e8; font-size: 12px; }"
        "QCalendarWidget QTableView { background-color: #ffffff; }"
        "QCalendarWidget QTableView::item { padding: 4px; }"
        "QCalendarWidget QTableView::item:selected { background-color: #1a73e8; color: white; }"
        "QCalendarWidget QHeaderView::section { background-color: #e8edf2; color: #2c3e50; padding: 2px; font-weight: bold; }"
    );
    connect(dateEdit, &QDateEdit::dateChanged, this, &RegistrationRecordsWidget::onDateChanged);
    headerLayout->addWidget(dateEdit);

    resetBtn = new QPushButton("重置");
    resetBtn->setFixedHeight(36);
    resetBtn->setStyleSheet(baseBtnStyle);
    connect(resetBtn, &QPushButton::clicked, this, &RegistrationRecordsWidget::resetFilter);
    headerLayout->addWidget(resetBtn);

    refreshBtn = new QPushButton("刷新");
    refreshBtn->setFixedHeight(36);
    refreshBtn->setStyleSheet(baseBtnStyle);
    connect(refreshBtn, &QPushButton::clicked, this, &RegistrationRecordsWidget::refreshRecords);
    headerLayout->addWidget(refreshBtn);

    mainLayout->addLayout(headerLayout);

    // 防抖定时器（窗口大小变化时延迟刷新列宽）
    resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &RegistrationRecordsWidget::applyFilter);

    // 表格
    recordTable = new QTableWidget(this);
    recordTable->setColumnCount(5);
    recordTable->setHorizontalHeaderLabels({"单号", "科室", "日期", "状态", "操作"});
    recordTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    recordTable->horizontalHeader()->setFont(QFont("", 14, QFont::Bold));
    recordTable->verticalHeader()->setVisible(false);
    recordTable->setSelectionMode(QAbstractItemView::NoSelection);
    recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recordTable->setShowGrid(false);
    recordTable->setFont(QFont("", 14));
    recordTable->setMinimumHeight(400);
    recordTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(recordTable);

    loadRecords();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this]() {
                updateTableStyle();
                applyFilter();
            });
}

void RegistrationRecordsWidget::loadRecords()
{
    if (isProcessing) return;
    isProcessing = true;
    if (refreshBtn) refreshBtn->setEnabled(false);

    QUrl url("http://127.0.0.1:5000/api/registrations");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, [this, reply, timeoutTimer]() {
        if (reply && reply->isRunning()) reply->abort();
        fetchWithCurl();
        timeoutTimer->deleteLater();
    });
    timeoutTimer->start(10000);

    connect(reply, &QNetworkReply::finished, [this, reply, timeoutTimer]() {
        timeoutTimer->stop();
        timeoutTimer->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            fetchWithCurl();
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj["success"].toBool()) {
                    displayRecords(obj["data"].toArray());
                } else {
                    showError("获取记录失败：" + obj["message"].toString());
                }
            } else {
                showError("返回数据格式错误");
            }
        }
        reply->deleteLater();
        if (refreshBtn) refreshBtn->setEnabled(true);
        isProcessing = false;
    });
}

void RegistrationRecordsWidget::fetchWithCurl()
{
    QProcess *curlProcess = new QProcess(this);
    connect(curlProcess, &QProcess::finished, [this, curlProcess](int exitCode, QProcess::ExitStatus status) {
        if (exitCode == 0 && status == QProcess::NormalExit) {
            QByteArray data = curlProcess->readAllStandardOutput();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj["success"].toBool()) {
                    displayRecords(obj["data"].toArray());
                } else {
                    showError("获取记录失败：" + obj["message"].toString());
                }
            } else {
                showError("返回数据格式错误");
            }
        } else {
            showError("网络请求失败，请检查后端服务");
        }
        curlProcess->deleteLater();
        if (refreshBtn) refreshBtn->setEnabled(true);
        isProcessing = false;
    });
    curlProcess->start("curl", QStringList() << "-s" << "http://127.0.0.1:5000/api/registrations");
}

void RegistrationRecordsWidget::showError(const QString &msg)
{
    recordTable->setRowCount(1);
    recordTable->setColumnCount(1);
    recordTable->setHorizontalHeaderLabels({"提示"});
    recordTable->setItem(0, 0, new QTableWidgetItem(msg));
    recordTable->item(0, 0)->setTextAlignment(Qt::AlignCenter);
    recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void RegistrationRecordsWidget::displayRecords(const QJsonArray &records)
{
    m_allRecords = records;
    applyFilter();
}

void RegistrationRecordsWidget::resetFilter()
{
    m_dateFilterEnabled = false;
    dateEdit->setDate(QDate());
    dateEdit->setSpecialValueText("选择日期");
    dateEdit->update();
    applyFilter();
}

void RegistrationRecordsWidget::onDateChanged(const QDate &date)
{
    Q_UNUSED(date);
    static QTimer *debounceTimer = nullptr;
    if (!debounceTimer) {
        debounceTimer = new QTimer(this);
        debounceTimer->setSingleShot(true);
        connect(debounceTimer, &QTimer::timeout, [this]() {
            m_dateFilterEnabled = dateEdit->date().isValid();
            applyFilter();
        });
    }
    debounceTimer->start(150);
}

void RegistrationRecordsWidget::applyFilter()
{
    if (!recordTable || !dateEdit) return;

    recordTable->setFont(QFont("", 16));
    recordTable->horizontalHeader()->setFont(QFont("", 16, QFont::Bold));

    TableUpdateGuard guard(recordTable);

    bool hasDate = m_dateFilterEnabled && dateEdit->date().isValid();
    QDate selectedDate = dateEdit->date();

    QJsonArray filtered;
    if (!hasDate) {
        int count = qMin(m_allRecords.size(), 100);
        for (int i = 0; i < count; ++i) filtered.append(m_allRecords[i]);
    } else {
        for (const auto &val : m_allRecords) {
            QJsonObject obj = val.toObject();
            if (QDate::fromString(obj["date"].toString(), "yyyy-MM-dd") == selectedDate) {
                filtered.append(obj);
            }
        }
    }

    // 按日期降序排序
    QList<QJsonValue> list;
    for (int i = 0; i < filtered.size(); ++i) list.append(filtered[i]);
    std::sort(list.begin(), list.end(), [](const QJsonValue &a, const QJsonValue &b) {
        return a.toObject()["date"].toString() > b.toObject()["date"].toString();
    });
    filtered = QJsonArray();
    for (const auto &v : list) filtered.append(v);

    recordTable->clearContents();
    recordTable->setRowCount(0);
    recordTable->setColumnCount(5);
    recordTable->setHorizontalHeaderLabels({"单号", "科室", "日期", "状态", "操作"});
    recordTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    recordTable->horizontalHeader()->setFont(QFont("", 16, QFont::Bold));

    if (filtered.isEmpty()) {
        recordTable->setRowCount(1);
        recordTable->setItem(0, 0, new QTableWidgetItem("暂无匹配的记录"));
        recordTable->setSpan(0, 0, 1, 5);
        for (int col = 1; col < 5; ++col) recordTable->setItem(0, col, nullptr);
        recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int col = 1; col < 5; ++col) {
            recordTable->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
        updateTableStyle();
        return;
    }

    recordTable->setRowCount(filtered.size());
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);

    for (int row = 0; row < filtered.size(); ++row) {
        QJsonObject obj = filtered[row].toObject();
        QString ticket = obj["ticket_id"].toString();
        QString dept = obj["department"].toString();
        QString date = obj["date"].toString();
        QString status = obj["status"].toString();

        QString statusText, statusColor, statusBg;
        if (status == "waiting") {
            statusText = "待就诊"; statusColor = "#d93025"; statusBg = isDark ? "#4a2a2a" : "#fce8e6";
        } else if (status == "completed") {
            statusText = "已完成"; statusColor = "#34a853"; statusBg = isDark ? "#1a3a2a" : "#e6f4ea";
        } else if (status == "cancelled") {
            statusText = "已取消"; statusColor = "#8899aa"; statusBg = isDark ? "#3a3a3a" : "#f1f3f4";
        } else {
            statusText = status; statusColor = "#8899aa"; statusBg = isDark ? "#3a3a3a" : "#f1f3f4";
        }

        recordTable->setItem(row, 0, makeTableItem(ticket));
        recordTable->setItem(row, 1, makeTableItem(dept));
        recordTable->setItem(row, 2, makeTableItem(date));

        // 状态标签
        QWidget *statusWidget = new QWidget();
        QHBoxLayout *statusLayout = new QHBoxLayout(statusWidget);
        statusLayout->setContentsMargins(0, 0, 0, 0);
        statusLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QLabel *statusLabel = new QLabel(statusText);
        statusLabel->setStyleSheet(QString("background-color: %1; color: %2; padding: 2px 12px; border-radius: 12px; font-weight: 500;")
                                   .arg(statusBg, statusColor));
        statusLabel->setFont(QFont("", 15));
        statusLayout->addWidget(statusLabel);
        recordTable->setCellWidget(row, 3, statusWidget);

        // 查看按钮
        QPushButton *viewBtn = new QPushButton("查看");
        viewBtn->setProperty("ticketId", ticket);
        viewBtn->setStyleSheet(isDark ?
            "QPushButton { color: #6a8aff; border: 1px solid #6a8aff; border-radius: 4px; padding: 4px 14px; font-size: 14px; background: transparent; }"
            "QPushButton:hover { background-color: #6a8aff; color: #1a1a1a; }" :
            "QPushButton { color: #1a73e8; border: 1px solid #1a73e8; border-radius: 4px; padding: 4px 14px; font-size: 14px; background: transparent; }"
            "QPushButton:hover { background-color: #1a73e8; color: white; }");
        connect(viewBtn, &QPushButton::clicked, [this, ticket]() {
            showRecordDetail(ticket);
        });
        recordTable->setCellWidget(row, 4, viewBtn);
    }

    for (int row = 0; row < recordTable->rowCount(); ++row) {
        recordTable->setRowHeight(row, 50);
    }

    int tableWidth = qMax(recordTable->width(), 800);
    recordTable->setColumnWidth(0, tableWidth * 0.20);
    recordTable->setColumnWidth(1, tableWidth * 0.20);
    recordTable->setColumnWidth(2, tableWidth * 0.20);
    recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    recordTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    recordTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    recordTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    recordTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    updateTableStyle();
}

QTableWidgetItem* RegistrationRecordsWidget::makeTableItem(const QString &text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return item;
}

void RegistrationRecordsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (resizeTimer) {
        if (resizeTimer->isActive()) resizeTimer->stop();
        resizeTimer->start(300);
    }
}

void RegistrationRecordsWidget::showRecordDetail(const QString &ticketId)
{
    static bool dialogOpen = false;
    if (dialogOpen) return;

    dialogOpen = true;

    if (ticketId.isEmpty()) {
        QMessageBox::information(this, "提示", "无效的记录标识");
        dialogOpen = false;
        return;
    }

    for (const auto &val : m_allRecords) {
        QJsonObject obj = val.toObject();
        if (obj["ticket_id"].toString() == ticketId) {
            QString detailHtml = QString(
                "<b>单号：</b>%1<br>"
                "<b>科室：</b>%2<br>"
                "<b>医生：</b>%3<br>"
                "<b>日期：</b>%4<br>"
                "<b>症状：</b>%5<br>"
                "<b>状态：</b>%6<br>"
                "<b>挂号时间：</b>%7"
            ).arg(ticketId,
                  obj["department"].toString(),
                  obj["doctor"].toString(),
                  obj["date"].toString(),
                  obj["symptoms"].toString(),
                  obj["status"].toString(),
                  obj["created_at"].toString());

            QMessageBox msgBox;
            msgBox.setWindowTitle("挂号详情");
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setText(detailHtml);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setModal(true);
            msgBox.exec();

            dialogOpen = false;
            return;
        }
    }

    QMessageBox::information(this, "提示", "未找到该记录");
    dialogOpen = false;
}

void RegistrationRecordsWidget::refreshRecords()
{
    loadRecords();
}

void RegistrationRecordsWidget::updateTableStyle()
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);

    QString style = isDark ?
        "QTableWidget { background-color: #1e1e1e; color: #d0d8e8; gridline-color: #3a4a5a; }"
        "QTableWidget::item { background-color: transparent; padding: 6px; }"
        "QHeaderView::section { background-color: #2a2a3a; color: #b0b8c8; padding: 8px; border: none; border-bottom: 2px solid #4a5a7a; }" :
        "QTableWidget { background-color: #ffffff; color: #2c3e50; gridline-color: #e8edf2; }"
        "QTableWidget::item { background-color: transparent; padding: 6px; }"
        "QHeaderView::section { background-color: #e8edf2; color: #2c3e50; padding: 8px; border: none; border-bottom: 2px solid #d0d8e0; }";
    recordTable->setStyleSheet(style);

    // 日期选择器调色板
    QPalette pal = dateEdit->palette();
    if (isDark) {
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Base, QColor(40, 40, 40));
        pal.setColor(QPalette::Button, QColor(50, 50, 50));
    } else {
        pal.setColor(QPalette::WindowText, Qt::black);
        pal.setColor(QPalette::Text, Qt::black);
        pal.setColor(QPalette::Base, Qt::white);
        pal.setColor(QPalette::Button, Qt::white);
    }
    dateEdit->setPalette(pal);

    // 日历控件
    QCalendarWidget *cal = dateEdit->calendarWidget();
    if (cal) {
        if (isDark) {
            cal->setStyleSheet(
                "QCalendarWidget { background-color: #2a2a3a; color: #d0d8e8; }"
                "QCalendarWidget QToolButton { color: #aab8d0; }"
                "QCalendarWidget QTableView { selection-background-color: #4a6a8a; }"
                "QCalendarWidget QHeaderView::section { background-color: #3a3a5a; color: #b0b8c8; }"
                "QCalendarWidget QTableView::item { color: #d0d8e8; }"
            );
        } else {
            cal->setStyleSheet(
                "QCalendarWidget { background-color: #ffffff; color: #2c3e50; }"
                "QCalendarWidget QToolButton { color: #1a73e8; }"
                "QCalendarWidget QTableView { selection-background-color: #1a73e8; }"
                "QCalendarWidget QHeaderView::section { background-color: #e8edf2; color: #2c3e50; }"
                "QCalendarWidget QTableView::item { color: #2c3e50; }"
            );
        }
    }

    // 头部按钮
    QString borderColor = isDark ? "#6a8aff" : "#1a73e8";
    QString textColor = isDark ? "#6a8aff" : "#1a73e8";
    QString hoverBg = isDark ? "#6a8aff" : "#1a73e8";
    QString hoverText = isDark ? "#1a1a1a" : "#ffffff";
    QString btnStyle = QString(
        "QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 4px; padding: 4px 14px; font-size: 14px; font-weight: 500; min-height: 36px; min-width: 60px; color: %2; }"
        "QPushButton:hover { background-color: %3; color: %4; }"
    ).arg(borderColor, textColor, hoverBg, hoverText);

    if (backBtn) backBtn->setStyleSheet(btnStyle);
    if (resetBtn) resetBtn->setStyleSheet(btnStyle);
    if (refreshBtn) refreshBtn->setStyleSheet(btnStyle);
}