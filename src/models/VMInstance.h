#pragma once
#include <QString>
#include <QStringList>

enum class VMProvider { Local, Custom, Scaleway };
enum class VMStatus   { Unknown, Provisioning, Running, Stopped, Error };

struct VMInstance {
    QString     id;
    QString     name;
    QString     host;
    int         sshPort      = 22;
    QString     sshUser      = "root";
    QString     sshKeyPath;
    VMProvider  provider     = VMProvider::Custom;
    VMStatus    status       = VMStatus::Unknown;
    QString     region;
    QString     instanceType;
    QString     scaApiToken;
    QString     scaServerId;
    QString     sudoPassword;   // optional; used to run privileged cmds for non-root users
    QStringList deployedServers;

    // A connection is "root" when no sudo elevation is needed for privileged
    // operations (Docker install, usermod, …).
    bool isRoot() const { return sshUser.trimmed() == "root"; }
};
