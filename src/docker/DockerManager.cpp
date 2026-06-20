#include "DockerManager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

DockerManager::DockerManager(QObject *parent) : QObject(parent) {}

QString DockerManager::runRaw(const QStringList &args, int timeoutMs) {
    return run(args, timeoutMs);
}

QString DockerManager::containerLogs(const QString &name, int tailLines) {
    QProcess proc;
    // Docker writes container stderr to its own stderr; merge so we capture both.
    proc.setProcessChannelMode(QProcess::MergedChannels);
    QStringList logArgs = {"logs", "--tail", QString::number(tailLines), name};
    if (m_remote) {
        QStringList sshArgs;
        sshArgs << "-o" << "StrictHostKeyChecking=no"
                << "-o" << "BatchMode=yes"
                << "-p" << QString::number(m_sshPort);
        if (!m_keyPath.isEmpty()) sshArgs << "-i" << m_keyPath;
        sshArgs << QStringLiteral("%1@%2").arg(m_user, m_host) << "docker" << logArgs;
        proc.start("ssh", sshArgs);
    } else {
        proc.start("docker", logArgs);
    }
    proc.waitForFinished(15000);
    return QString::fromUtf8(proc.readAllStandardOutput());
}

void DockerManager::setLocalMode() { m_remote = false; }

void DockerManager::setRemoteMode(const QString &host, int sshPort,
                                   const QString &user, const QString &keyPath) {
    m_remote  = true;
    m_host    = host;
    m_sshPort = sshPort;
    m_user    = user;
    m_keyPath = keyPath;
}

QString DockerManager::run(const QStringList &args, int timeoutMs, const QByteArray &stdinData) {
    QProcess proc;
    if (m_remote) {
        QStringList sshArgs;
        sshArgs << "-o" << "StrictHostKeyChecking=no"
                << "-o" << "BatchMode=yes"
                << "-p" << QString::number(m_sshPort);
        if (!m_keyPath.isEmpty()) sshArgs << "-i" << m_keyPath;
        sshArgs << QStringLiteral("%1@%2").arg(m_user, m_host) << "docker" << args;
        proc.start("ssh", sshArgs);
    } else {
        proc.start("docker", args);
    }
    if (!stdinData.isEmpty()) {
        proc.waitForStarted(5000);
        proc.write(stdinData);
        proc.closeWriteChannel();
    }
    proc.waitForFinished(timeoutMs);
    return QString::fromUtf8(proc.readAllStandardOutput());
}

bool DockerManager::containerExists(const QString &name) {
    QString out = run({"ps", "-a", "--filter",
                        "name=^" + name + "$", "--format", "{{.Names}}"});
    return out.trimmed() == name;
}

bool DockerManager::isContainerRunning(const QString &name) {
    if (!containerExists(name)) return false;
    QString out = run({"inspect", "--format", "{{.State.Running}}", name});
    return out.trimmed() == "true";
}

void DockerManager::startContainer(const QString &name) {
    run({"start", name});
}

void DockerManager::stopContainer(const QString &name) {
    run({"stop", name}, 30000);
}

void DockerManager::restartContainer(const QString &name) {
    run({"restart", name}, 30000);
}

void DockerManager::removeContainer(const QString &name, bool withVolume) {
    run({"stop", name}, 30000);
    QStringList args = {"rm", "-f"};
    if (withVolume) args << "-v";
    args << name;
    run(args);
}

void DockerManager::runDetached(const QStringList &args) {
    if (m_remote) {
        QStringList sshArgs;
        sshArgs << "-o" << "StrictHostKeyChecking=no"
                << "-o" << "BatchMode=yes"
                << "-p" << QString::number(m_sshPort);
        if (!m_keyPath.isEmpty()) sshArgs << "-i" << m_keyPath;
        sshArgs << QStringLiteral("%1@%2").arg(m_user, m_host) << "docker" << args;
        QProcess::startDetached("ssh", sshArgs);
    } else {
        QProcess::startDetached("docker", args);
    }
}

float DockerManager::parseMemValue(const QString &s) {
    QString t = s.trimmed();
    QRegularExpression re(R"(([\d.]+)\s*(GiB|MiB|KiB|GB|MB|KB|B))");
    auto m = re.match(t);
    if (!m.hasMatch()) return 0.f;
    float val   = m.captured(1).toFloat();
    QString unit = m.captured(2);
    if (unit == "GiB" || unit == "GB") return val;
    if (unit == "MiB" || unit == "MB") return val / 1024.f;
    if (unit == "KiB" || unit == "KB") return val / (1024.f * 1024.f);
    return val / (1024.f * 1024.f * 1024.f);
}

ServerStats DockerManager::getStats(const QString &name) {
    ServerStats s;
    s.running = isContainerRunning(name);
    if (!s.running) return s;

    QString out = run({
        "stats", "--no-stream", "--format",
        "{{.CPUPerc}}\t{{.MemUsage}}", name
    }, 10000);

    if (!out.trimmed().isEmpty()) {
        QStringList cols = out.trimmed().split('\t');
        if (cols.size() >= 2) {
            s.cpu = cols[0].trimmed().remove('%').toFloat();
            QStringList mem = cols[1].split('/');
            if (mem.size() == 2) {
                s.memUsed  = parseMemValue(mem[0]);
                s.memTotal = parseMemValue(mem[1]);
            }
        }
    }

    // Disk space via df
    QString df = run({"exec", name, "df", "-BG", "--output=used,size", "/data"}, 5000);
    QStringList dfLines = df.trimmed().split('\n');
    if (dfLines.size() >= 2) {
        QStringList cols = dfLines.last().trimmed().split(QRegularExpression(R"(\s+)"));
        if (cols.size() >= 2) {
            s.diskUsed  = cols[0].remove('G').toFloat();
            s.diskTotal = cols[1].remove('G').toFloat();
        }
    }

    return s;
}

int DockerManager::getMinecraftPlayerCount(const QString &name) {
    QString out = run({"exec", name, "rcon-cli", "list"}, 8000);
    QRegularExpression re(R"(There are (\d+))");
    auto m = re.match(out);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

int DockerManager::getCS2PlayerCount(const QString &name, const QString &rconPass) {
    QString out = run({"exec", name, "rcon", "-H", "127.0.0.1",
                        "-p", "27015", "-P", rconPass, "status"}, 8000);
    QRegularExpression re(R"(players\s*:\s*(\d+))");
    auto m = re.match(out);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

QStringList DockerManager::listBackups(const QString &name) {
    QString out = run({"exec", name, "sh", "-c",
                        "ls /data/backups/*.tar.gz 2>/dev/null | xargs -I{} basename {}"}, 5000);
    if (out.trimmed().isEmpty()) return {};
    return out.trimmed().split('\n', Qt::SkipEmptyParts);
}

void DockerManager::createBackup(const QString &name) {
    QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    run({"exec", name, "sh", "-c",
         QStringLiteral("mkdir -p /data/backups && "
                        "tar czf /data/backups/backup_%1.tar.gz -C /data "
                        "$(ls /data | grep -v backups) 2>/dev/null").arg(ts)
        }, 120000);
}

void DockerManager::restoreBackup(const QString &name, const QString &backupFile) {
    run({"exec", name, "sh", "-c",
         QStringLiteral("tar xzf /data/backups/%1 -C /data").arg(backupFile)
        }, 120000);
}

QString DockerManager::execInContainer(const QString &container,
                                        const QStringList &cmd,
                                        int timeoutMs) {
    QStringList args;
    args << "exec" << container << cmd;
    return run(args, timeoutMs);
}

QString DockerManager::dockerInfo() {
    return run({"info", "--format", "{{.ServerVersion}}"}, 10000);
}

QStringList DockerManager::listLocalImages() {
    QString out = run({"images", "--format", "{{.Repository}}:{{.Tag}}"}, 15000);
    if (out.trimmed().isEmpty()) return {};
    QStringList imgs = out.trimmed().split('\n', Qt::SkipEmptyParts);
    imgs.erase(std::remove_if(imgs.begin(), imgs.end(),
        [](const QString &s){ return s.contains("<none>"); }), imgs.end());
    return imgs;
}

bool DockerManager::loginRegistry(const QString &registry,
                                   const QString &username,
                                   const QString &password) {
    QString out = run({"login", registry,
                        "-u", username,
                        "--password-stdin"}, 30000,
                       (password + "\n").toUtf8());
    return out.contains("Login Succeeded");
}

QStringList DockerManager::listPlugins(const QString &container) {
    QString out = run({"exec", container, "sh", "-c",
        "find /data/plugins -maxdepth 1 -name '*.jar' -exec basename {} \\; 2>/dev/null"}, 5000);
    if (out.trimmed().isEmpty()) return {};
    return out.trimmed().split('\n', Qt::SkipEmptyParts);
}

QStringList DockerManager::listPluginDirs(const QString &container) {
    QString out = run({"exec", container, "sh", "-c",
        "find /data/plugins -mindepth 1 -maxdepth 1 -type d -exec basename {} \\; 2>/dev/null"}, 5000);
    if (out.trimmed().isEmpty()) return {};
    return out.trimmed().split('\n', Qt::SkipEmptyParts);
}

QStringList DockerManager::listPluginConfigs(const QString &container, const QString &pluginDir) {
    QString cmd = QString(
        "find /data/plugins/%1 -maxdepth 1 -type f "
        "\\( -name '*.yml' -o -name '*.yaml' -o -name '*.json' "
        "-o -name '*.conf' -o -name '*.properties' -o -name '*.toml' \\) "
        "-exec basename {} \\; 2>/dev/null").arg(pluginDir);
    QString out = run({"exec", container, "sh", "-c", cmd}, 5000);
    if (out.trimmed().isEmpty()) return {};
    return out.trimmed().split('\n', Qt::SkipEmptyParts);
}

QString DockerManager::readFile(const QString &container, const QString &filePath) {
    return run({"exec", container, "cat", filePath}, 5000);
}

bool DockerManager::writeFile(const QString &container, const QString &filePath,
                               const QString &content) {
    QString cmd = QString("cat > '%1'").arg(filePath);
    run({"exec", "-i", container, "sh", "-c", cmd}, 10000, content.toUtf8());
    return true;
}

QString DockerManager::readPluginConfig(const QString &container, const QString &filePath) {
    return readFile(container, filePath);
}

bool DockerManager::writePluginConfig(const QString &container, const QString &filePath,
                                       const QString &content) {
    return writeFile(container, filePath, content);
}

bool DockerManager::installPlugin(const QString &container, const QString &localJarPath) {
    QString filename = QFileInfo(localJarPath).fileName();
    if (!m_remote) {
        QProcess proc;
        proc.start("docker", {"cp", localJarPath, container + ":/data/plugins/" + filename});
        proc.waitForFinished(30000);
        return proc.exitCode() == 0;
    }
    // Remote: encode as base64 and pipe via exec
    QFile f(localJarPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray b64 = f.readAll().toBase64();
    f.close();
    QString cmd = QString("echo %1 | base64 -d > /data/plugins/%2")
                      .arg(QString::fromLatin1(b64), filename);
    run({"exec", container, "sh", "-c", cmd}, 120000);
    return true;
}

bool DockerManager::deletePlugin(const QString &container, const QString &jarName) {
    run({"exec", container, "rm", "-f", "/data/plugins/" + jarName}, 5000);
    return true;
}
