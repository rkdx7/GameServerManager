#pragma once
#include "CloudProviderBase.h"
#include <QNetworkAccessManager>
#include <QTimer>

class ScalewayProvider : public CloudProviderBase {
    Q_OBJECT
public:
    explicit ScalewayProvider(QObject *parent = nullptr);

    void createInstance(const VMInstance &config) override;
    void deleteInstance(const QString &serverId) override;
    void listInstances() override;

private:
    void fetchUbuntuImageId(const VMInstance &config);
    void doCreateServer(const VMInstance &config, const QString &imageId);
    void pollUntilRunning(const QString &zone, const QString &serverId,
                          VMInstance vm, int attempts = 0);
    void installDockerOnVM(const VMInstance &vm);

    QNetworkRequest makeRequest(const QString &zone, const QString &path,
                                const QString &token) const;

    QNetworkAccessManager *m_nam;
    QTimer                *m_pollTimer = nullptr;
};
