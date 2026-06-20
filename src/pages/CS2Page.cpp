#include "CS2Page.h"
#include "DockerManager.h"
#include "ServerDashboard.h"
#include "ImagePickerDialog.h"
#include "DeploymentTargetSelector.h"
#include "GameBanner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QThread>
#include <QListWidget>
#include <QSettings>
#include <QUuid>
#include <QTextEdit>
#include <QFile>
#include <QDir>

namespace {
const char *INPUT_STYLE = R"(
    QLineEdit, QComboBox, QSpinBox {
        border: 2px solid #e2e8f0;
        border-radius: 8px;
        padding: 8px 12px;
        font-size: 13px;
        color: #1e293b;
        background: #f8fafc;
        min-height: 20px;
    }
    QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
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
static const char *DEFAULT_IMAGE = "joedwards32/cs2";
static const char *GAME_KEY      = "cs2-server";
} // namespace

// ── Persistence ───────────────────────────────────────────────────────────────

void CS2Page::loadInstances()
{
    QSettings s("GameServerManager", "App");
    const QString base = QString("instances/") + GAME_KEY;
    int count = s.value(base + "/count", 0).toInt();

    m_instances.clear();
    for (int i = 0; i < count; ++i) {
        const QString pfx = base + "/" + QString::number(i);
        ServerInstanceConfig inst;
        inst.id            = s.value(pfx + "/id").toString();
        inst.displayName   = s.value(pfx + "/displayName").toString();
        inst.containerName = s.value(pfx + "/containerName").toString();
        inst.port          = s.value(pfx + "/port", 27015).toInt();
        inst.vmId          = s.value(pfx + "/vmId", "local").toString();
        inst.imageOverride        = s.value(pfx + "/imageOverride").toString();
        inst.customConfigEnabled  = s.value(pfx + "/customConfigEnabled", false).toBool();
        inst.customConfig         = s.value(pfx + "/customConfig").toString();

        int fc = s.value(pfx + "/fields/count", 0).toInt();
        for (int j = 0; j < fc; ++j) {
            QString key = s.value(pfx + "/fields/" + QString::number(j) + "/key").toString();
            QString val = s.value(pfx + "/fields/" + QString::number(j) + "/value").toString();
            if (!key.isEmpty()) inst.fieldValues[key] = val;
        }

        if (!inst.id.isEmpty() && !inst.containerName.isEmpty())
            m_instances.append(inst);
    }
}

void CS2Page::saveInstances()
{
    QSettings s("GameServerManager", "App");
    const QString base = QString("instances/") + GAME_KEY;
    s.remove(base);
    s.setValue(base + "/count", m_instances.size());

    for (int i = 0; i < m_instances.size(); ++i) {
        const auto &inst = m_instances[i];
        const QString pfx = base + "/" + QString::number(i);
        s.setValue(pfx + "/id",                  inst.id);
        s.setValue(pfx + "/displayName",         inst.displayName);
        s.setValue(pfx + "/containerName",       inst.containerName);
        s.setValue(pfx + "/port",                inst.port);
        s.setValue(pfx + "/vmId",                inst.vmId);
        s.setValue(pfx + "/imageOverride",       inst.imageOverride);
        s.setValue(pfx + "/customConfigEnabled", inst.customConfigEnabled);
        s.setValue(pfx + "/customConfig",        inst.customConfig);

        QStringList keys = inst.fieldValues.keys();
        s.setValue(pfx + "/fields/count", keys.size());
        for (int j = 0; j < keys.size(); ++j) {
            s.setValue(pfx + "/fields/" + QString::number(j) + "/key",   keys[j]);
            s.setValue(pfx + "/fields/" + QString::number(j) + "/value", inst.fieldValues[keys[j]]);
        }
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────

CS2Page::CS2Page(DockerManager *docker,
                 const QVector<VMInstance> *vmList,
                 QWidget *parent)
    : QWidget(parent), m_docker(docker), m_vmList(vmList)
{
    loadInstances();
    if (m_instances.isEmpty()) {
        ServerInstanceConfig def;
        def.id            = QUuid::createUuid().toString(QUuid::WithoutBraces);
        def.displayName   = GAME_KEY;
        def.containerName = GAME_KEY;
        def.port          = 27015;
        def.vmId          = "local";
        def.fieldValues   = {{"gslt",""},{"rconPass","rcon123"},
                              {"gameMode","Compétitif"},{"startMap","de_dust2"}};
        m_instances.append(def);
        saveInstances();
    }

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    if (m_vmList) {
        m_targetSelector = new DeploymentTargetSelector(GAME_KEY, m_docker, m_vmList, this);
        root->addWidget(m_targetSelector);
        connect(m_targetSelector, &DeploymentTargetSelector::targetChanged,
                this, [this](const VMInstance *vm) {
            if (m_currentInstanceIdx >= 0 && m_currentInstanceIdx < m_instances.size()) {
                m_instances[m_currentInstanceIdx].vmId = vm ? vm->id : QString("local");
                saveInstances();
            }
        });
    }

    auto *body       = new QWidget;
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    bodyLayout->addWidget(buildInstancePanel());

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(buildInstallForm());
    bodyLayout->addWidget(m_stack, 1);

    root->addWidget(body, 1);

    m_instanceList->setCurrentRow(0);
}

// ── Instance panel ────────────────────────────────────────────────────────────

QWidget *CS2Page::buildInstancePanel()
{
    auto *panel = new QWidget;
    panel->setFixedWidth(185);
    panel->setAttribute(Qt::WA_StyledBackground, true);
    panel->setStyleSheet("QWidget { background: #f8fafc; border-right: 1px solid #e2e8f0; }");

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 14, 10, 14);
    layout->setSpacing(6);

    auto *titleLbl = new QLabel("SERVEURS");
    titleLbl->setStyleSheet(
        "font-size: 10px; font-weight: 700; color: #94a3b8; "
        "background: transparent; letter-spacing: 1px;");
    layout->addWidget(titleLbl);
    layout->addSpacing(2);

    m_instanceList = new QListWidget;
    m_instanceList->setStyleSheet(R"(
        QListWidget { background: transparent; border: none; outline: none; font-size: 12px; }
        QListWidget::item { padding: 7px 10px; border-radius: 6px; color: #374151; }
        QListWidget::item:selected { background: #eef2ff; color: #4f46e5; font-weight: 600; }
        QListWidget::item:hover:!selected { background: #f1f5f9; }
    )");
    for (const auto &inst : m_instances)
        m_instanceList->addItem(inst.displayName);
    layout->addWidget(m_instanceList, 1);
    layout->addSpacing(6);

    auto *addBtn = new QPushButton("+ Nouveau");
    addBtn->setFixedHeight(32);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(R"(
        QPushButton { background: #6366f1; color: white; border: none; border-radius: 7px;
                      font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: #4f46e5; }
    )");
    connect(addBtn, &QPushButton::clicked, this, &CS2Page::addInstance);
    layout->addWidget(addBtn);

    m_deleteInstanceBtn = new QPushButton("Supprimer");
    m_deleteInstanceBtn->setFixedHeight(28);
    m_deleteInstanceBtn->setCursor(Qt::PointingHandCursor);
    m_deleteInstanceBtn->setStyleSheet(R"(
        QPushButton { background: #fee2e2; color: #dc2626; border: none; border-radius: 7px;
                      font-size: 11px; font-weight: 600; }
        QPushButton:hover { background: #fecaca; }
        QPushButton:disabled { background: #f1f5f9; color: #cbd5e1; }
    )");
    connect(m_deleteInstanceBtn, &QPushButton::clicked, this, [this]() {
        int idx = m_instanceList->currentRow();
        if (idx >= 0) deleteInstance(idx);
    });
    layout->addWidget(m_deleteInstanceBtn);

    connect(m_instanceList, &QListWidget::currentRowChanged,
            this, &CS2Page::switchToInstance);

    updateInstanceButtons();
    return panel;
}

void CS2Page::updateInstanceButtons()
{
    if (m_deleteInstanceBtn)
        m_deleteInstanceBtn->setEnabled(m_instances.size() > 1);
}

// ── Instance switching ────────────────────────────────────────────────────────

void CS2Page::saveCurrentFormState()
{
    if (m_currentInstanceIdx < 0 || m_currentInstanceIdx >= m_instances.size()) return;
    auto &inst = m_instances[m_currentInstanceIdx];
    inst.containerName = m_serverName->text();
    inst.port          = m_port->value();
    inst.fieldValues["gslt"]     = m_gslt->text();
    inst.fieldValues["rconPass"] = m_rconPass->text();
    inst.fieldValues["gameMode"] = m_gameMode->currentText();
    inst.fieldValues["startMap"] = m_startMap->currentText();
    inst.imageOverride           = m_imageOverride;
    inst.customConfigEnabled     = m_advancedToggleBtn && m_advancedToggleBtn->isChecked();
    inst.customConfig            = m_customConfigEdit  ? m_customConfigEdit->toPlainText() : QString();
    saveInstances();
}

void CS2Page::loadFormFromInstance(const ServerInstanceConfig &inst)
{
    m_serverName->setText(inst.containerName);
    m_port->setValue(inst.port > 0 ? inst.port : 27015);
    m_gslt->setText(inst.fieldValues.value("gslt", ""));
    m_rconPass->setText(inst.fieldValues.value("rconPass", "rcon123"));

    auto setCombo = [](QComboBox *cb, const QString &val) {
        int i = cb->findText(val);
        if (i >= 0) cb->setCurrentIndex(i);
    };
    setCombo(m_gameMode, inst.fieldValues.value("gameMode", "Compétitif"));
    setCombo(m_startMap, inst.fieldValues.value("startMap", "de_dust2"));

    m_imageOverride = inst.imageOverride;
    if (m_imageBadge) {
        const QString label    = m_imageOverride.isEmpty() ? QString(DEFAULT_IMAGE) : m_imageOverride;
        const bool    overridden = !m_imageOverride.isEmpty() && m_imageOverride != QString(DEFAULT_IMAGE);
        m_imageBadge->setText((overridden ? "🔄 " : "🐳 ") + label);
        m_imageBadge->setStyleSheet(overridden
            ? "font-size: 10px; color: #4f46e5; font-weight: 600; background: transparent;"
            : "font-size: 10px; color: #94a3b8; background: transparent;");
    }

    if (m_advancedToggleBtn) m_advancedToggleBtn->setChecked(inst.customConfigEnabled);
    if (m_advancedContent)   m_advancedContent->setVisible(inst.customConfigEnabled);
    if (m_customConfigEdit)  m_customConfigEdit->setPlainText(inst.customConfig);
}

void CS2Page::switchToInstance(int idx)
{
    if (idx < 0 || idx >= m_instances.size()) return;

    if (m_currentInstanceIdx >= 0 && m_currentInstanceIdx < m_instances.size()) {
        const QString &prevId = m_instances[m_currentInstanceIdx].id;
        if (m_dashboards.contains(prevId))
            m_dashboards[prevId]->stopPolling();
    }

    saveCurrentFormState();
    m_currentInstanceIdx = idx;

    const ServerInstanceConfig &inst = m_instances[idx];
    if (m_targetSelector)
        m_targetSelector->setTargetById(inst.vmId.isEmpty() ? "local" : inst.vmId);

    loadFormFromInstance(inst);
    checkStatus();
}

void CS2Page::addInstance()
{
    saveCurrentFormState();

    ServerInstanceConfig inst;
    inst.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const int n = m_instances.size() + 1;
    inst.containerName = QString(GAME_KEY) + "-" + QString::number(n);
    inst.displayName   = inst.containerName;
    inst.port          = 27015 + n - 1;
    inst.vmId          = (m_targetSelector && m_targetSelector->selectedVM())
                         ? m_targetSelector->selectedVM()->id : QString("local");
    inst.fieldValues   = {{"gslt",""},{"rconPass","rcon123"},
                           {"gameMode","Compétitif"},{"startMap","de_dust2"}};

    m_instances.append(inst);
    m_instanceList->addItem(inst.displayName);
    saveInstances();
    updateInstanceButtons();
    m_instanceList->setCurrentRow(m_instances.size() - 1);
}

void CS2Page::deleteInstance(int idx)
{
    if (idx < 0 || idx >= m_instances.size() || m_instances.size() <= 1) return;

    const QString id = m_instances[idx].id;
    if (m_dashboards.contains(id)) {
        m_dashboards[id]->stopPolling();
        m_stack->removeWidget(m_dashboards[id]);
        m_dashboards[id]->deleteLater();
        m_dashboards.remove(id);
    }

    const bool deletingCurrent = (idx == m_currentInstanceIdx);
    if (deletingCurrent)
        m_currentInstanceIdx = -1;
    else if (idx < m_currentInstanceIdx)
        m_currentInstanceIdx--;

    m_instances.removeAt(idx);
    delete m_instanceList->takeItem(idx);
    saveInstances();
    updateInstanceButtons();

    if (deletingCurrent) {
        m_instanceList->setCurrentRow(qMin(idx, m_instances.size() - 1));
    } else {
        m_instanceList->blockSignals(true);
        m_instanceList->setCurrentRow(m_currentInstanceIdx);
        m_instanceList->blockSignals(false);
    }
}

// ── Install form ──────────────────────────────────────────────────────────────

QWidget *CS2Page::buildInstallForm() {
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");

    auto *container = new QWidget;
    container->setStyleSheet("background: transparent;");

    auto *outer = new QVBoxLayout(container);
    outer->setContentsMargins(40, 32, 40, 32);
    outer->setSpacing(0);

    auto *gearBtn = new QPushButton("⚙  Image Docker", container);
    gearBtn->setFixedHeight(34);
    gearBtn->setCursor(Qt::PointingHandCursor);
    gearBtn->setStyleSheet(R"(
        QPushButton { background: #eef2ff; color: #4f46e5; border: none;
                      border-radius: 8px; font-size: 12px; font-weight: 600; padding: 0 14px; }
        QPushButton:hover { background: #e0e7ff; }
    )");
    connect(gearBtn, &QPushButton::clicked, this, &CS2Page::openImagePicker);

    m_imageBadge = new QLabel(QString("🐳 ") + DEFAULT_IMAGE, container);
    m_imageBadge->setStyleSheet("font-size: 10px; color: #94a3b8; background: transparent;");
    m_imageBadge->setAlignment(Qt::AlignRight);

    auto *gearCol = new QVBoxLayout;
    gearCol->setSpacing(4);
    gearCol->addWidget(gearBtn, 0, Qt::AlignRight);
    gearCol->addWidget(m_imageBadge);

    auto *topRow = new QHBoxLayout;
    topRow->addStretch();
    topRow->addLayout(gearCol);
    outer->addLayout(topRow);
    outer->addSpacing(8);

    auto *gsltNote = new QLabel("ℹ Le token GSLT est requis pour les serveurs publics. Laissez vide pour LAN.", container);
    gsltNote->setStyleSheet("font-size: 12px; color: #94a3b8; background: transparent;");
    gsltNote->setWordWrap(true);

    outer->addWidget(buildGameBanner(
        "🔫", "CS2 / CS:GO Server",
        "Déployez un serveur CS2 dédié via Docker (image joedwards32/cs2).",
        "#ea580c", "#c2410c", ":/games/cs2.png", container));
    outer->addSpacing(12);
    outer->addWidget(gsltNote);
    outer->addSpacing(24);

    auto *card = new QFrame(container);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("QFrame { background: #ffffff; border-radius: 14px; }");

    auto *form = new QFormLayout(card);
    form->setContentsMargins(28, 24, 28, 24);
    form->setSpacing(14);
    form->setLabelAlignment(Qt::AlignLeft);

    auto mkLabel = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet(LABEL_STYLE);
        return l;
    };

    m_serverName = new QLineEdit(card);
    m_serverName->setText(GAME_KEY);
    m_serverName->setStyleSheet(INPUT_STYLE);

    m_gslt = new QLineEdit(card);
    m_gslt->setPlaceholderText("Optionnel — token Steam Game Server Login");
    m_gslt->setStyleSheet(INPUT_STYLE);

    m_rconPass = new QLineEdit(card);
    m_rconPass->setText("rcon123");
    m_rconPass->setEchoMode(QLineEdit::Password);
    m_rconPass->setStyleSheet(INPUT_STYLE);

    m_gameMode = new QComboBox(card);
    m_gameMode->addItems({"Compétitif", "Casual", "Deathmatch"});
    m_gameMode->setStyleSheet(INPUT_STYLE);

    m_startMap = new QComboBox(card);
    m_startMap->addItems({"de_dust2", "de_inferno", "de_mirage",
                           "de_nuke", "de_ancient", "de_overpass"});
    m_startMap->setStyleSheet(INPUT_STYLE);

    m_port = new QSpinBox(card);
    m_port->setRange(1024, 65535);
    m_port->setValue(27015);
    m_port->setStyleSheet(INPUT_STYLE);

    form->addRow(mkLabel("Nom du conteneur"), m_serverName);
    form->addRow(mkLabel("Token GSLT"),       m_gslt);
    form->addRow(mkLabel("Mot de passe RCON"),m_rconPass);
    form->addRow(mkLabel("Mode de jeu"),      m_gameMode);
    form->addRow(mkLabel("Map de départ"),    m_startMap);
    form->addRow(mkLabel("Port"),             m_port);

    outer->addWidget(card);
    outer->addSpacing(16);

    // ── Advanced settings ─────────────────────────────────────────────────────
    auto *advCard = new QFrame(container);
    advCard->setAttribute(Qt::WA_StyledBackground, true);
    advCard->setStyleSheet("QFrame { background: #ffffff; border-radius: 14px; }");

    auto *advOuter = new QVBoxLayout(advCard);
    advOuter->setContentsMargins(28, 16, 28, 16);
    advOuter->setSpacing(0);

    auto *advHeaderRow = new QHBoxLayout;
    advHeaderRow->setSpacing(8);

    m_advancedToggleBtn = new QPushButton("▶", advCard);
    m_advancedToggleBtn->setFixedSize(22, 22);
    m_advancedToggleBtn->setCheckable(true);
    m_advancedToggleBtn->setCursor(Qt::PointingHandCursor);
    m_advancedToggleBtn->setStyleSheet(R"(
        QPushButton { background: #f1f5f9; color: #64748b; border: none;
                      border-radius: 5px; font-size: 9px; font-weight: 700; }
        QPushButton:checked { background: #eef2ff; color: #4f46e5; }
        QPushButton:hover { background: #e2e8f0; }
    )");

    auto *advTitleLbl = new QLabel("Paramètres avancés", advCard);
    advTitleLbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151; background: transparent;");
    auto *advSubLbl = new QLabel("Configuration personnalisée du serveur", advCard);
    advSubLbl->setStyleSheet("font-size: 11px; color: #94a3b8; background: transparent;");

    auto *advTitleCol = new QVBoxLayout;
    advTitleCol->setSpacing(1);
    advTitleCol->addWidget(advTitleLbl);
    advTitleCol->addWidget(advSubLbl);

    advHeaderRow->addWidget(m_advancedToggleBtn);
    advHeaderRow->addLayout(advTitleCol);
    advHeaderRow->addStretch();
    advOuter->addLayout(advHeaderRow);

    m_advancedContent = new QWidget(advCard);
    m_advancedContent->setVisible(false);
    auto *advContentLayout = new QVBoxLayout(m_advancedContent);
    advContentLayout->setContentsMargins(0, 14, 0, 4);
    advContentLayout->setSpacing(10);

    auto *docLink = new QLabel(
        "<a href='https://developer.valvesoftware.com/wiki/Counter-Strike_2/Dedicated_Servers'"
        " style='color: #ef4444; font-weight: 600; text-decoration: none;'>"
        "📖 Documentation serveur CS2 →</a>",
        m_advancedContent);
    docLink->setTextFormat(Qt::RichText);
    docLink->setOpenExternalLinks(true);
    docLink->setStyleSheet("font-size: 13px; background: transparent;");
    advContentLayout->addWidget(docLink);

    m_customConfigEdit = new QTextEdit(m_advancedContent);
    m_customConfigEdit->setPlaceholderText(
        "Collez votre server.cfg ici...\n"
        "Exemple :\nhostname \"Mon serveur CS2\"\nsv_cheats 0\nmp_maxrounds 16");
    m_customConfigEdit->setMinimumHeight(180);
    m_customConfigEdit->setStyleSheet(R"(
        QTextEdit {
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            padding: 10px;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 12px;
            background: #f8fafc;
            color: #374151;
        }
        QTextEdit:focus { border-color: #ef4444; background: #ffffff; }
    )");
    advContentLayout->addWidget(m_customConfigEdit);

    advOuter->addWidget(m_advancedContent);
    outer->addWidget(advCard);
    outer->addSpacing(20);

    connect(m_advancedToggleBtn, &QPushButton::toggled,
            m_advancedContent, &QWidget::setVisible);
    connect(m_advancedToggleBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_advancedToggleBtn->setText(checked ? "▼" : "▶");
    });

    // ── Status & install button ───────────────────────────────────────────────
    m_installStatus = new QLabel("", container);
    m_installStatus->setStyleSheet("font-size: 13px; color: #64748b; background: transparent;");
    m_installStatus->setAlignment(Qt::AlignCenter);
    outer->addWidget(m_installStatus);
    outer->addSpacing(8);

    m_installBtn = new QPushButton("🚀  Installer le serveur CS2", container);
    m_installBtn->setFixedHeight(48);
    m_installBtn->setCursor(Qt::PointingHandCursor);
    m_installBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #ef4444, stop:1 #f59e0b);
            color: white; border: none; border-radius: 10px;
            font-size: 15px; font-weight: 700;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #dc2626, stop:1 #d97706);
        }
        QPushButton:disabled { background: #94a3b8; }
    )");
    connect(m_installBtn, &QPushButton::clicked, this, &CS2Page::onInstall);
    outer->addWidget(m_installBtn);
    outer->addStretch();

    scroll->setWidget(container);
    return scroll;
}

// ── Status check ──────────────────────────────────────────────────────────────

void CS2Page::checkStatus() {
    if (m_currentInstanceIdx < 0 || m_currentInstanceIdx >= m_instances.size()) return;
    const ServerInstanceConfig &inst = m_instances[m_currentInstanceIdx];

    if (m_docker->containerExists(inst.containerName)) {
        if (!m_dashboards.contains(inst.id)) {
            const QString rcon = inst.fieldValues.value("rconPass", "rcon123");
            auto *dash = new ServerDashboard(m_docker, inst.containerName, GameType::CS2, rcon, this);
            connect(dash, &ServerDashboard::uninstallRequested,
                    this, &CS2Page::onUninstall);
            m_stack->addWidget(dash);
            m_dashboards[inst.id] = dash;
        }
        m_stack->setCurrentWidget(m_dashboards[inst.id]);
        m_dashboards[inst.id]->startPolling();
    } else {
        m_stack->setCurrentIndex(0);
    }
}

// ── Image picker ──────────────────────────────────────────────────────────────

void CS2Page::openImagePicker()
{
    const QString current = m_imageOverride.isEmpty() ? QString(DEFAULT_IMAGE) : m_imageOverride;
    ImagePickerDialog dlg(m_docker, current, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_imageOverride = dlg.selectedImage();
    m_needsLogin    = dlg.requiresLogin();
    m_loginReg      = dlg.loginRegistry();
    m_loginUser     = dlg.loginUsername();
    m_loginPass     = dlg.loginPassword();

    if (m_imageBadge) {
        const QString label    = m_imageOverride.isEmpty() ? QString(DEFAULT_IMAGE) : m_imageOverride;
        const bool    overridden = !m_imageOverride.isEmpty() && m_imageOverride != QString(DEFAULT_IMAGE);
        m_imageBadge->setText((overridden ? "🔄 " : "🐳 ") + label);
        m_imageBadge->setStyleSheet(overridden
            ? "font-size: 10px; color: #4f46e5; font-weight: 600; background: transparent;"
            : "font-size: 10px; color: #94a3b8; background: transparent;");
    }

    if (m_currentInstanceIdx >= 0 && m_currentInstanceIdx < m_instances.size()) {
        m_instances[m_currentInstanceIdx].imageOverride = m_imageOverride;
        saveInstances();
    }
}

// ── Install / Uninstall ───────────────────────────────────────────────────────

void CS2Page::onInstall() {
    saveCurrentFormState();

    m_installBtn->setEnabled(false);
    m_installStatus->setStyleSheet("font-size: 13px; color: #6366f1; background: transparent;");
    m_installStatus->setText("⏳ Téléchargement de l'image CS2… (l'image fait ~30 GB, patience)");

    const ServerInstanceConfig inst = m_instances[m_currentInstanceIdx];
    const QString name         = inst.containerName;
    const QString gslt         = inst.fieldValues.value("gslt", "");
    const QString rcon         = inst.fieldValues.value("rconPass", "rcon123");
    const QString gmStr        = inst.fieldValues.value("gameMode", "Compétitif");
    const QString map          = inst.fieldValues.value("startMap", "de_dust2");
    const int     port         = inst.port;
    const QString image        = inst.imageOverride.isEmpty() ? QString(DEFAULT_IMAGE) : inst.imageOverride;
    const bool    customEnabled = inst.customConfigEnabled;
    const QString customConfig  = inst.customConfig;

    int gameType = 0, gameMode = 1;
    if (gmStr == "Casual")     { gameType = 0; gameMode = 0; }
    else if (gmStr == "Deathmatch") { gameType = 1; gameMode = 2; }

    bool    needsLogin = m_needsLogin;
    QString loginReg   = m_loginReg;
    QString loginUser  = m_loginUser;
    QString loginPass  = m_loginPass;

    DockerManager *docker = m_docker;
    auto *thread = QThread::create([this, docker, name, gslt, rcon, gameType, gameMode,
                                    map, port, image,
                                    needsLogin, loginReg, loginUser, loginPass,
                                    customEnabled, customConfig]() {
        if (needsLogin && !loginUser.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                m_installStatus->setText("🔒 Connexion au registry privé…");
            }, Qt::QueuedConnection);
            docker->loginRegistry(loginReg, loginUser, loginPass);
        }

        QStringList args = {
            "run", "-d",
            "--name", name,
            "-p", QString("%1:27015/udp").arg(port),
            "-p", QString("%1:27015/tcp").arg(port),
            "-e", "CS2_SERVERNAME=" + name,
            "-e", "CS2_GAMETYPE=" + QString::number(gameType),
            "-e", "CS2_GAMEMODE=" + QString::number(gameMode),
            "-e", "CS2_STARTMAP=" + map,
            "-e", "CS2_RCON_PORT=27015",
            "-e", "CS2_RCONPW=" + rcon,
            "-v", name + "_data:/home/steam/cs2-dedicated"
        };
        if (!gslt.isEmpty()) args << "-e" << "CS2_GSLT=" + gslt;
        args << image;

        docker->runDetached(args);

        for (int i = 0; i < 60; ++i) {
            QThread::sleep(2);
            if (docker->containerExists(name)) break;
        }

        if (customEnabled && !customConfig.trimmed().isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                m_installStatus->setText("⚙ Application de la configuration personnalisée…");
            }, Qt::QueuedConnection);
            QThread::sleep(5);
            const QString tmpPath = QDir::tempPath() + "/gsm_cs2_" + name + ".tmp";
            {
                QFile f(tmpPath);
                if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                    f.write(customConfig.toUtf8());
            }
            docker->runRaw({"cp", tmpPath,
                            name + ":/home/steam/cs2-dedicated/game/csgo/cfg/server.cfg"});
            QFile::remove(tmpPath);
            docker->restartContainer(name);
            QThread::sleep(3);
        }

        QMetaObject::invokeMethod(this, [this]() {
            m_installStatus->setText("");
            m_installBtn->setEnabled(true);
            checkStatus();
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void CS2Page::onUninstall() {
    if (m_currentInstanceIdx < 0 || m_currentInstanceIdx >= m_instances.size()) return;
    const QString id   = m_instances[m_currentInstanceIdx].id;
    const QString name = m_instances[m_currentInstanceIdx].containerName;

    if (m_dashboards.contains(id))
        m_dashboards[id]->stopPolling();

    DockerManager *docker = m_docker;
    auto *thread = QThread::create([docker, name]() {
        docker->removeContainer(name, true);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, this, [this, id]() {
        if (m_dashboards.contains(id)) {
            m_stack->removeWidget(m_dashboards[id]);
            m_dashboards[id]->deleteLater();
            m_dashboards.remove(id);
        }
        m_stack->setCurrentIndex(0);
    });
}
