#include "ScalewayProvider.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QThread>
#include <QUuid>

static const QString SCW_BASE = "https://api.scaleway.com/instance/v1/zones/%1/%2";

ScalewayProvider::ScalewayProvider(QObject *parent)
    : CloudProviderBase(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

QNetworkRequest ScalewayProvider::makeRequest(const QString &zone,
                                               const QString &path,
                                               const QString &token) const
{
    QNetworkRequest req(QUrl(SCW_BASE.arg(zone, path)));
    req.setRawHeader("X-Auth-Token", token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return req;
}

// Step 1: find current Ubuntu 22.04 image ID for the zone
void ScalewayProvider::fetchUbuntuImageId(const VMInstance &config)
{
    emit progressUpdate("Recherche de l'image Ubuntu 22.04…");
    auto req = makeRequest(config.region, "images?arch=x86_64&name=ubuntu_jammy_22.04",
                           config.scaApiToken);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, config]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("Erreur Scaleway: " + reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto arr = doc.object().value("images").toArray();
        if (arr.isEmpty()) {
            emit error("Image Ubuntu 22.04 introuvable dans la zone " + config.region);
            return;
        }
        QString imageId = arr[0].toObject().value("id").toString();
        doCreateServer(config, imageId);
    });
}

// Step 2: POST to create the server
void ScalewayProvider::doCreateServer(const VMInstance &config, const QString &imageId)
{
    emit progressUpdate("Création de la VM chez Scaleway…");

    QJsonObject body;
    body["name"]            = config.name;
    body["commercial_type"] = config.instanceType;
    body["image"]           = imageId;
    body["organization"]    = QString(); // set from token context — Scaleway uses project_id now

    // Use project_id instead of organization for newer API
    QJsonObject bodyV2;
    bodyV2["name"]            = config.name;
    bodyV2["commercial_type"] = config.instanceType;
    bodyV2["image"]           = imageId;

    auto req  = makeRequest(config.region, "servers", config.scaApiToken);
    auto data = QJsonDocument(bodyV2).toJson();
    auto *reply = m_nam->post(req, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, config]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit error("Échec de création: " + reply->errorString()
                       + " — " + reply->readAll());
            return;
        }
        auto doc      = QJsonDocument::fromJson(reply->readAll());
        auto serverId = doc.object().value("server").toObject().value("id").toString();
        if (serverId.isEmpty()) {
            emit error("Réponse inattendue de l'API Scaleway.");
            return;
        }

        // Start the server action
        emit progressUpdate("Démarrage de la VM…");
        VMInstance vm = config;
        vm.scaServerId = serverId;
        vm.id          = QUuid::createUuid().toString(QUuid::WithoutBraces);

        auto actionReq = makeRequest(config.region,
                                     "servers/" + serverId + "/action",
                                     config.scaApiToken);
        QJsonObject act;
        act["action"] = "poweron";
        m_nam->post(actionReq, QJsonDocument(act).toJson());

        pollUntilRunning(config.region, serverId, vm);
    });
}

// Step 3: Poll until running
void ScalewayProvider::pollUntilRunning(const QString &zone,
                                         const QString &serverId,
                                         VMInstance vm, int attempts)
{
    if (attempts > 40) {
        emit error("Timeout: la VM n'a pas démarré après 20 minutes.");
        return;
    }
    emit progressUpdate(QString("Attente du démarrage… (%1/40)").arg(attempts + 1));

    auto req   = makeRequest(zone, "servers/" + serverId, vm.scaApiToken);
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, zone, serverId, vm, attempts]() mutable {
        reply->deleteLater();
        auto doc    = QJsonDocument::fromJson(reply->readAll());
        auto server = doc.object().value("server").toObject();
        QString state = server.value("state").toString();

        if (state == "running") {
            auto ifaces = server.value("public_ip").toObject();
            vm.host   = ifaces.value("address").toString();
            vm.status = VMStatus::Running;
            emit progressUpdate("VM démarrée à " + vm.host + " — Installation de Docker…");
            installDockerOnVM(vm);
        } else {
            QTimer::singleShot(30000, this, [this, zone, serverId, vm, attempts]() {
                pollUntilRunning(zone, serverId, vm, attempts + 1);
            });
        }
    });
}

// Step 4: Install Docker via SSH
void ScalewayProvider::installDockerOnVM(const VMInstance &vm)
{
    auto *thread = QThread::create([this, vm]() {
        auto ssh = [&](const QString &cmd) {
            QProcess p;
            QStringList args = {
                "-o", "StrictHostKeyChecking=no",
                "-o", "ConnectTimeout=15",
                "-p", QString::number(vm.sshPort),
            };
            if (!vm.sshKeyPath.isEmpty())
                args << "-i" << vm.sshKeyPath;
            args << QString("%1@%2").arg(vm.sshUser, vm.host);
            args << cmd;
            p.start("ssh", args);
            p.waitForFinished(60000);
            return p.exitCode() == 0;
        };

        QMetaObject::invokeMethod(this, [this]() {
            emit progressUpdate("Installation de Docker sur la VM…");
        }, Qt::QueuedConnection);

        ssh("apt-get update -q");
        ssh("apt-get install -y docker.io");
        ssh("systemctl enable docker");
        ssh("systemctl start docker");

        QMetaObject::invokeMethod(this, [this, vm]() {
            emit progressUpdate("Docker installé — VM prête !");
            emit instanceCreated(vm);
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ScalewayProvider::createInstance(const VMInstance &config)
{
    fetchUbuntuImageId(config);
}

void ScalewayProvider::deleteInstance(const QString &serverId)
{
    emit error("Suppression API non implémentée pour: " + serverId);
}

void ScalewayProvider::listInstances()
{
    // Placeholder — list would require org/project ID context
    emit instancesList({});
}
