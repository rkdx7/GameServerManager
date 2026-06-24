#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QVector>
#include "ServerInstance.h"
#include "VMInstance.h"

class DockerManager;
class ServerDashboard;
class DeploymentTargetSelector;
class QStackedWidget;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QLabel;
class QPushButton;
class QListWidget;
class QTextEdit;

class CS2Page : public QWidget {
    Q_OBJECT
public:
    explicit CS2Page(DockerManager *docker,
                     const QVector<VMInstance> *vmList = nullptr,
                     QWidget *parent = nullptr);

private slots:
    void onInstall();
    void onUninstall();
    void onUpgrade();
    void openImagePicker();
    void switchToInstance(int idx);
    void addInstance();
    void deleteInstance(int idx);

private:
    void checkStatus();
    QWidget *buildInstallForm();
    QWidget *buildInstancePanel();
    void recreateContainer(const QString &statusMsg);
    void saveCurrentFormState();
    void loadFormFromInstance(const ServerInstanceConfig &inst);
    void loadInstances();
    void saveInstances();
    void updateInstanceButtons();

    DockerManager             *m_docker;
    const QVector<VMInstance> *m_vmList    = nullptr;
    QStackedWidget            *m_stack;
    DeploymentTargetSelector  *m_targetSelector = nullptr;

    // Instance management
    QList<ServerInstanceConfig>         m_instances;
    int                                  m_currentInstanceIdx = -1;
    QListWidget                         *m_instanceList       = nullptr;
    QMap<QString, ServerDashboard*>      m_dashboards;
    QPushButton                         *m_deleteInstanceBtn   = nullptr;

    // Install form fields
    QLineEdit  *m_serverName;
    QLineEdit  *m_gslt;
    QLineEdit  *m_rconPass;
    QComboBox  *m_gameMode;
    QComboBox  *m_startMap;
    QSpinBox   *m_port;
    QPushButton *m_installBtn;
    QLabel      *m_installStatus;
    QLabel      *m_imageBadge   = nullptr;

    // Advanced settings
    QPushButton *m_advancedToggleBtn = nullptr;
    QWidget     *m_advancedContent   = nullptr;
    QTextEdit   *m_customConfigEdit  = nullptr;

    // Image override
    QString  m_imageOverride;
    bool     m_needsLogin = false;
    QString  m_loginReg;
    QString  m_loginUser;
    QString  m_loginPass;
};
