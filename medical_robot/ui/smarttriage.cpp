// smarttriage.cpp
// 一键挂号组件：科室选择 + 挂号提交

#include "smarttriage.h"
#include "config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUrl>
#include <QTimer>

namespace {
    const QStringList DEFAULT_DEPTS = {
        "全科门诊", "外科", "内科", "皮肤科", "儿科", "心血管内科"
    };
}

SmartTriage::SmartTriage(QWidget *parent)
    : QWidget(parent)
    , networkManager(new QNetworkAccessManager(this))
    , m_selectedDept(QString())
    , isRefreshing(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 24, 30, 24);

    // 标题
    QLabel *titleLabel = new QLabel("📋 一键挂号");
    titleLabel->setObjectName("smartTitle");
    titleLabel->setFont(QFont("", 24, QFont::Bold));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 科室选择卡片
    QWidget *deptCard = new QWidget(this);
    deptCard->setObjectName("deptCard");
    QVBoxLayout *deptCardLayout = new QVBoxLayout(deptCard);
    deptCardLayout->setSpacing(12);
    deptCardLayout->setContentsMargins(20, 16, 20, 16);

    QLabel *deptTitle = new QLabel("选择就诊科室");
    deptTitle->setObjectName("deptTitle");
    deptTitle->setFont(QFont("", 16, QFont::Bold));
    deptCardLayout->addWidget(deptTitle);

    // 科室网格
    deptButtonContainer = new QWidget();
    deptGridLayout = new QGridLayout(deptButtonContainer);
    deptGridLayout->setSpacing(8);
    deptGridLayout->setContentsMargins(0, 0, 0, 0);
    deptButtonGroup = new QButtonGroup(this);
    deptButtonGroup->setExclusive(true);
    connect(deptButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &SmartTriage::onDeptButtonClicked);
    deptCardLayout->addWidget(deptButtonContainer);

    // 操作按钮
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(14);

    QPushButton *refreshDeptBtn = new QPushButton("刷新科室");
    refreshDeptBtn->setObjectName("refreshDeptBtn");
    refreshDeptBtn->setFont(QFont("", 16));
    connect(refreshDeptBtn, &QPushButton::clicked, this, &SmartTriage::onDepartmentRefreshClicked);
    actionLayout->addWidget(refreshDeptBtn);

    actionLayout->addStretch();

    registerBtn = new QPushButton("确认挂号");
    registerBtn->setObjectName("registerBtn");
    registerBtn->setFont(QFont("", 16, QFont::Bold));
    registerBtn->setMinimumHeight(48);
    connect(registerBtn, &QPushButton::clicked, this, &SmartTriage::onRegisterClicked);
    actionLayout->addWidget(registerBtn);

    deptCardLayout->addLayout(actionLayout);
    mainLayout->addWidget(deptCard);

    // 挂号结果卡片
    QWidget *resultCard = new QWidget(this);
    resultCard->setObjectName("resultCard");
    QVBoxLayout *resultCardLayout = new QVBoxLayout(resultCard);
    resultCardLayout->setSpacing(8);
    resultCardLayout->setContentsMargins(20, 16, 20, 16);

    QLabel *resultTitle = new QLabel("挂号结果");
    resultTitle->setObjectName("resultTitle");
    resultTitle->setFont(QFont("", 18, QFont::Bold));
    resultCardLayout->addWidget(resultTitle);

    resultBrowser = new QTextBrowser();
    resultBrowser->setObjectName("resultBrowser");
    resultBrowser->setFont(QFont("", 18));
    resultBrowser->setMinimumHeight(180);
    resultBrowser->setOpenExternalLinks(false);
    resultBrowser->setHtml(
        "<div style='color:#8899aa; font-size:18px; text-align:center; padding:40px 0;'>"
        "选择科室后点击「确认挂号」按钮<br>系统将为您办理挂号并显示结果</div>"
    );
    resultCardLayout->addWidget(resultBrowser);

    mainLayout->addWidget(resultCard);
    mainLayout->addStretch();

    // 启动后加载科室
    QTimer::singleShot(200, this, &SmartTriage::loadDepartments);
}

SmartTriage::~SmartTriage() {}

// 加载科室列表
void SmartTriage::loadDepartments()
{
    QString backendUrl = Config::instance().getBackendUrl();
    QUrl url(backendUrl + "/api/departments");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            m_departments = DEFAULT_DEPTS;
            populateDepartmentButtons();
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        reply->deleteLater();

        if (doc.isNull() || !doc.isObject()) {
            m_departments = DEFAULT_DEPTS;
            populateDepartmentButtons();
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray depts = obj["departments"].toArray();
        m_departments.clear();
        for (const auto &item : depts) {
            m_departments.append(item.toString());
        }

        if (m_departments.isEmpty()) {
            m_departments = DEFAULT_DEPTS;
        }
        populateDepartmentButtons();
    });
}

// 填充科室网格
void SmartTriage::populateDepartmentButtons()
{
    if (!deptGridLayout || !deptButtonGroup) return;

    // 删除旧按钮
    while (deptGridLayout->count() > 0) {
        QLayoutItem *item = deptGridLayout->takeAt(0);
        if (item->widget()) {
            QAbstractButton *btn = qobject_cast<QAbstractButton*>(item->widget());
            if (btn) deptButtonGroup->removeButton(btn);
            delete item->widget();
        }
        delete item;
    }

    if (m_departments.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无科室数据，请点击刷新按钮重试");
        emptyLabel->setStyleSheet("color: #8899aa;");
        deptGridLayout->addWidget(emptyLabel, 0, 0, 1, 4);
        return;
    }

    // 创建科室按钮（4列网格）
    const int cols = 4;
    for (int i = 0; i < m_departments.size(); ++i) {
        QPushButton *btn = new QPushButton(m_departments[i]);
        btn->setCheckable(true);
        btn->setAutoExclusive(false);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setMinimumHeight(40);
        deptButtonGroup->addButton(btn);
        deptGridLayout->addWidget(btn, i / cols, i % cols);
    }

    // 补齐占位
    int remainder = m_departments.size() % cols;
    if (remainder != 0) {
        int lastRow = m_departments.size() / cols;
        for (int i = remainder; i < cols; ++i) {
            QWidget *spacer = new QWidget();
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            deptGridLayout->addWidget(spacer, lastRow, i);
        }
    }

    m_selectedDept.clear();
}

// 科室按钮点击
void SmartTriage::onDeptButtonClicked(QAbstractButton *button)
{
    if (button) {
        m_selectedDept = button->text();
        resultBrowser->setHtml(QString(
            "<div style='color:#1a73e8; font-size:16px; padding:8px;'>✅ 已选择：<b>%1</b></div>"
        ).arg(m_selectedDept));
    }
}

// 刷新科室
void SmartTriage::onDepartmentRefreshClicked()
{
    if (isRefreshing) return;

    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) btn->setEnabled(false);

    isRefreshing = true;
    resultBrowser->setHtml(
        "<div style='color:#1a73e8; font-size:16px; padding:10px;'>🔄 正在刷新科室列表...</div>"
    );

    loadDepartments();

    isRefreshing = false;
    if (btn) btn->setEnabled(true);
    resultBrowser->setHtml(
        "<div style='color:#34a853; font-size:16px; padding:10px;'>✅ 科室列表已刷新</div>"
    );
}

// 确认挂号
void SmartTriage::onRegisterClicked()
{
    if (m_selectedDept.isEmpty()) {
        resultBrowser->setHtml(
            "<div style='color:#d93025; font-size:16px; padding:10px;'>⚠️ 请先选择就诊科室</div>"
        );
        return;
    }

    QString dept = m_selectedDept;
    resultBrowser->setHtml(
        "<div style='color:#1a73e8; font-size:16px; padding:10px;'>🔄 正在办理挂号，请稍候...</div>"
    );
    registerBtn->setEnabled(false);
    registerBtn->setText("挂号中...");

    QUrl url(Config::instance().getBackendUrl() + "/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["department"] = dept;
    body["patient_name"] = "用户";
    body["symptoms"] = "一键挂号";

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(body).toJson());
    
    // 使用局部 lambda 处理挂号响应
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        registerBtn->setEnabled(true);
        registerBtn->setText("确认挂号");

        if (reply->error() != QNetworkReply::NoError) {
            resultBrowser->setHtml(buildErrorHtml(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        reply->deleteLater();

        if (doc.isNull() || !doc.isObject()) {
            resultBrowser->setHtml(buildErrorHtml("解析响应失败"));
            return;
        }

        QJsonObject obj = doc.object();
        if (obj["success"].toBool()) {
            resultBrowser->setHtml(buildSuccessHtml(
                obj["department"].toString(),
                obj["ticket_id"].toString(),
                obj["queue_number"].toInt()
            ));
        } else {
            resultBrowser->setHtml(buildErrorHtml(obj["message"].toString("未知错误")));
        }
    });
}

// 构造成功 HTML
QString SmartTriage::buildSuccessHtml(const QString &dept, const QString &ticketId, int queueNum)
{
    return QString(R"(
        <div style='background-color: #e6f4ea; border: 2px solid #34a853; border-radius: 12px; padding: 20px; margin: 8px 0;'>
            <div style='font-size: 20px; font-weight: bold; color: #1e7e34; margin-bottom: 12px;'>✅ 挂号成功</div>
            <table style='font-size: 15px; color: #2c3e50; line-height: 2;'>
                <tr><td style='padding-right: 20px; color: #5f6b7a;'>科室</td><td style='font-weight: bold;'>%1</td></tr>
                <tr><td style='padding-right: 20px; color: #5f6b7a;'>号单号</td><td style='font-weight: bold; color: #1a73e8;'>%2</td></tr>
                <tr><td style='padding-right: 20px; color: #5f6b7a;'>排队号</td><td style='font-weight: bold; font-size: 22px; color: #d93025;'>%3 号</td></tr>
                <tr><td style='padding-right: 20px; color: #5f6b7a;'>时间</td><td>%4</td></tr>
            </table>
        </div>
    )").arg(dept, ticketId)
      .arg(queueNum)
      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

// 构造失败 HTML
QString SmartTriage::buildErrorHtml(const QString &error)
{
    return QString(R"(
        <div style='background-color: #fce8e6; border: 2px solid #d93025; border-radius: 12px; padding: 20px; margin: 8px 0;'>
            <div style='font-size: 18px; font-weight: bold; color: #d93025; margin-bottom: 8px;'>❌ 挂号失败</div>
            <div style='font-size: 16px; color: #5f6368;'>%1</div>
            <div style='font-size: 16px; color: #8899aa; margin-top: 8px;'>请检查后端服务是否运行，或稍后重试</div>
        </div>
    )").arg(error);
}