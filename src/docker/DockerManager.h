#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

struct ServerStats {
    float cpu      = 0.f;
    float memUsed  = 0.f;
    float memTotal = 0.f;
    float diskUsed = 0.f;
    float diskTotal= 0.f;
    bool  running  = false;
};

class DockerManager : public QObject {
    Q_OBJECT
public:
    explicit DockerManager(QObject *parent = nullptr);

    void setLocalMode();
    void setRemoteMode(const QString &host, int sshPort,
                       const QString &user, const QString &keyPath);

    bool        containerExists(const QString &name);
    bool        isContainerRunning(const QString &name);
    void        startContainer(const QString &name);
    void        stopContainer(const QString &name);
    void        restartContainer(const QString &name);
    void        removeContainer(const QString &name, bool withVolume = true);
    void        runDetached(const QStringList &args);

    ServerStats  getStats(const QString &name);
    int          getMinecraftPlayerCount(const QString &name);
    int          getCS2PlayerCount(const QString &name, const QString &rconPass);
    QStringList  listBackups(const QString &name);
    void         createBackup(const QString &name);
    void         restoreBackup(const QString &name, const QString &backupFile);

    QString      execInContainer(const QString &container,
                                   const QStringList &cmd,
                                   int timeoutMs = 10000);

    // Installed-version / image introspection
    QString      containerImage(const QString &name);
    QString      containerEnv(const QString &name, const QString &key);

    // Generic config-file read/write inside a container
    QString      readContainerFile(const QString &name, const QString &filePath);
    bool         writeContainerFile(const QString &name, const QString &filePath,
                                    const QString &content);

    QString      dockerInfo();
    QStringList  listLocalImages();
    bool         loginRegistry(const QString &registry,
                                const QString &username,
                                const QString &password);

    QString      runRaw(const QStringList &args, int timeoutMs = 15000);

    // Container logs (stdout + stderr merged), most recent `tailLines` lines
    QString      containerLogs(const QString &name, int tailLines = 200);

    // Plugin management (Minecraft PAPER/SPIGOT)
    QStringList  listPlugins(const QString &container);
    QStringList  listPluginDirs(const QString &container);
    QStringList  listPluginConfigs(const QString &container, const QString &pluginDir);
    QString      readPluginConfig(const QString &container, const QString &filePath);
    bool         writePluginConfig(const QString &container, const QString &filePath, const QString &content);
    bool         installPlugin(const QString &container, const QString &localJarPath);
    bool         deletePlugin(const QString &container, const QString &jarName);

private:
    QString      run(const QStringList &args, int timeoutMs = 15000,
                     const QByteArray &stdinData = {});
    float        parseMemValue(const QString &s);

    bool    m_remote  = false;
    QString m_host;
    int     m_sshPort = 22;
    QString m_user;
    QString m_keyPath;
};
