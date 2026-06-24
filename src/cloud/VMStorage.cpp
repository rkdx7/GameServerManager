#include "VMStorage.h"
#include <QSettings>
#include <QUuid>

static QString providerStr(VMProvider p) {
    switch (p) {
    case VMProvider::Local:    return "local";
    case VMProvider::Scaleway: return "scaleway";
    default:                   return "custom";
    }
}

static VMProvider providerFrom(const QString &s) {
    if (s == "local")    return VMProvider::Local;
    if (s == "scaleway") return VMProvider::Scaleway;
    return VMProvider::Custom;
}

void VMStorage::save(const QVector<VMInstance> &vms)
{
    QSettings s("GameServerManager", "App");
    s.beginWriteArray("infra/vms");
    for (int i = 0; i < vms.size(); ++i) {
        s.setArrayIndex(i);
        const auto &vm = vms[i];
        s.setValue("id",           vm.id);
        s.setValue("name",         vm.name);
        s.setValue("host",         vm.host);
        s.setValue("sshPort",      vm.sshPort);
        s.setValue("sshUser",      vm.sshUser);
        s.setValue("sshKeyPath",   vm.sshKeyPath);
        s.setValue("sudoPassword", vm.sudoPassword);
        s.setValue("provider",     providerStr(vm.provider));
        s.setValue("region",       vm.region);
        s.setValue("instanceType", vm.instanceType);
        s.setValue("scaServerId",  vm.scaServerId);
    }
    s.endArray();
}

QVector<VMInstance> VMStorage::load()
{
    QVector<VMInstance> result;

    // Always ensure Local entry at index 0
    VMInstance local;
    local.id       = "local";
    local.name     = "Local Docker";
    local.provider = VMProvider::Local;
    local.status   = VMStatus::Running;
    result.append(local);

    QSettings s("GameServerManager", "App");
    int count = s.beginReadArray("infra/vms");
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        VMInstance vm;
        vm.id           = s.value("id").toString();
        vm.name         = s.value("name").toString();
        vm.host         = s.value("host").toString();
        vm.sshPort      = s.value("sshPort", 22).toInt();
        vm.sshUser      = s.value("sshUser", "root").toString();
        vm.sshKeyPath   = s.value("sshKeyPath").toString();
        vm.sudoPassword = s.value("sudoPassword").toString();
        vm.provider     = providerFrom(s.value("provider").toString());
        vm.region       = s.value("region").toString();
        vm.instanceType = s.value("instanceType").toString();
        vm.scaServerId  = s.value("scaServerId").toString();

        // Skip duplicated local entries from old saves
        if (vm.provider == VMProvider::Local) continue;
        if (vm.id.isEmpty()) continue;

        result.append(vm);
    }
    s.endArray();
    return result;
}
