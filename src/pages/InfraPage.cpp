#include "InfraPage.h"
#include "VMStorage.h"
#include "AddCustomVMDialog.h"
#include "CreateVMDialog.h"
#include "VMAdminPage.h"
#include "RemoteShell.h"

#include <QVBoxLayout>
#include <QThread>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>

namespace {
const char *CARD_STYLE = R"(
    QFrame {
        background: #ffffff;
        border-radius: 14px;
    }
)";
const char *BTN_PRIMARY = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
            stop:0 #6366f1, stop:1 #818cf8);
        color: white; border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 14px; min-height: 32px;
    }
    QPushButton:hover {
        background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
            stop:0 #4f46e5, stop:1 #6366f1);
    }
)";
const char *BTN_SECONDARY = R"(
    QPushButton {
        background: #eef2ff; color: #4f46e5;
        border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 14px; min-height: 32px;
    }
    QPushButton:hover { background: #e0e7ff; }
)";
const char *BTN_DANGER = R"(
    QPushButton {
        background: #fef2f2; color: #ef4444;
        border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 14px; min-height: 32px;
    }
    QPushButton:hover { background: #fee2e2; }
)";
} // namespace

InfraPage::InfraPage(QVector<VMInstance> *vmList, QWidget *parent)
    : QWidget(parent), m_vmList(vmList)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout;
    auto *iconLbl   = new QLabel("☁", this);
    iconLbl->setStyleSheet("font-size: 36px; background: transparent;");
    auto *titleCol  = new QVBoxLayout;
    auto *titleLbl  = new QLabel("Infrastructure", this);
    titleLbl->setStyleSheet("font-size: 24px; font-weight: 700; color: #1e293b; background: transparent;");
    auto *subLbl    = new QLabel("Gérez vos VMs et déployez vos serveurs de jeux.", this);
    subLbl->setStyleSheet("font-size: 13px; color: #64748b; background: transparent;");
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(subLbl);
    headerRow->addWidget(iconLbl);
    headerRow->addSpacing(12);
    headerRow->addLayout(titleCol, 1);

    auto *addCustomBtn = new QPushButton("🖥  Ajouter une VM", this);
    addCustomBtn->setFixedHeight(40);
    addCustomBtn->setCursor(Qt::PointingHandCursor);
    addCustomBtn->setStyleSheet(BTN_SECONDARY);
    connect(addCustomBtn, &QPushButton::clicked, this, &InfraPage::onAddCustomVM);

    auto *createScwBtn = new QPushButton("☁  Créer sur Scaleway", this);
    createScwBtn->setFixedHeight(40);
    createScwBtn->setCursor(Qt::PointingHandCursor);
    createScwBtn->setStyleSheet(BTN_PRIMARY);
    connect(createScwBtn, &QPushButton::clicked, this, &InfraPage::onCreateScalewayVM);

    headerRow->addWidget(addCustomBtn);
    headerRow->addSpacing(8);
    headerRow->addWidget(createScwBtn);
    root->addLayout(headerRow);
    root->addSpacing(20);

    // ── Prerequisites info banner ─────────────────────────────────────────
    auto *infoBanner = new QFrame(this);
    infoBanner->setAttribute(Qt::WA_StyledBackground, true);
    infoBanner->setStyleSheet(
        "QFrame { background: #fffbeb; border-radius: 10px; border: 1px solid #fde68a; }");
    auto *bannerLayout = new QHBoxLayout(infoBanner);
    bannerLayout->setContentsMargins(16, 12, 16, 12);
    auto *bannerIcon = new QLabel("💡", infoBanner);
    bannerIcon->setStyleSheet("font-size: 16px; background: transparent;");
    auto *bannerText = new QLabel(
        "<b>Scaleway :</b> Avant de créer une VM, vous aurez besoin d'un compte Scaleway, "
        "d'un token API et d'une clé SSH enregistrée. Cliquez sur "
        "<i>Créer sur Scaleway</i> pour voir les prérequis détaillés.",
        infoBanner);
    bannerText->setStyleSheet("font-size: 12px; color: #92400e; background: transparent;");
    bannerText->setWordWrap(true);
    bannerLayout->addWidget(bannerIcon);
    bannerLayout->addWidget(bannerText, 1);
    root->addWidget(infoBanner);
    root->addSpacing(20);

    // ── VM Cards scrollable area ──────────────────────────────────────────
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent; border: none;");

    m_cardsArea = new QWidget;
    m_cardsArea->setStyleSheet("background: transparent;");
    m_cardsLayout = new QVBoxLayout(m_cardsArea);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(12);
    m_cardsLayout->addStretch();

    scrollArea->setWidget(m_cardsArea);
    root->addWidget(scrollArea, 1);

    refreshVMList();
}

QWidget *InfraPage::buildVMCard(const VMInstance &vm, int index)
{
    auto *card = new QFrame(this);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(CARD_STYLE);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    // Status dot
    QString dotColor;
    switch (vm.status) {
    case VMStatus::Running:      dotColor = "#22c55e"; break;
    case VMStatus::Provisioning: dotColor = "#f59e0b"; break;
    case VMStatus::Stopped:      dotColor = "#ef4444"; break;
    default:                     dotColor = "#94a3b8"; break;
    }
    auto *dot = new QLabel("●", card);
    dot->setStyleSheet(QString("font-size: 14px; color: %1; background: transparent;").arg(dotColor));

    // VM info
    auto *infoCol = new QVBoxLayout;
    infoCol->setSpacing(3);

    auto *nameLbl = new QLabel(vm.name, card);
    nameLbl->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b; background: transparent;");

    QString detailStr;
    switch (vm.provider) {
    case VMProvider::Local:
        detailStr = "🖥  Local Docker";
        break;
    case VMProvider::Scaleway:
        detailStr = "☁  Scaleway · " + vm.region + " · " + vm.instanceType;
        break;
    default:
        detailStr = "🌐  VM distante · " + vm.host + ":" + QString::number(vm.sshPort);
        break;
    }
    auto *detailLbl = new QLabel(detailStr, card);
    detailLbl->setStyleSheet("font-size: 12px; color: #64748b; background: transparent;");

    QString serversStr = vm.deployedServers.isEmpty()
        ? "Aucun serveur déployé"
        : "Serveurs : " + vm.deployedServers.join(", ");
    auto *serversLbl = new QLabel(serversStr, card);
    serversLbl->setStyleSheet("font-size: 11px; color: #94a3b8; background: transparent;");

    infoCol->addWidget(nameLbl);
    infoCol->addWidget(detailLbl);
    infoCol->addWidget(serversLbl);

    layout->addWidget(dot);
    layout->addLayout(infoCol, 1);

    // Action buttons
    auto *adminBtn = new QPushButton("⚙  Administrer", card);
    adminBtn->setFixedHeight(34);
    adminBtn->setCursor(Qt::PointingHandCursor);
    adminBtn->setStyleSheet(BTN_SECONDARY);
    connect(adminBtn, &QPushButton::clicked, this, [this, index]() {
        onAdminVM(index);
    });
    layout->addWidget(adminBtn);

    // No delete for Local
    if (vm.provider != VMProvider::Local) {
        auto *delBtn = new QPushButton("🗑", card);
        delBtn->setFixedSize(34, 34);
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setStyleSheet(BTN_DANGER);
        connect(delBtn, &QPushButton::clicked, this, [this, index]() {
            onDeleteVM(index);
        });
        layout->addWidget(delBtn);
    }

    return card;
}

void InfraPage::refreshVMList()
{
    // Remove all cards (keep the stretch at end)
    while (m_cardsLayout->count() > 1) {
        auto *item = m_cardsLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (int i = 0; i < m_vmList->size(); ++i) {
        m_cardsLayout->insertWidget(i, buildVMCard((*m_vmList)[i], i));
    }
}

void InfraPage::onAddCustomVM()
{
    AddCustomVMDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    VMInstance vm = dlg.result();
    if (vm.name.isEmpty() || vm.host.isEmpty()) return;

    m_vmList->append(vm);
    VMStorage::save(*m_vmList);
    refreshVMList();
    emit vmListChanged();

    // For a non-root account, Docker commands require the user to be in the
    // `docker` group. Try to add them automatically (best-effort: only works if
    // Docker is already installed and a sudo password was provided). If Docker
    // isn't installed yet, the Install Docker step does this again.
    if (!vm.isRoot()) {
        QString sudoNote = vm.sudoPassword.isEmpty()
            ? "\n\nAucun mot de passe sudo n'a été enregistré : si l'ajout échoue, "
              "utilisez le bouton « Installer Docker » dans la console d'administration, "
              "qui vous guidera."
            : "";
        QMessageBox::information(this, "Utilisateur non-root",
            "L'utilisateur <b>" + vm.sshUser + "</b> n'est pas root. Pour lancer des "
            "commandes Docker sans sudo, il doit appartenir au groupe <b>docker</b>.<br><br>"
            "Tentative d'ajout automatique au groupe en arrière-plan."
            + sudoNote.toHtmlEscaped());

        VMInstance vmCopy = vm;
        auto *t = QThread::create([vmCopy]() {
            RemoteShell::run(vmCopy,
                RemoteShell::privileged(vmCopy, "usermod -aG docker " + vmCopy.sshUser),
                30000);
        });
        t->start();
        connect(t, &QThread::finished, t, &QThread::deleteLater);
    }
}

void InfraPage::onCreateScalewayVM()
{
    CreateVMDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    VMInstance vm = dlg.createdVM();
    if (vm.id.isEmpty()) return;

    m_vmList->append(vm);
    VMStorage::save(*m_vmList);
    refreshVMList();
    emit vmListChanged();
}

void InfraPage::onAdminVM(int index)
{
    if (index < 0 || index >= m_vmList->size()) return;
    auto *page = new VMAdminPage((*m_vmList)[index], this);
    page->show();
}

void InfraPage::onDeleteVM(int index)
{
    if (index < 0 || index >= m_vmList->size()) return;
    if ((*m_vmList)[index].provider == VMProvider::Local) return;

    auto ret = QMessageBox::question(this, "Supprimer la VM",
        "Supprimer <b>" + (*m_vmList)[index].name + "</b> de la liste ?\n"
        "(La VM distante ne sera pas supprimée chez le cloud provider.)",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    m_vmList->removeAt(index);
    VMStorage::save(*m_vmList);
    refreshVMList();
    emit vmListChanged();
}
