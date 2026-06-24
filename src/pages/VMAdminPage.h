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
class QCheckBox;

class VMAdminPage : public QDialog {
    Q_OBJECT
public:
    explicit VMAdminPage(const VMInstance &vm, QWidget *parent = nullptr);

private slots:
    void onStart();
    void onStop();
    void onRestart();
    void onSendConsoleCmd();
    void onInstallDocker();
    void refreshServers();
    void refreshMetrics();

private:
    QWidget *buildOverviewTab();
    QWidget *buildServersTab();
    QWidget *buildMetricsTab();
    QWidget *buildConsoleTab();

    // Appends an HTML-coloured line to the console output (thread-safe via the
    // widget's event loop is the caller's responsibility — call on the GUI thread).
    void appendConsole(const QString &html);

    // Run a Docker-install script over SSH, streaming output to the console.
    void runDockerInstall(const QString &script);
    // Show the ready-to-run interactive command for a non-root user without a
    // stored sudo password (so they only type their password in a terminal).
    void promptManualDockerInstall(const QString &script);

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
    QCheckBox *m_sudoCheck     = nullptr;
    QPushButton *m_installBtn  = nullptr;

    // Status
    QLabel  *m_vmStatusLabel = nullptr;
    QTimer  *m_metricsTimer  = nullptr;
};
