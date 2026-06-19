#include "CreateVMDialog.h"
#include "ScalewayProvider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QFileDialog>

namespace {
const char *INPUT_STYLE = R"(
    QLineEdit, QComboBox {
        border: 2px solid #e2e8f0;
        border-radius: 8px;
        padding: 8px 12px;
        font-size: 13px;
        color: #1e293b;
        background: #f8fafc;
        min-height: 20px;
    }
    QLineEdit:focus, QComboBox:focus {
        border-color: #6366f1;
        background: #ffffff;
    }
    QComboBox::drop-down { border: none; }
    QComboBox QAbstractItemView {
        background: #ffffff; color: #1e293b;
        selection-background-color: #6366f1; selection-color: #ffffff;
        border: 1px solid #e2e8f0; border-radius: 4px; outline: none;
    }
)";
const char *LABEL_STYLE = "font-size: 13px; font-weight: 600; color: #374151;";
} // namespace

CreateVMDialog::CreateVMDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Créer une VM Scaleway");
    setMinimumSize(560, 540);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *titleLbl = new QLabel("☁  Créer une VM chez Scaleway", this);
    titleLbl->setStyleSheet("font-size: 18px; font-weight: 700; color: #1e293b;");
    root->addWidget(titleLbl);

    auto *tabs = new QTabWidget(this);
    tabs->setStyleSheet(R"(
        QTabWidget::pane { border: none; }
        QTabBar::tab {
            background: #f1f5f9; color: #64748b;
            border: none; border-radius: 6px;
            padding: 8px 18px; margin-right: 4px; font-size: 12px; font-weight: 600;
        }
        QTabBar::tab:selected { background: #6366f1; color: white; }
    )");
    tabs->addTab(buildPrerequisitesTab(), "📋  Prérequis");
    tabs->addTab(buildConfigTab(),        "⚙  Configuration");
    root->addWidget(tabs, 1);

    // Status
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    // Buttons
    auto *btnRow = new QHBoxLayout;
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

    m_createBtn = new QPushButton("🚀  Créer la VM", this);
    m_createBtn->setFixedHeight(40);
    m_createBtn->setCursor(Qt::PointingHandCursor);
    m_createBtn->setStyleSheet(R"(
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
        QPushButton:disabled { background: #94a3b8; }
    )");
    connect(m_createBtn, &QPushButton::clicked, this, &CreateVMDialog::onCreate);

    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_createBtn);
    root->addLayout(btnRow);
}

QWidget *CreateVMDialog::buildPrerequisitesTab()
{
    auto *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("background: transparent;");

    auto *w = new QWidget;
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(8, 16, 8, 16);
    layout->setSpacing(12);

    auto makeStep = [&](const QString &num, const QString &title, const QString &detail) {
        auto *frame = new QFrame(w);
        frame->setStyleSheet("QFrame { background: #f8fafc; border-radius: 10px; }");
        frame->setAttribute(Qt::WA_StyledBackground, true);
        auto *fl = new QHBoxLayout(frame);
        fl->setContentsMargins(16, 14, 16, 14);
        fl->setSpacing(14);

        auto *numLbl = new QLabel(num, frame);
        numLbl->setFixedSize(32, 32);
        numLbl->setAlignment(Qt::AlignCenter);
        numLbl->setStyleSheet(
            "background: #6366f1; color: white; border-radius: 16px;"
            "font-size: 13px; font-weight: 700;");

        auto *textCol = new QVBoxLayout;
        auto *titleLbl = new QLabel(title, frame);
        titleLbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #1e293b;");
        auto *detailLbl = new QLabel(detail, frame);
        detailLbl->setStyleSheet("font-size: 12px; color: #64748b;");
        detailLbl->setWordWrap(true);
        textCol->addWidget(titleLbl);
        textCol->addWidget(detailLbl);

        fl->addWidget(numLbl);
        fl->addLayout(textCol, 1);
        layout->addWidget(frame);
    };

    auto *headerLbl = new QLabel("Avant de créer votre VM, vous devez :", w);
    headerLbl->setStyleSheet("font-size: 14px; font-weight: 700; color: #1e293b;");
    layout->addWidget(headerLbl);

    makeStep("1", "Créer un compte Scaleway",
             "Rendez-vous sur console.scaleway.com et créez votre compte.\n"
             "Aucune carte bancaire requise pour commencer.");

    makeStep("2", "Générer une clé SSH",
             "Exécutez : ssh-keygen -t ed25519 -C \"votre@email.com\"\n"
             "Puis ajoutez la clé publique (~/.ssh/id_ed25519.pub) dans\n"
             "votre compte Scaleway → Credentials → SSH Keys.");

    makeStep("3", "Créer un token API",
             "Dans la console Scaleway :\n"
             "IAM → API Keys → Générer un token API.\n"
             "Copiez le token — il ne s'affiche qu'une seule fois !");

    makeStep("4", "Récupérer votre Project ID",
             "Dans la console Scaleway :\n"
             "Project Dashboard → copier l'ID du projet affiché.\n"
             "Celui-ci sera demandé dans l'onglet Configuration.");

    auto *noteLbl = new QLabel(
        "ℹ  Une fois la VM créée, Docker sera automatiquement installé dessus "
        "et elle apparaîtra dans votre liste de VMs.", w);
    noteLbl->setStyleSheet(
        "font-size: 11px; color: #6366f1; background: #eef2ff;"
        "border-radius: 8px; padding: 10px 14px;");
    noteLbl->setWordWrap(true);
    layout->addWidget(noteLbl);
    layout->addStretch();

    scroll->setWidget(w);
    return scroll;
}

QWidget *CreateVMDialog::buildConfigTab()
{
    auto *w    = new QWidget;
    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(8, 16, 8, 8);
    root->setSpacing(14);

    auto *card = new QFrame(w);
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

    m_vmName = new QLineEdit(card);
    m_vmName->setPlaceholderText("mon-serveur-jeux");
    m_vmName->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Nom de la VM"), m_vmName);

    m_apiToken = new QLineEdit(card);
    m_apiToken->setPlaceholderText("scw-xxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    m_apiToken->setEchoMode(QLineEdit::Password);
    m_apiToken->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Token API Scaleway"), m_apiToken);

    m_projectId = new QLineEdit(card);
    m_projectId->setPlaceholderText("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
    m_projectId->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Project ID"), m_projectId);

    m_region = new QComboBox(card);
    m_region->addItems({
        "fr-par-1  (Paris 1)",
        "fr-par-2  (Paris 2)",
        "fr-par-3  (Paris 3)",
        "nl-ams-1  (Amsterdam 1)",
        "nl-ams-2  (Amsterdam 2)",
        "pl-waw-1  (Varsovie 1)",
        "pl-waw-2  (Varsovie 2)",
    });
    m_region->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Zone / Région"), m_region);

    m_instanceType = new QComboBox(card);
    m_instanceType->addItems({
        "DEV1-S   — 2 vCPU, 2 GB RAM, 20 GB  (~2.88 €/mois)",
        "DEV1-M   — 3 vCPU, 4 GB RAM, 40 GB  (~5.52 €/mois)",
        "DEV1-L   — 4 vCPU, 8 GB RAM, 80 GB  (~10.68 €/mois)",
        "DEV1-XL  — 4 vCPU, 12 GB RAM, 120 GB (~15.48 €/mois)",
        "GP1-XS   — 4 vCPU, 16 GB RAM, 150 GB (~25.92 €/mois)",
        "GP1-S    — 8 vCPU, 32 GB RAM, 300 GB (~51.84 €/mois)",
        "GP1-M    — 16 vCPU, 64 GB RAM, 600 GB (~103.68 €/mois)",
    });
    m_instanceType->setStyleSheet(INPUT_STYLE);
    form->addRow(mkL("Type de VM"), m_instanceType);

    auto *keyRow    = new QWidget(card);
    auto *keyLayout = new QHBoxLayout(keyRow);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    keyLayout->setSpacing(8);

    m_sshKeyPath = new QLineEdit(keyRow);
    m_sshKeyPath->setPlaceholderText("~/.ssh/id_ed25519");
    m_sshKeyPath->setStyleSheet(INPUT_STYLE);

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
    connect(browseBtn, &QPushButton::clicked, this, &CreateVMDialog::onBrowseKey);
    keyLayout->addWidget(m_sshKeyPath);
    keyLayout->addWidget(browseBtn);
    form->addRow(mkL("Clé SSH privée"), keyRow);

    root->addWidget(card);
    root->addStretch();
    return w;
}

static QString extractZoneCode(const QComboBox *cb) {
    // "fr-par-1  (Paris 1)" → "fr-par-1"
    return cb->currentText().split("  ").first().trimmed();
}

static QString extractInstanceCode(const QComboBox *cb) {
    // "DEV1-S   — 2 vCPU…" → "DEV1-S"
    return cb->currentText().split("   ").first().trimmed();
}

void CreateVMDialog::onCreate()
{
    if (m_apiToken->text().trimmed().isEmpty() ||
        m_vmName->text().trimmed().isEmpty()) {
        m_statusLabel->setStyleSheet("font-size: 12px; color: #ef4444;");
        m_statusLabel->setText("❌ Veuillez renseigner le Token API et le nom de la VM.");
        return;
    }

    m_createBtn->setEnabled(false);
    m_statusLabel->setStyleSheet("font-size: 12px; color: #6366f1;");
    m_statusLabel->setText("⏳ Connexion à Scaleway…");

    VMInstance config;
    config.name         = m_vmName->text().trimmed();
    config.scaApiToken  = m_apiToken->text().trimmed();
    config.region       = extractZoneCode(m_region);
    config.instanceType = extractInstanceCode(m_instanceType);
    config.sshKeyPath   = m_sshKeyPath->text().trimmed();
    config.sshUser      = "root";
    config.provider     = VMProvider::Scaleway;

    if (!m_provider) {
        m_provider = new ScalewayProvider(this);
        connect(m_provider, &ScalewayProvider::progressUpdate,
                this, [this](const QString &msg) {
            m_statusLabel->setStyleSheet("font-size: 12px; color: #6366f1;");
            m_statusLabel->setText("⏳ " + msg);
        });
        connect(m_provider, &ScalewayProvider::instanceCreated,
                this, [this](const VMInstance &vm) {
            m_result = vm;
            m_statusLabel->setStyleSheet("font-size: 12px; color: #22c55e; font-weight: 600;");
            m_statusLabel->setText("✅ VM créée et Docker installé — " + vm.host);
            accept();
        });
        connect(m_provider, &ScalewayProvider::error,
                this, [this](const QString &msg) {
            m_statusLabel->setStyleSheet("font-size: 12px; color: #ef4444;");
            m_statusLabel->setText("❌ " + msg);
            m_createBtn->setEnabled(true);
        });
    }

    m_provider->createInstance(config);
}

void CreateVMDialog::onBrowseKey()
{
    QString path = QFileDialog::getOpenFileName(this, "Sélectionner la clé SSH privée",
                                                QDir::homePath());
    if (!path.isEmpty())
        m_sshKeyPath->setText(path);
}
