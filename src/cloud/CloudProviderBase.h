#pragma once
#include <QObject>
#include <QVector>
#include "VMInstance.h"

class CloudProviderBase : public QObject {
    Q_OBJECT
public:
    explicit CloudProviderBase(QObject *parent = nullptr) : QObject(parent) {}

    virtual void createInstance(const VMInstance &config) = 0;
    virtual void deleteInstance(const QString &serverId)  = 0;
    virtual void listInstances()                          = 0;

signals:
    void instanceCreated(VMInstance vm);
    void instanceDeleted(QString id);
    void instancesList(QVector<VMInstance> vms);
    void progressUpdate(QString msg);
    void error(QString msg);
};
