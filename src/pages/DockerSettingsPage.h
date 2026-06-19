#pragma once
#include <QWidget>

class DockerManager;
class SshTunnel;
class QRadioButton;
class QLineEdit;
class QSpinBox;
class QLabel;
class QWidget;

class DockerSettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit DockerSettingsPage(DockerManager *docker, QWidget *parent = nullptr);

private slots:
    void onModeChanged();
    void onTest();
    void onSave();
    void onBrowseKey();

private:
    void loadSettings();

    DockerManager *m_docker;
    SshTunnel     *m_tunnel;

    QRadioButton *m_localMode;
    QRadioButton *m_remoteMode;
    QWidget      *m_sshGroup;
    QLineEdit    *m_host;
    QSpinBox     *m_sshPort;
    QLineEdit    *m_user;
    QLineEdit    *m_keyPath;
    QLabel       *m_statusLabel;
};
