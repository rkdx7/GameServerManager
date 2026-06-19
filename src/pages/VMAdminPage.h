#pragma once
#include <QDialog>
#include "VMInstance.h"

class DockerManager;
class QLabel;
class QListWidget;
class QProgressBar;
class QTextEdit;
class QLineEdit;
class QTabWidget;
class QTimer;
class QPushButton;

class VMAdminPage : public QDialog {
    Q_OBJECT
public:
    explicit VMAdminPage(const VMInstance &vm, QWidget *parent = nullptr);

private slots:
    void onStart();
    void onStop();
    void onRestart();
    void onSendConsoleCmd();
    void refreshServers();
    void refreshMetrics();

private:
    QWidget *buildOverviewTab();
    QWidget *buildServersTab();
    QWidget *buildMetricsTab();
    QWidget *buildConsoleTab();

    VMInstance    m_vm;
    DockerManager *m_docker = nullptr;

    // Servers tab
    QListWidget *m_serverList = nullptr;

    // Metrics tab
    QProgressBar *m_cpuBar  = nullptr;
    QProgressBar *m_ramBar  = nullptr;
    QProgressBar *m_diskBar = nullptr;
    QLabel       *m_cpuLbl  = nullptr;
    QLabel       *m_ramLbl  = nullptr;
    QLabel       *m_diskLbl = nullptr;

    // Console tab
    QTextEdit *m_consoleOutput = nullptr;
    QLineEdit *m_consoleInput  = nullptr;

    // Status
    QLabel  *m_vmStatusLabel = nullptr;
    QTimer  *m_metricsTimer  = nullptr;
};
