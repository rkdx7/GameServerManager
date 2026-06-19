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
    QStringList deployedServers;
};
