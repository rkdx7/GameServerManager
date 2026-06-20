#pragma once
#include <QString>
#include <QMap>

struct ServerInstanceConfig {
    QString id;
    QString displayName;
    QString containerName;
    int     port          = 0;
    QString vmId;                          // "" or "local" = local docker
    QMap<QString, QString> fieldValues;    // game-specific fields
    QString imageOverride;
    QString customConfig;
    bool    customConfigEnabled = false;
    bool    customConfigAtInstall = true;  // true = apply customConfig during install;
                                           // false = leave it for later editing via the dashboard
};
