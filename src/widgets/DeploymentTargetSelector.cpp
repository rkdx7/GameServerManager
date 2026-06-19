#include "DeploymentTargetSelector.h"
#include "DockerManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSettings>

DeploymentTargetSelector::DeploymentTargetSelector(const QString &gameKey,
                                                    DockerManager *docker,
                                                    const QVector<VMInstance> *vmList,
                                                    QWidget *parent)
    : QWidget(parent), m_gameKey(gameKey), m_docker(docker), m_vmList(vmList)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background: #eef2ff; border-radius: 8px;");
    setFixedHeight(44);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 14, 0);
    layout->setSpacing(10);

    auto *icon = new QLabel("☁", this);
    icon->setStyleSheet("font-size: 16px; background: transparent;");

    auto *label = new QLabel("Déploiement :", this);
    label->setStyleSheet("font-size: 12px; font-weight: 600; color: #4f46e5; background: transparent;");

    m_combo = new QComboBox(this);
    m_combo->setStyleSheet(R"(
        QComboBox {
            border: none; background: transparent;
            font-size: 12px; font-weight: 600; color: #4f46e5;
            padding: 2px 8px; min-height: 24px;
        }
        QComboBox::drop-down { border: none; width: 16px; }
        QComboBox QAbstractItemView {
            background: #ffffff; color: #1e293b;
            selection-background-color: #6366f1; selection-color: #ffffff;
            border: 1px solid #e2e8f0; border-radius: 4px; outline: none;
            font-size: 12px;
        }
    )");
    m_combo->setCursor(Qt::PointingHandCursor);

    layout->addWidget(icon);
    layout->addWidget(label);
    layout->addWidget(m_combo, 1);

    refreshList();

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { applySelection(); });
}

void DeploymentTargetSelector::refreshList()
{
    QSettings settings("GameServerManager", "App");
    QString savedId = settings.value("deployment/" + m_gameKey + "/target", "local").toString();

    m_combo->blockSignals(true);
    m_combo->clear();

    for (int i = 0; i < m_vmList->size(); ++i) {
        const auto &vm = (*m_vmList)[i];
        QString icon;
        switch (vm.provider) {
        case VMProvider::Local:    icon = "🖥  "; break;
        case VMProvider::Scaleway: icon = "☁  "; break;
        default:                   icon = "🌐  "; break;
        }
        m_combo->addItem(icon + vm.name, vm.id);
    }

    // Restore saved selection
    int idx = 0;
    for (int i = 0; i < m_combo->count(); ++i) {
        if (m_combo->itemData(i).toString() == savedId) {
            idx = i;
            break;
        }
    }
    m_combo->setCurrentIndex(idx);
    m_combo->blockSignals(false);

    applySelection();
}

const VMInstance *DeploymentTargetSelector::selectedVM() const
{
    int idx = m_combo->currentIndex();
    if (idx < 0 || idx >= m_vmList->size()) return nullptr;
    const VMInstance &vm = (*m_vmList)[idx];
    if (vm.provider == VMProvider::Local) return nullptr;
    return &vm;
}

void DeploymentTargetSelector::setTargetById(const QString &vmId)
{
    m_combo->blockSignals(true);
    int targetIdx = 0;
    for (int i = 0; i < m_combo->count(); ++i) {
        if (m_combo->itemData(i).toString() == vmId) { targetIdx = i; break; }
    }
    m_combo->setCurrentIndex(targetIdx);
    m_combo->blockSignals(false);
    applySelection();
}

void DeploymentTargetSelector::applySelection()
{
    const VMInstance *vm = selectedVM();

    // Persist selection
    QSettings settings("GameServerManager", "App");
    settings.setValue("deployment/" + m_gameKey + "/target",
                      vm ? vm->id : "local");

    // Configure docker
    if (!vm) {
        m_docker->setLocalMode();
    } else {
        m_docker->setRemoteMode(vm->host, vm->sshPort, vm->sshUser, vm->sshKeyPath);
    }

    emit targetChanged(vm);
}
