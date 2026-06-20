#pragma once
#include <QWidget>
#include <QString>
#include "DockerManager.h"

class QLabel;
class QProgressBar;
class QPushButton;
class QComboBox;
class QTimer;
class QFrame;
class QTabWidget;
class PluginManagerWidget;
class GameConsoleWidget;
class GameLogsWidget;
class ConfigEditorWidget;

enum class GameType { Minecraft, CS2, Generic };

class ServerDashboard : public QWidget {
    Q_OBJECT
public:
    explicit ServerDashboard(DockerManager *docker,
                              const QString &containerName,
                              GameType       type,
                              const QString &rconPass       = {},
                              const QString &configFilePath = {},
                              QWidget       *parent          = nullptr);
    ~ServerDashboard();

    void startPolling();
    void stopPolling();

signals:
    void uninstallRequested();

private slots:
    void refresh();
    void onBackup();
    void onRestore();

private:
    void updateDisplay(const ServerStats &stats, int players,
                       const QStringList &backups);
    QFrame *makeStatCard(const QString &title, QWidget *valueWidget,
                          QWidget *extra = nullptr);

    DockerManager *m_docker;
    QString        m_containerName;
    GameType       m_gameType;
    QString        m_rconPass;
    QString        m_configFilePath;
    QTimer        *m_timer;
    bool           m_refreshing = false;

    // Status
    QLabel       *m_statusDot;
    QLabel       *m_statusText;

    // CPU
    QProgressBar *m_cpuBar;
    QLabel       *m_cpuVal;

    // RAM
    QProgressBar *m_ramBar;
    QLabel       *m_ramVal;

    // Disk
    QProgressBar *m_diskBar;
    QLabel       *m_diskVal;

    // Players
    QLabel       *m_playerVal;

    // Backups
    QLabel       *m_backupCount;
    QComboBox    *m_backupCombo;

    // Action buttons
    QPushButton        *m_startBtn;
    QPushButton        *m_stopBtn;
    QPushButton        *m_restartBtn;

    // Plugin manager (Minecraft only)
    PluginManagerWidget *m_pluginManager = nullptr;

    // Admin console (all games)
    GameConsoleWidget   *m_console = nullptr;

    // Logs viewer (all games)
    GameLogsWidget      *m_logs = nullptr;

    // Config file editor (games with a known config path)
    ConfigEditorWidget  *m_configEditor = nullptr;
};
