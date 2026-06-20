#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QVector>
#include "GamePageConfig.h"
#include "ServerInstance.h"
#include "VMInstance.h"

class DockerManager;
class ServerDashboard;
class DeploymentTargetSelector;
class QStackedWidget;
class QLineEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QListWidget;
class QTextEdit;
class QComboBox;

class GenericGamePage : public QWidget {
    Q_OBJECT
public:
    explicit GenericGamePage(DockerManager *docker,
                              const GamePageConfig &config,
                              const QVector<VMInstance> *vmList,
                              QWidget *parent = nullptr);

private slots:
    void onInstall();
    void onUninstall();
    void openImagePicker();
    void switchToInstance(int idx);
    void addInstance();
    void deleteInstance(int idx);

private:
    void    checkStatus();
    QWidget *buildInstallForm();
    QWidget *buildInstancePanel();
    QString  fieldValue(const QString &key) const;
    void     saveCurrentFormState();
    void     loadFormFromInstance(const ServerInstanceConfig &inst);
    void     loadInstances();
    void     saveInstances();
    void     updateInstanceButtons();

    DockerManager              *m_docker;
    GamePageConfig              m_config;
    const QVector<VMInstance>  *m_vmList;
    QStackedWidget             *m_stack;
    DeploymentTargetSelector   *m_targetSelector     = nullptr;

    // Instance management
    QList<ServerInstanceConfig>         m_instances;
    int                                  m_currentInstanceIdx = -1;
    QListWidget                         *m_instanceList       = nullptr;
    QMap<QString, ServerDashboard*>      m_dashboards;
    QPushButton                         *m_deleteInstanceBtn   = nullptr;

    // Install form widgets
    QLineEdit              *m_serverName = nullptr;
    QSpinBox               *m_port       = nullptr;
    QMap<QString, QWidget*> m_fieldWidgets;
    QPushButton            *m_installBtn    = nullptr;
    QLabel                 *m_installStatus = nullptr;

    // Advanced settings
    QPushButton *m_advancedToggleBtn = nullptr;
    QWidget     *m_advancedContent   = nullptr;
    QTextEdit   *m_customConfigEdit  = nullptr;
    QComboBox   *m_customTimingCombo = nullptr;  // when to apply the custom config

    // Image override
    QString  m_imageOverride;
    bool     m_needsLogin   = false;
    QString  m_loginReg;
    QString  m_loginUser;
    QString  m_loginPass;
    QLabel  *m_imageBadge   = nullptr;
};
