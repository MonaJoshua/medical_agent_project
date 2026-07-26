#include "moretoolswidget.h"
#include "registrationrecordswidget.h"
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

    stackedWidget->addWidget(gridPage);
    stackedWidget->addWidget(recordsPage);
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

    // ---- 挂号记录卡片 ----
    QPushButton *recordsCard = new QPushButton();
    recordsCard->setFixedSize(180, 120);
    recordsCard->setStyleSheet(R"(
        QPushButton {
            background-color: #f0f4f8;
            border: 2px solid #d5e4ed;
            border-radius: 12px;
            font-size: 16px;
            font-weight: bold;
            color: #1a1a1a;
            padding: 10px;
        }
        QPushButton:hover {
            background-color: #e0e8f0;
            border-color: #1a73e8;
        }
    )");
    QVBoxLayout *cardLayout = new QVBoxLayout(recordsCard);
    cardLayout->setAlignment(Qt::AlignCenter);

    QLabel *icon1 = new QLabel("📋");
    icon1->setFont(QFont("", 28));
    icon1->setStyleSheet("background-color: #f0f4f8; border-radius: 4px; padding: 4px;");
    cardLayout->addWidget(icon1);

    QLabel *text1 = new QLabel("挂号记录");
    text1->setFont(QFont("", 14, QFont::Bold));
    text1->setStyleSheet("background-color: #f0f4f8; color: #4a8af4;");
    cardLayout->addWidget(text1);

    connect(recordsCard, &QPushButton::clicked, this, &MoreToolsWidget::showRecordsPage);
    grid->addWidget(recordsCard, 0, 0);

    // ---- 占位卡片 ----
    QPushButton *placeholderCard = new QPushButton();
    placeholderCard->setFixedSize(180, 120);
    placeholderCard->setStyleSheet(R"(
        QPushButton {
            background-color: #f8f9fa;
            border: 2px dashed #d5e4ed;
            border-radius: 12px;
            font-size: 14px;
            color: #8899aa;
            padding: 10px;
        }
        QPushButton:hover {
            background-color: #f0f4f8;
        }
    )");
    QVBoxLayout *cardLayout2 = new QVBoxLayout(placeholderCard);
    cardLayout2->setAlignment(Qt::AlignCenter);

    QLabel *icon2 = new QLabel("⚙️");
    icon2->setFont(QFont("", 28));
    icon2->setStyleSheet("background-color: #f8f9fa; border-radius: 4px; padding: 4px;");
    cardLayout2->addWidget(icon2);

    QLabel *text2 = new QLabel("其他功能\n待开发");
    text2->setFont(QFont("", 13));
    text2->setAlignment(Qt::AlignCenter);
    text2->setStyleSheet("background-color: #f8f9fa; color: #4a8af4;");
    cardLayout2->addWidget(text2);

    placeholderCard->setEnabled(false);
    grid->addWidget(placeholderCard, 0, 1);

    grid->setColumnStretch(2, 1);
    gridLayout->addWidget(gridContainer);
    gridLayout->addStretch();
}

void MoreToolsWidget::showRecordsPage()
{
    stackedWidget->setCurrentIndex(1);
    // 如果记录为空，触发加载
    // 但 RegistrationRecordsWidget 会在构造时自动加载，所以无需额外操作
}

void MoreToolsWidget::showGridPage()
{
    stackedWidget->setCurrentIndex(0);
}