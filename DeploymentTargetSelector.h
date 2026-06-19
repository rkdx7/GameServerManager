#pragma once
#include <QWidget>
#include <QVector>
#include "VMInstance.h"

class DockerManager;
class QComboBox;

class DeploymentTargetSelector : public QWidget {
    Q_OBJECT
public:
    explicit DeploymentTargetSelector(const QString &gameKey,
                                       DockerManager *docker,
                                       const QVector<VMInstance> *vmList,
                                       QWidget *parent = nullptr);

    void refreshList();
    void setTargetById(const QString &vmId); // switch target programmatically
    const VMInstance *selectedVM() const; // nullptr = local

signals:
    void targetChanged(const VMInstance *vm); // nullptr = local docker

private:
    void applySelection();

    QString                    m_gameKey;
    DockerManager             *m_docker;
    const QVector<VMInstance> *m_vmList;
    QComboBox                 *m_combo;
};
