#include "AddCustomVMDialog.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFrame>
#include <QThread>
#include <QUuid>

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
const char *LABEL_STYLE = "font-size: 13px; font-weight: 600; color: #374151;";
} // namespace

AddCustomVMDialog::AddCustomVMDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Ajouter une VM personnalisée");
    setMinimumWidth(480);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *titleLbl = new QLabel("🖥  Ajouter une VM distante", this);
    titleLbl->setStyleSheet("font-size: 18px; font-weight: 700; color: #1e293b;");
    root->addWidget(titleLbl);

    auto *infoLbl = new QLabel(
        "Connectez une VM existante en fournissant son adresse IP et les détails SSH.\n"
        "Docker doit être déjà installé sur la machine.", this);
    infoLbl->setStyleSheet("font-size: 12px; color: #64748b;");
    infoLbl->setWordWrap(true);
    root->addWidget(infoLbl);

    auto *card = new QFrame(this);
    card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 12px; }");
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto *form = new QFormLayout(card);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft);

    auto mkL = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet(LABEL_STYLE);
        return l;
    };

    m_name = new QLineEdit(card);
    m_name->setPlaceholderText("Mon serveur de jeux");
    m_name->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Nom"), m_name);

    m_host = new QLineEdit(card);
    m_host->setPlaceholderText("192.168.1.100 ou domaine.com");
    m_host->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("IP / Hostname"), m_host);

    m_port = new QSpinBox(card);
    m_port->setRange(1, 65535);
    m_port->setValue(22);
    m_port->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Port SSH"), m_port);

    m_user = new QLineEdit(card);
    m_user->setText("root");
    m_user->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Utilisateur SSH"), m_user);

    auto *keyRow = new QWidget(card);
    auto *keyLayout = new QHBoxLayout(keyRow);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    keyLayout->setSpacing(8);

    m_keyPath = new QLineEdit(keyRow);
    m_keyPath->setPlaceholderText("Chemin vers votre clé privée SSH");
    m_keyPath->setStyleSheet(INPUT_STYLE);

    auto *browseBtn = new QPushButton("Parcourir", keyRow);
    browseBtn->setFixedHeight(36);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet(R"(
        QPushButton {
            background: #eef2ff; color: #4f46e5;
            border: none; border-radius: 8px;
            font-size: 12px; font-weight: 600; padding: 0 14px;
        }
        QPushButton:hover { background: #e0e7ff; }
    )");
    connect(browseBtn, &QPushButton::clicked, this, &AddCustomVMDialog::onBrowseKey);
    keyLayout->addWidget(m_keyPath);
    keyLayout->addWidget(browseBtn);
    form->addRow(mkL("Clé SSH privée"), keyRow);

    root->addWidget(card);

    // Status label
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_statusLabel);

    // Buttons
    auto *btnRow = new QHBoxLayout;
    auto *testBtn = new QPushButton("🔌  Tester la connexion", this);
    testBtn->setFixedHeight(40);
    testBtn->setCursor(Qt::PointingHandCursor);
    testBtn->setStyleSheet(R"(
        QPushButton {
            background: #eef2ff; color: #4f46e5;
            border: none; border-radius: 8px;
            font-size: 13px; font-weight: 600; padding: 0 18px;
        }
        QPushButton:hover { background: #e0e7ff; }
    )");
    connect(testBtn, &QPushButton::clicked, this, &AddCustomVMDialog::onTestConnection);

    auto *addBtn = new QPushButton("➕  Ajouter la VM", this);
    addBtn->setFixedHeight(40);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #6366f1, stop:1 #818cf8);
            color: white; border: none; border-radius: 8px;
            font-size: 13px; font-weight: 600; padding: 0 18px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #4f46e5, stop:1 #6366f1);
        }
    )");
    connect(addBtn, &QPushButton::clicked, this, &QDialog::accept);

    auto *cancelBtn = new QPushButton("Annuler", this);
    cancelBtn->setFixedHeight(40);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: #f1f5f9; color: #64748b;
            border: none; border-radius: 8px;
            font-size: 13px; font-weight: 600; padding: 0 18px;
        }
        QPushButton:hover { background: #e2e8f0; }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnRow->addWidget(testBtn);
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(addBtn);
    root->addLayout(btnRow);
}

VMInstance AddCustomVMDialog::result() const
{
    VMInstance vm;
    vm.id       = QUuid::createUuid().toString(QUuid::WithoutBraces);
    vm.name     = m_name->text().trimmed();
    vm.host     = m_host->text().trimmed();
    vm.sshPort  = m_port->value();
    vm.sshUser  = m_user->text().trimmed();
    vm.sshKeyPath = m_keyPath->text().trimmed();
    vm.provider = VMProvider::Custom;
    vm.status   = VMStatus::Unknown;
    return vm;
}

void AddCustomVMDialog::onBrowseKey()
{
    QString path = QFileDialog::getOpenFileName(this, "Sélectionner la clé SSH privée",
                                                QDir::homePath());
    if (!path.isEmpty())
        m_keyPath->setText(path);
}

void AddCustomVMDialog::onTestConnection()
{
    m_statusLabel->setStyleSheet("font-size: 12px; color: #6366f1;");
    m_statusLabel->setText("⏳ Test de connexion…");

    QString host    = m_host->text().trimmed();
    int     port    = m_port->value();
    QString user    = m_user->text().trimmed();
    QString keyPath = m_keyPath->text().trimmed();

    auto *thread = QThread::create([this, host, port, user, keyPath]() {
        DockerManager tmp;
        tmp.setRemoteMode(host, port, user, keyPath);
        QString info = tmp.dockerInfo();
        bool ok = !info.isEmpty() && !info.startsWith("Error");

        QMetaObject::invokeMethod(this, [this, ok, info]() {
            if (ok) {
                m_statusLabel->setStyleSheet("font-size: 12px; color: #22c55e; font-weight: 600;");
                m_statusLabel->setText("✅ Connexion réussie — Docker opérationnel");
            } else {
                m_statusLabel->setStyleSheet("font-size: 12px; color: #ef4444;");
                m_statusLabel->setText("❌ Échec de connexion: " + info);
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}
