// moretoolswidget.cpp
// 更多工具页面：挂号记录 + 监护数据入口

#include "moretoolswidget.h"
#include "registrationrecordswidget.h"
#include "monitordatawidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

MoreToolsWidget::MoreToolsWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    stackedWidget = new QStackedWidget(this);
    createGridPage();

    recordsPage = new RegistrationRecordsWidget(this);
    connect(recordsPage, &RegistrationRecordsWidget::backToGrid,
            this, &MoreToolsWidget::showGridPage);

    m_monitorDataPage = new MonitorDataWidget(this);
    connect(m_monitorDataPage, &MonitorDataWidget::backToGrid,
            this, &MoreToolsWidget::showGridPage);

    stackedWidget->addWidget(gridPage);
    stackedWidget->addWidget(recordsPage);
    stackedWidget->addWidget(m_monitorDataPage);
    stackedWidget->setCurrentIndex(0);

    mainLayout->addWidget(stackedWidget);
}

void MoreToolsWidget::createGridPage()
{
    gridPage = new QWidget();
    QVBoxLayout *gridLayout = new QVBoxLayout(gridPage);
    gridLayout->setAlignment(Qt::AlignTop);

    QLabel *title = new QLabel("📋 更多工具");
    title->setFont(QFont("", 20, QFont::Bold));
    gridLayout->addWidget(title);

    QWidget *gridContainer = new QWidget();
    QGridLayout *grid = new QGridLayout(gridContainer);
    grid->setSpacing(16);
    grid->setContentsMargins(0, 20, 0, 0);

    // 功能卡片样式
    const QString cardStyle =
        "QPushButton {"
        "  background-color: #f0f4f8;"
        "  border: 2px solid #d5e4ed;"
        "  border-radius: 12px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  color: #1a1a1a;"
        "  padding: 10px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e0e8f0;"
        "  border-color: #1a73e8;"
        "}";

    // 挂号记录卡片
    QPushButton *recordsCard = createCard("📋", "挂号记录", cardStyle);
    connect(recordsCard, &QPushButton::clicked, this, &MoreToolsWidget::showRecordsPage);
    grid->addWidget(recordsCard, 0, 0);

    // 监护数据卡片
    QPushButton *monitorCard = createCard("📊", "监护数据", cardStyle);
    connect(monitorCard, &QPushButton::clicked, this, &MoreToolsWidget::showMonitorData);
    grid->addWidget(monitorCard, 0, 1);

    // 占位卡片（预留扩展）
    QPushButton *placeholderCard = new QPushButton();
    placeholderCard->setFixedSize(180, 120);
    placeholderCard->setStyleSheet(
        "QPushButton {"
        "  background-color: #f8f9fa;"
        "  border: 2px dashed #d5e4ed;"
        "  border-radius: 12px;"
        "  font-size: 14px;"
        "  color: #8899aa;"
        "  padding: 10px;"
        "}"
        "QPushButton:hover { background-color: #f0f4f8; }"
    );
    QVBoxLayout *cardLayout = new QVBoxLayout(placeholderCard);
    cardLayout->setAlignment(Qt::AlignCenter);

    QLabel *icon = new QLabel("⚙️");
    icon->setFont(QFont("", 28));
    icon->setStyleSheet("background-color: #f8f9fa; border-radius: 4px; padding: 4px;");
    cardLayout->addWidget(icon);

    QLabel *text = new QLabel("其他功能\n待开发");
    text->setFont(QFont("", 13));
    text->setAlignment(Qt::AlignCenter);
    text->setStyleSheet("background-color: #f8f9fa; color: #4a8af4;");
    cardLayout->addWidget(text);

    placeholderCard->setEnabled(false);
    grid->addWidget(placeholderCard, 0, 2);

    grid->setColumnStretch(3, 1);
    gridLayout->addWidget(gridContainer);
    gridLayout->addStretch();
}

QPushButton* MoreToolsWidget::createCard(const QString &iconText, const QString &labelText,
                                         const QString &styleSheet)
{
    QPushButton *card = new QPushButton();
    card->setFixedSize(180, 120);
    card->setStyleSheet(styleSheet);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *icon = new QLabel(iconText);
    icon->setFont(QFont("", 28));
    icon->setStyleSheet("background-color: #f0f4f8; border-radius: 4px; padding: 4px;");
    layout->addWidget(icon);

    QLabel *label = new QLabel(labelText);
    label->setFont(QFont("", 14, QFont::Bold));
    label->setStyleSheet("background-color: #f0f4f8; color: #4a8af4;");
    layout->addWidget(label);

    return card;
}

void MoreToolsWidget::showRecordsPage()
{
    stackedWidget->setCurrentWidget(recordsPage);
    recordsPage->refreshRecords();
}

void MoreToolsWidget::showMonitorData()
{
    if (m_monitorDataPage) {
        stackedWidget->setCurrentWidget(m_monitorDataPage);
        m_monitorDataPage->refreshData();
    }
}

void MoreToolsWidget::showGridPage()
{
    stackedWidget->setCurrentWidget(gridPage);
}