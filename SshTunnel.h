#pragma once
#include <QObject>
#include <QString>

class QProcess;

class SshTunnel : public QObject {
    Q_OBJECT
public:
    explicit SshTunnel(QObject *parent = nullptr);
    ~SshTunnel();

    void start(const QString &host, int port,
               const QString &user, const QString &keyPath);
    void stop();
    bool isRunning() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);

private:
    QProcess *m_process = nullptr;
};
