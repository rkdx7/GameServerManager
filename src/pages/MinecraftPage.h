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
class QCheckBox;
class QLabel;
class QPushButton;
class QListWidget;
class QTextEdit;

class MinecraftPage : public QWidget {
    Q_OBJECT
public:
    explicit MinecraftPage(DockerManager *docker,
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
    QComboBox  *m_version;
    QComboBox  *m_serverType;
    QComboBox  *m_memory;
    QSpinBox   *m_port;
    QCheckBox  *m_eula;
    QPushButton *m_installBtn;
    QLabel      *m_installStatus;
    QLabel      *m_imageBadge   = nullptr;

    // Advanced settings
    QPushButton *m_advancedToggleBtn = nullptr;
    QWidget     *m_advancedContent   = nullptr;
    QTextEdit   *m_customConfigEdit  = nullptr;
    QComboBox   *m_customTimingCombo = nullptr;  // when to apply the custom config

    // Image override
    QString  m_imageOverride;
    bool     m_needsLogin = false;
    QString  m_loginReg;
    QString  m_loginUser;
    QString  m_loginPass;
};
