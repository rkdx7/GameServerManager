#include "DockerSettingsPage.h"
#include "DockerManager.h"
#include "SshTunnel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFrame>
#include <QSettings>
#include <QThread>

namespace {
const char *INPUT_STYLE = R"(
    QLineEdit, QSpinBox {
        border: 2px solid #e2e8f0;
        border-radius: 8px;
        padding: 8px 12px;
        font-size: 13px;
        color: #1e293b;
        background: #f8fafc;
        min-height: 20px;
    }
    QLineEdit:focus, QSpinBox:focus {
        border-color: #6366f1;
        background: #ffffff;
    }
)";
} // namespace

DockerSettingsPage::DockerSettingsPage(DockerManager *docker, QWidget *parent)
    : QWidget(parent), m_docker(docker)
{
    m_tunnel = new SshTunnel(this);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 32, 40, 32);
    outer->setSpacing(0);

    // Header
    auto *icon  = new QLabel("⚙", this);
    icon->setStyleSheet("font-size: 40px; background: transparent;");
    auto *title = new QLabel("Configuration Docker", this);
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #1e293b; background: transparent;");
    auto *sub   = new QLabel("Choisissez entre Docker local ou un hôte distant via tunnel SSH.", this);
    sub->setStyleSheet("font-size: 13px; color: #64748b; background: transparent;");

    outer->addWidget(icon);
    outer->addSpacing(4);
    outer->addWidget(title);
    outer->addSpacing(4);
    outer->addWidget(sub);
    outer->addSpacing(28);

    // Config card
    auto *card = new QFrame(this);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("QFrame { background: #ffffff; border-radius: 14px; }");

    auto *cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(28, 24, 28, 24);
    cardLay->setSpacing(16);

    // Mode selection
    auto *modeRow = new QHBoxLayout;
    m_localMode  = new QRadioButton("🖥  Docker local", card);
    m_remoteMode = new QRadioButton("🌐  Hôte distant (SSH)", card);
    m_localMode->setChecked(true);
    const char *radStyle = "font-size: 14px; font-weight: 600; color: #374151;";
    m_localMode->setStyleSheet(radStyle);
    m_remoteMode->setStyleSheet(radStyle);
    modeRow->addWidget(m_localMode);
    modeRow->addWidget(m_remoteMode);
    modeRow->addStretch();
    cardLay->addLayout(modeRow);

    // Separator
    auto *sep = new QFrame(card);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: #e2e8f0; border: none; max-height: 1px;");
    cardLay->addWidget(sep);

    // SSH group
    m_sshGroup = new QWidget(card);
    auto *sshForm = new QFormLayout(m_sshGroup);
    sshForm->setContentsMargins(0, 0, 0, 0);
    sshForm->setSpacing(12);
    sshForm->setLabelAlignment(Qt::AlignLeft);

    auto mkLabel = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151;");
        return l;
    };

    m_host = new QLineEdit(m_sshGroup);
    m_host->setPlaceholderText("192.168.1.100 ou mon-serveur.com");
    m_host->setStyleSheet(INPUT_STYLE);

    m_sshPort = new QSpinBox(m_sshGroup);
    m_sshPort->setRange(1, 65535);
    m_sshPort->setValue(22);
    m_sshPort->setStyleSheet(INPUT_STYLE);

    m_user = new QLineEdit(m_sshGroup);
    m_user->setPlaceholderText("ubuntu");
    m_user->setStyleSheet(INPUT_STYLE);

    m_keyPath = new QLineEdit(m_sshGroup);
    m_keyPath->setPlaceholderText("Chemin vers la clé privée SSH (optionnel)");
    m_keyPath->setStyleSheet(INPUT_STYLE);

    auto *browseBtn = new QPushButton("Parcourir…", m_sshGroup);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet(R"(
        QPushButton {
            background: #f1f5f9; color: #374151;
            border: 1px solid #e2e8f0; border-radius: 6px;
            padding: 6px 14px; font-size: 12px;
        }
        QPushButton:hover { background: #e2e8f0; }
    )");
    connect(browseBtn, &QPushButton::clicked, this, &DockerSettingsPage::onBrowseKey);

    auto *keyRow = new QHBoxLayout;
    keyRow->setSpacing(8);
    keyRow->addWidget(m_keyPath, 1);
    keyRow->addWidget(browseBtn);
    auto *keyWidget = new QWidget(m_sshGroup);
    keyWidget->setLayout(keyRow);

    sshForm->addRow(mkLabel("Hôte"), m_host);
    sshForm->addRow(mkLabel("Port SSH"), m_sshPort);
    sshForm->addRow(mkLabel("Utilisateur"), m_user);
    sshForm->addRow(mkLabel("Clé privée"), keyWidget);

    m_sshGroup->setVisible(false);
    cardLay->addWidget(m_sshGroup);

    outer->addWidget(card);
    outer->addSpacing(20);

    // Status
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("font-size: 13px; background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    outer->addWidget(m_statusLabel);
    outer->addSpacing(12);

    // Buttons row
    auto *btnTest = new QPushButton("🔌  Tester la connexion", this);
    auto *btnSave = new QPushButton("💾  Enregistrer", this);

    auto makeBtn = [](QPushButton *b, const QString &bg, const QString &hover) {
        b->setFixedHeight(44);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString(R"(
            QPushButton { background:%1; color:white; border:none;
                          border-radius:10px; font-size:14px; font-weight:700; }
            QPushButton:hover { background:%2; }
            QPushButton:disabled { background: #94a3b8; }
        )").arg(bg, hover));
    };
    makeBtn(btnTest, "#6366f1", "#4f46e5");
    makeBtn(btnSave, "#22c55e", "#16a34a");

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);
    btnRow->addWidget(btnTest);
    btnRow->addWidget(btnSave);
    outer->addLayout(btnRow);

    outer->addStretch();

    // Connections
    connect(m_localMode,  &QRadioButton::toggled, this, &DockerSettingsPage::onModeChanged);
    connect(m_remoteMode, &QRadioButton::toggled, this, &DockerSettingsPage::onModeChanged);
    connect(btnTest, &QPushButton::clicked, this, &DockerSettingsPage::onTest);
    connect(btnSave, &QPushButton::clicked, this, &DockerSettingsPage::onSave);

    loadSettings();
}

void DockerSettingsPage::loadSettings() {
    QSettings s("GameServerManager", "App");
    bool remote = s.value("docker/remote", false).toBool();
    m_remoteMode->setChecked(remote);
    m_localMode->setChecked(!remote);
    m_host->setText(s.value("docker/host").toString());
    m_sshPort->setValue(s.value("docker/sshPort", 22).toInt());
    m_user->setText(s.value("docker/user").toString());
    m_keyPath->setText(s.value("docker/keyPath").toString());
    m_sshGroup->setVisible(remote);
}

void DockerSettingsPage::onModeChanged() {
    m_sshGroup->setVisible(m_remoteMode->isChecked());
}

void DockerSettingsPage::onBrowseKey() {
    QString path = QFileDialog::getOpenFileName(this, "Sélectionner la clé SSH",
                                                  QDir::homePath(), "Tous les fichiers (*)");
    if (!path.isEmpty()) m_keyPath->setText(path);
}

void DockerSettingsPage::onTest() {
    m_statusLabel->setStyleSheet("font-size: 13px; color: #6366f1; background: transparent;");
    m_statusLabel->setText("⏳ Test de connexion en cours…");

    bool remote = m_remoteMode->isChecked();
    QString host = m_host->text().trimmed();
    int port = m_sshPort->value();
    QString user = m_user->text().trimmed();
    QString key  = m_keyPath->text().trimmed();

    DockerManager *docker = m_docker;
    auto *thread = QThread::create([this, docker, remote, host, port, user, key]() {
        if (remote)
            docker->setRemoteMode(host, port, user, key);
        else
            docker->setLocalMode();

        QString version = docker->dockerInfo().trimmed();

        QMetaObject::invokeMethod(this, [this, version]() {
            if (!version.isEmpty()) {
                m_statusLabel->setStyleSheet("font-size: 13px; color: #22c55e; background: transparent;");
                m_statusLabel->setText("✅ Connexion réussie — Docker " + version);
            } else {
                m_statusLabel->setStyleSheet("font-size: 13px; color: #ef4444; background: transparent;");
                m_statusLabel->setText("❌ Docker non détecté. Vérifiez que Docker est installé et démarré.");
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void DockerSettingsPage::onSave() {
    QSettings s("GameServerManager", "App");
    bool remote = m_remoteMode->isChecked();
    s.setValue("docker/remote",  remote);
    s.setValue("docker/host",    m_host->text().trimmed());
    s.setValue("docker/sshPort", m_sshPort->value());
    s.setValue("docker/user",    m_user->text().trimmed());
    s.setValue("docker/keyPath", m_keyPath->text().trimmed());

    if (remote)
        m_docker->setRemoteMode(m_host->text().trimmed(), m_sshPort->value(),
                                 m_user->text().trimmed(), m_keyPath->text().trimmed());
    else
        m_docker->setLocalMode();

    m_statusLabel->setStyleSheet("font-size: 13px; color: #22c55e; background: transparent;");
    m_statusLabel->setText("✅ Configuration enregistrée.");
}
