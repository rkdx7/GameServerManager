#pragma once
#include <QString>
#include <QStringList>
#include <QList>

struct GameFieldConfig {
    enum FieldType { Text, Password, Combo, Spin, Check };
    QString     label;
    QString     key;
    FieldType   type         = Text;
    QString     defaultValue;
    QStringList comboOptions;
    int         spinMin      = 1;
    int         spinMax      = 65535;
    QString     placeholder;
    QString     envVar;   // empty = do not add -e flag
};

struct GamePortMapping {
    int  containerPort;
    int  hostPort = 0;    // 0 = user-configured primary port; >0 = fixed host port
    bool udp      = true;
    bool tcp      = false;
};

struct GamePageConfig {
    QString  icon;
    QString  title;
    QString  description;
    QString  note;
    QString  defaultContainerName;
    QString  dockerImage;
    QString  dataVolumePath  = "/data";
    int      defaultPort     = 27015;
    QList<GamePortMapping> ports;
    QString  btnColorStart   = "#6366f1";
    QString  btnColorEnd     = "#8b5cf6";
    QList<GameFieldConfig> fields;
    QString  configDocUrl;    // URL vers la documentation de configuration
    QString  configFilePath;  // Chemin du fichier de config dans le conteneur
};
