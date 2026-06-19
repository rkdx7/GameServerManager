#include "SshTunnel.h"
#include <QProcess>

SshTunnel::SshTunnel(QObject *parent) : QObject(parent) {}

SshTunnel::~SshTunnel() { stop(); }

void SshTunnel::start(const QString &host, int port,
                       const QString &user, const QString &keyPath) {
    stop();

    m_process = new QProcess(this);

    QStringList args;
    args << "-N"
         << "-L" << "2375:/var/run/docker.sock"
         << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "BatchMode=yes"
         << "-p" << QString::number(port);
    if (!keyPath.isEmpty()) args << "-i" << keyPath;
    args << QStringLiteral("%1@%2").arg(user, host);

    connect(m_process, &QProcess::started,
            this, &SshTunnel::connected);
    connect(m_process, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError) {
        emit errorOccurred(m_process ? m_process->errorString() : "Erreur inconnue");
    });
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SshTunnel::disconnected);

    m_process->start("ssh", args);
}

void SshTunnel::stop() {
    if (m_process) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000))
            m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

bool SshTunnel::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}
