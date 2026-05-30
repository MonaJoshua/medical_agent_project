#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include "patientmonitor.h"
#include "smarttriage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTabWidget *tabWidget;
    PatientMonitor *patientMonitor;
    SmartTriage *smartTriage;
};

#endif // MAINWINDOW_H
