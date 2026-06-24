#pragma once
#include <QWidget>
#include <QString>

class DockerManager;
class QTextEdit;
class QLabel;
class QPushButton;

// Post-startup configuration editor for a running server. Loads the server's
// main config file from its container, lets the user edit it, then writes it
// back and restarts the server. Shared by every game dashboard so configuration
// stays editable after installation. When `configFilePath` is empty the widget
// shows an informational note instead (server has no editable config file).
class ServerSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ServerSettingsWidget(DockerManager *docker,
                                  const QString &containerName,
                                  const QString &configFilePath,
                                  const QString &configDocUrl = {},
                                  QWidget       *parent = nullptr);

public slots:
    void reload();   // re-read the config file from the container

private slots:
    void apply();    // write the edited config back and restart the server

private:
    void setStatus(const QString &text, bool error = false);

    DockerManager *m_docker;
    QString        m_container;
    QString        m_configPath;
    bool           m_busy = false;

    QTextEdit   *m_editor   = nullptr;
    QLabel      *m_status    = nullptr;
    QPushButton *m_reloadBtn = nullptr;
    QPushButton *m_applyBtn  = nullptr;
};
