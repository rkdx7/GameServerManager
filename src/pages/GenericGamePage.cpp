#include "GenericGamePage.h"
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
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QThread>
#include <QListWidget>
#include <QSettings>
#include <QUuid>
#include <QMessageBox>
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
} // namespace

// ── Persistence ───────────────────────────────────────────────────────────────

void GenericGamePage::loadInstances()
{
    QSettings s("GameServerManager", "App");
    const QString base = "instances/" + m_config.defaultContainerName;
    int count = s.value(base + "/count", 0).toInt();

    m_instances.clear();
    for (int i = 0; i < count; ++i) {
        const QString pfx = base + "/" + QString::number(i);
        ServerInstanceConfig inst;
        inst.id            = s.value(pfx + "/id").toString();
        inst.displayName   = s.value(pfx + "/displayName").toString();
        inst.containerName = s.value(pfx + "/containerName").toString();
        inst.port          = s.value(pfx + "/port", m_config.defaultPort).toInt();
        inst.vmId          = s.value(pfx + "/vmId", "local").toString();
        inst.imageOverride        = s.value(pfx + "/imageOverride").toString();
        inst.customConfigEnabled  = s.value(pfx + "/customConfigEnabled", false).toBool();
        inst.customConfig         = s.value(pfx + "/customConfig").toString();
        inst.customConfigAtInstall = s.value(pfx + "/customConfigAtInstall", true).toBool();

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

void GenericGamePage::saveInstances()
{
    QSettings s("GameServerManager", "App");
    const QString base = "instances/" + m_config.defaultContainerName;
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
        s.setValue(pfx + "/customConfigAtInstall", inst.customConfigAtInstall);

        QStringList keys = inst.fieldValues.keys();
        s.setValue(pfx + "/fields/count", keys.size());
        for (int j = 0; j < keys.size(); ++j) {
            s.setValue(pfx + "/fields/" + QString::number(j) + "/key",   keys[j]);
            s.setValue(pfx + "/fields/" + QString::number(j) + "/value", inst.fieldValues[keys[j]]);
        }
    }
}

// ── Constructor ───────────────────────────────────────────────────────────────

GenericGamePage::GenericGamePage(DockerManager *docker,
                                  const GamePageConfig &config,
                                  const QVector<VMInstance> *vmList,
                                  QWidget *parent)
    : QWidget(parent), m_docker(docker), m_config(config), m_vmList(vmList)
{
    loadInstances();
    if (m_instances.isEmpty()) {
        ServerInstanceConfig def;
        def.id            = QUuid::createUuid().toString(QUuid::WithoutBraces);
        def.displayName   = m_config.defaultContainerName;
        def.containerName = m_config.defaultContainerName;
        def.port          = m_config.defaultPort;
        def.vmId          = "local";
        for (const auto &f : m_config.fields)
            def.fieldValues[f.key] = f.defaultValue;
        m_instances.append(def);
        saveInstances();
    }

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    if (m_vmList) {
        m_targetSelector = new DeploymentTargetSelector(
            m_config.defaultContainerName, m_docker, m_vmList, this);
        root->addWidget(m_targetSelector);
        connect(m_targetSelector, &DeploymentTargetSelector::targetChanged,
                this, [this](const VMInstance *vm) {
            if (m_currentInstanceIdx >= 0 && m_currentInstanceIdx < m_instances.size()) {
                m_instances[m_currentInstanceIdx].vmId = vm ? vm->id : QString("local");
                saveInstances();
            }
        });
    }

    // Horizontal body: instance panel left + stack right
    auto *body       = new QWidget;
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    bodyLayout->addWidget(buildInstancePanel());

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildInstallForm());
    bodyLayout->addWidget(m_stack, 1);

    root->addWidget(body, 1);

    // Select first instance (form widgets already built at this point)
    m_instanceList->setCurrentRow(0);
}

// ── Instance panel ────────────────────────────────────────────────────────────

QWidget *GenericGamePage::buildInstancePanel()
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
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
            font-size: 12px;
        }
        QListWidget::item {
            padding: 7px 10px;
            border-radius: 6px;
            color: #374151;
        }
        QListWidget::item:selected {
            background: #eef2ff;
            color: #4f46e5;
            font-weight: 600;
        }
        QListWidget::item:hover:!selected {
            background: #f1f5f9;
        }
    )");
    for (const auto &inst : m_instances)
        m_instanceList->addItem(inst.displayName);

    layout->addWidget(m_instanceList, 1);
    layout->addSpacing(6);

    auto *addBtn = new QPushButton("+ Nouveau");
    addBtn->setFixedHeight(32);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(R"(
        QPushButton {
            background: #6366f1; color: white;
            border: none; border-radius: 7px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #4f46e5; }
    )");
    connect(addBtn, &QPushButton::clicked, this, &GenericGamePage::addInstance);
    layout->addWidget(addBtn);

    m_deleteInstanceBtn = new QPushButton("Supprimer");
    m_deleteInstanceBtn->setFixedHeight(28);
    m_deleteInstanceBtn->setCursor(Qt::PointingHandCursor);
    m_deleteInstanceBtn->setStyleSheet(R"(
        QPushButton {
            background: #fee2e2; color: #dc2626;
            border: none; border-radius: 7px;
            font-size: 11px; font-weight: 600;
        }
        QPushButton:hover { background: #fecaca; }
        QPushButton:disabled { background: #f1f5f9; color: #cbd5e1; }
    )");
    connect(m_deleteInstanceBtn, &QPushButton::clicked, this, [this]() {
        int idx = m_instanceList->currentRow();
        if (idx >= 0) deleteInstance(idx);
    });
    layout->addWidget(m_deleteInstanceBtn);

    connect(m_instanceList, &QListWidget::currentRowChanged,
            this, &GenericGamePage::switchToInstance);

    updateInstanceButtons();
    return panel;
}

void GenericGamePage::updateInstanceButtons()
{
    if (m_deleteInstanceBtn)
        m_deleteInstanceBtn->setEnabled(m_instances.size() > 1);
}

// ── Instance switching ────────────────────────────────────────────────────────

void GenericGamePage::saveCurrentFormState()
{
    if (m_currentInstanceIdx < 0 || m_currentInstanceIdx >= m_instances.size()) return;
    auto &inst = m_instances[m_currentInstanceIdx];
    if (m_serverName) inst.containerName = m_serverName->text();
    if (m_port)       inst.port          = m_port->value();
    inst.imageOverride       = m_imageOverride;
    inst.customConfigEnabled = m_advancedToggleBtn && m_advancedToggleBtn->isChecked();
    inst.customConfig        = m_customConfigEdit  ? m_customConfigEdit->toPlainText() : QString();
    inst.customConfigAtInstall = !m_customTimingCombo || m_customTimingCombo->currentIndex() == 0;
    for (const auto &f : m_config.fields)
        inst.fieldValues[f.key] = fieldValue(f.key);
    saveInstances();
}

void GenericGamePage::loadFormFromInstance(const ServerInstanceConfig &inst)
{
    if (m_serverName) m_serverName->setText(inst.containerName);
    if (m_port)       m_port->setValue(inst.port > 0 ? inst.port : m_config.defaultPort);

    for (const auto &f : m_config.fields) {
        auto it = m_fieldWidgets.find(f.key);
        if (it == m_fieldWidgets.end()) continue;
        const QString val = inst.fieldValues.value(f.key, f.defaultValue);
        QWidget *w = it.value();
        if (auto *le  = qobject_cast<QLineEdit *>(w))  le->setText(val);
        else if (auto *cb = qobject_cast<QComboBox *>(w)) {
            int i = cb->findText(val);
            if (i >= 0) cb->setCurrentIndex(i);
        }
        else if (auto *sb = qobject_cast<QSpinBox  *>(w)) sb->setValue(val.toInt());
        else if (auto *chk = qobject_cast<QCheckBox *>(w)) chk->setChecked(val == "1");
    }

    m_imageOverride = inst.imageOverride;
    if (m_imageBadge) {
        const QString label    = m_imageOverride.isEmpty() ? m_config.dockerImage : m_imageOverride;
        const bool    overridden = !m_imageOverride.isEmpty() && m_imageOverride != m_config.dockerImage;
        m_imageBadge->setText((overridden ? "🔄 " : "🐳 ") + label);
        m_imageBadge->setStyleSheet(overridden
            ? "font-size: 10px; color: #4f46e5; font-weight: 600; background: transparent;"
            : "font-size: 10px; color: #94a3b8; background: transparent;");
    }

    if (m_advancedToggleBtn) m_advancedToggleBtn->setChecked(inst.customConfigEnabled);
    if (m_advancedContent)   m_advancedContent->setVisible(inst.customConfigEnabled);
    if (m_customConfigEdit)  m_customConfigEdit->setPlainText(inst.customConfig);
    if (m_customTimingCombo) m_customTimingCombo->setCurrentIndex(inst.customConfigAtInstall ? 0 : 1);
}

void GenericGamePage::switchToInstance(int idx)
{
    if (idx < 0 || idx >= m_instances.size()) return;

    // Stop current dashboard polling
    if (m_currentInstanceIdx >= 0 && m_currentInstanceIdx < m_instances.size()) {
        const QString &prevId = m_instances[m_currentInstanceIdx].id;
        if (m_dashboards.contains(prevId))
            m_dashboards[prevId]->stopPolling();
    }

    saveCurrentFormState();
    m_currentInstanceIdx = idx;

    const ServerInstanceConfig &inst = m_instances[idx];

    // Update VM target selector
    if (m_targetSelector)
        m_targetSelector->setTargetById(inst.vmId.isEmpty() ? "local" : inst.vmId);

    loadFormFromInstance(inst);
    checkStatus();
}

void GenericGamePage::addInstance()
{
    saveCurrentFormState();

    ServerInstanceConfig inst;
    inst.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const int n = m_instances.size() + 1;
    inst.containerName = m_config.defaultContainerName + "-" + QString::number(n);
    inst.displayName   = inst.containerName;
    inst.port          = m_config.defaultPort + n - 1;
    inst.vmId          = (m_targetSelector && m_targetSelector->selectedVM())
                         ? m_targetSelector->selectedVM()->id : QString("local");
    for (const auto &f : m_config.fields)
        inst.fieldValues[f.key] = f.defaultValue;

    m_instances.append(inst);
    m_instanceList->addItem(inst.displayName);
    saveInstances();
    updateInstanceButtons();

    m_instanceList->setCurrentRow(m_instances.size() - 1);
}

void GenericGamePage::deleteInstance(int idx)
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
        int newIdx = qMin(idx, m_instances.size() - 1);
        m_instanceList->setCurrentRow(newIdx);
    } else {
        m_instanceList->blockSignals(true);
        m_instanceList->setCurrentRow(m_currentInstanceIdx);
        m_instanceList->blockSignals(false);
    }
}

// ── Install form ──────────────────────────────────────────────────────────────

QWidget *GenericGamePage::buildInstallForm()
{
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");

    auto *container = new QWidget;
    container->setStyleSheet("background: transparent;");

    auto *outer = new QVBoxLayout(container);
    outer->setContentsMargins(40, 32, 40, 32);
    outer->setSpacing(0);

    // ── Gear button (top-right) ─────────────────────────────────────────
    auto *gearBtn = new QPushButton("⚙  Image Docker", container);
    gearBtn->setFixedHeight(34);
    gearBtn->setCursor(Qt::PointingHandCursor);
    gearBtn->setStyleSheet(R"(
        QPushButton {
            background: #eef2ff;
            color: #4f46e5;
            border: none;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 600;
            padding: 0 14px;
        }
        QPushButton:hover { background: #e0e7ff; }
    )");
    connect(gearBtn, &QPushButton::clicked, this, &GenericGamePage::openImagePicker);

    m_imageBadge = new QLabel("🐳 " + m_config.dockerImage, container);
    m_imageBadge->setStyleSheet("font-size: 10px; color: #94a3b8; background: transparent;");
    m_imageBadge->setAlignment(Qt::AlignRight);

    auto *gearCol = new QVBoxLayout;
    gearCol->setSpacing(4);
    gearCol->addWidget(gearBtn, 0, Qt::AlignRight);
    gearCol->addWidget(m_imageBadge);

    auto *topRow = new QHBoxLayout;
    topRow->setSpacing(0);
    topRow->addStretch();
    topRow->addLayout(gearCol);
    outer->addLayout(topRow);
    outer->addSpacing(8);

    outer->addWidget(buildGameBanner(
        m_config.icon, m_config.title, m_config.description,
        m_config.btnColorStart, m_config.btnColorEnd, container));

    if (!m_config.note.isEmpty()) {
        outer->addSpacing(12);
        auto *noteLbl = new QLabel(m_config.note, container);
        noteLbl->setStyleSheet("font-size: 12px; color: #94a3b8; background: transparent;");
        noteLbl->setWordWrap(true);
        outer->addWidget(noteLbl);
    }
    outer->addSpacing(24);

    // ── Form card ─────────────────────────────────────────────────────────
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
    m_serverName->setText(m_config.defaultContainerName);
    m_serverName->setStyleSheet(INPUT_STYLE);
    form->addRow(mkLabel("Nom du conteneur"), m_serverName);

    for (const auto &f : m_config.fields) {
        QWidget *w = nullptr;
        switch (f.type) {
        case GameFieldConfig::Text: {
            auto *le = new QLineEdit(card);
            le->setText(f.defaultValue);
            if (!f.placeholder.isEmpty()) le->setPlaceholderText(f.placeholder);
            le->setStyleSheet(INPUT_STYLE);
            w = le;
            break;
        }
        case GameFieldConfig::Password: {
            auto *le = new QLineEdit(card);
            le->setText(f.defaultValue);
            if (!f.placeholder.isEmpty()) le->setPlaceholderText(f.placeholder);
            le->setEchoMode(QLineEdit::Password);
            le->setStyleSheet(INPUT_STYLE);
            w = le;
            break;
        }
        case GameFieldConfig::Combo: {
            auto *cb = new QComboBox(card);
            cb->addItems(f.comboOptions);
            if (!f.defaultValue.isEmpty()) {
                int idx = f.comboOptions.indexOf(f.defaultValue);
                if (idx >= 0) cb->setCurrentIndex(idx);
            }
            cb->setStyleSheet(INPUT_STYLE);
            w = cb;
            break;
        }
        case GameFieldConfig::Spin: {
            auto *sb = new QSpinBox(card);
            sb->setRange(f.spinMin, f.spinMax);
            sb->setValue(f.defaultValue.toInt());
            sb->setStyleSheet(INPUT_STYLE);
            w = sb;
            break;
        }
        case GameFieldConfig::Check: {
            auto *chk = new QCheckBox(f.label, card);
            chk->setChecked(f.defaultValue == "1" || f.defaultValue.toLower() == "true");
            chk->setStyleSheet("font-size: 13px; color: #374151;");
            m_fieldWidgets[f.key] = chk;
            form->addRow("", chk);
            continue;
        }
        }
        if (w) {
            form->addRow(mkLabel(f.label), w);
            m_fieldWidgets[f.key] = w;
        }
    }

    m_port = new QSpinBox(card);
    m_port->setRange(1024, 65535);
    m_port->setValue(m_config.defaultPort);
    m_port->setStyleSheet(INPUT_STYLE);
    form->addRow(mkLabel("Port principal"), m_port);

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

    if (!m_config.configDocUrl.isEmpty()) {
        auto *docLink = new QLabel(
            "<a href='" + m_config.configDocUrl + "'"
            " style='color: " + m_config.btnColorStart + "; font-weight: 600; text-decoration: none;'>"
            "📖 Documentation de configuration →</a>",
            m_advancedContent);
        docLink->setTextFormat(Qt::RichText);
        docLink->setOpenExternalLinks(true);
        docLink->setStyleSheet("font-size: 13px; background: transparent;");
        advContentLayout->addWidget(docLink);
    }

    // When to apply the custom config: during install, or later (the config file
    // stays editable after installation via the dashboard "Configuration" tab).
    // Only relevant for games that expose a known config file path.
    if (!m_config.configFilePath.isEmpty()) {
        auto *timingRow = new QHBoxLayout;
        timingRow->setSpacing(8);
        auto *timingLbl = new QLabel("Quand personnaliser :", m_advancedContent);
        timingLbl->setStyleSheet("font-size: 12px; color: #374151; background: transparent;");
        m_customTimingCombo = new QComboBox(m_advancedContent);
        m_customTimingCombo->addItem("À l'installation");
        m_customTimingCombo->addItem("Plus tard (onglet Configuration)");
        m_customTimingCombo->setStyleSheet(QString(R"(
            QComboBox {
                border: 1.5px solid #e2e8f0; border-radius: 8px;
                padding: 6px 10px; font-size: 12px;
                background: #f8fafc; color: #1e293b;
            }
            QComboBox:focus { border-color: %1; }
        )").arg(m_config.btnColorStart));
        timingRow->addWidget(timingLbl);
        timingRow->addWidget(m_customTimingCombo, 1);
        advContentLayout->addLayout(timingRow);
    }

    m_customConfigEdit = new QTextEdit(m_advancedContent);
    m_customConfigEdit->setPlaceholderText("Collez votre configuration serveur ici...");
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
        QTextEdit:focus { border-color: #6366f1; background: #ffffff; }
    )");
    advContentLayout->addWidget(m_customConfigEdit);

    if (m_customTimingCombo) {
        auto *timingHint = new QLabel(
            "La configuration restera modifiable après l'installation depuis "
            "l'onglet « Configuration » du serveur.", m_advancedContent);
        timingHint->setStyleSheet("font-size: 11px; color: #94a3b8; background: transparent;");
        timingHint->setWordWrap(true);
        advContentLayout->addWidget(timingHint);

        connect(m_customTimingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            if (m_customConfigEdit) m_customConfigEdit->setEnabled(idx == 0);
        });
    }

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

    m_installBtn = new QPushButton("🚀  Installer le serveur", container);
    m_installBtn->setFixedHeight(48);
    m_installBtn->setCursor(Qt::PointingHandCursor);
    m_installBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 %1, stop:1 %2);
            color: white; border: none; border-radius: 10px;
            font-size: 15px; font-weight: 700;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 %1, stop:1 %2);
        }
        QPushButton:disabled { background: #94a3b8; }
    )").arg(m_config.btnColorStart, m_config.btnColorEnd));
    connect(m_installBtn, &QPushButton::clicked, this, &GenericGamePage::onInstall);
    outer->addWidget(m_installBtn);
    outer->addStretch();

    scroll->setWidget(container);
    return scroll;
}

QString GenericGamePage::fieldValue(const QString &key) const
{
    auto it = m_fieldWidgets.find(key);
    if (it == m_fieldWidgets.end()) return {};
    QWidget *w = it.value();
    if (auto *le  = qobject_cast<QLineEdit *>(w))  return le->text();
    if (auto *cb  = qobject_cast<QComboBox *>(w))  return cb->currentText();
    if (auto *sb  = qobject_cast<QSpinBox  *>(w))  return QString::number(sb->value());
    if (auto *chk = qobject_cast<QCheckBox *>(w))  return chk->isChecked() ? "1" : "0";
    return {};
}

// ── Status check ──────────────────────────────────────────────────────────────

void GenericGamePage::checkStatus()
{
    if (m_currentInstanceIdx < 0 || m_currentInstanceIdx >= m_instances.size()) return;
    const ServerInstanceConfig &inst = m_instances[m_currentInstanceIdx];

    if (m_docker->containerExists(inst.containerName)) {
        if (!m_dashboards.contains(inst.id)) {
            auto *dash = new ServerDashboard(m_docker, inst.containerName, GameType::Generic,
                                             {}, m_config.configFilePath, this);
            connect(dash, &ServerDashboard::uninstallRequested,
                    this, &GenericGamePage::onUninstall);
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

void GenericGamePage::openImagePicker()
{
    const QString current = m_imageOverride.isEmpty() ? m_config.dockerImage : m_imageOverride;
    ImagePickerDialog dlg(m_docker, current, this);
    if (dlg.exec() != QDialog::Accepted) return;

    m_imageOverride = dlg.selectedImage();
    m_needsLogin    = dlg.requiresLogin();
    m_loginReg      = dlg.loginRegistry();
    m_loginUser     = dlg.loginUsername();
    m_loginPass     = dlg.loginPassword();

    if (m_imageBadge) {
        const QString label    = m_imageOverride.isEmpty() ? m_config.dockerImage : m_imageOverride;
        const bool    overridden = !m_imageOverride.isEmpty() && m_imageOverride != m_config.dockerImage;
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

void GenericGamePage::onInstall()
{
    saveCurrentFormState();

    m_installBtn->setEnabled(false);
    m_installStatus->setStyleSheet("font-size: 13px; color: #6366f1; background: transparent;");
    m_installStatus->setText("⏳ Téléchargement de l'image Docker… (peut prendre quelques minutes)");

    const ServerInstanceConfig inst = m_instances[m_currentInstanceIdx];
    const QString name         = inst.containerName;
    const int     port         = inst.port;
    // Only push the pasted config at install time when the user chose to
    // customize "now"; otherwise it stays editable from the dashboard later.
    const bool    customEnabled = inst.customConfigEnabled && inst.customConfigAtInstall;
    const QString customConfig  = inst.customConfig;

    QMap<QString, QString> values = inst.fieldValues;

    DockerManager  *docker = m_docker;
    GamePageConfig  config = m_config;
    if (!inst.imageOverride.isEmpty())
        config.dockerImage = inst.imageOverride;

    bool    needsLogin = m_needsLogin;
    QString loginReg   = m_loginReg;
    QString loginUser  = m_loginUser;
    QString loginPass  = m_loginPass;

    auto *thread = QThread::create([this, docker, config, name, port, values,
                                    needsLogin, loginReg, loginUser, loginPass,
                                    customEnabled, customConfig]() {
        if (needsLogin && !loginUser.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                m_installStatus->setText("🔒 Connexion au registry privé…");
            }, Qt::QueuedConnection);
            docker->loginRegistry(loginReg, loginUser, loginPass);
        }

        QStringList args = { "run", "-d", "--name", name };

        for (const auto &pm : config.ports) {
            int hPort = (pm.hostPort == 0) ? port : pm.hostPort;
            const QString hStr = QString::number(hPort);
            const QString cStr = QString::number(pm.containerPort);
            if (pm.udp && pm.tcp) {
                args << "-p" << hStr + ":" + cStr + "/udp";
                args << "-p" << hStr + ":" + cStr + "/tcp";
            } else if (pm.udp) {
                args << "-p" << hStr + ":" + cStr + "/udp";
            } else {
                args << "-p" << hStr + ":" + cStr + "/tcp";
            }
        }

        for (const auto &f : config.fields) {
            if (f.envVar.isEmpty()) continue;
            const QString val = values.value(f.key);
            const bool isOptText = (f.type == GameFieldConfig::Text ||
                                    f.type == GameFieldConfig::Password);
            if (isOptText && val.isEmpty()) continue;
            args << "-e" << f.envVar + "=" + val;
        }

        if (!config.dataVolumePath.isEmpty())
            args << "-v" << name + "_data:" + config.dataVolumePath;

        args << config.dockerImage;
        docker->runDetached(args);

        for (int i = 0; i < 30; ++i) {
            QThread::sleep(2);
            if (docker->containerExists(name)) break;
        }

        if (customEnabled && !customConfig.trimmed().isEmpty() && !config.configFilePath.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                m_installStatus->setText("⚙ Application de la configuration personnalisée…");
            }, Qt::QueuedConnection);
            QThread::sleep(5);
            const QString tmpPath = QDir::tempPath() + "/gsm_" + name + ".tmp";
            {
                QFile f(tmpPath);
                if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                    f.write(customConfig.toUtf8());
            }
            docker->runRaw({"cp", tmpPath, name + ":" + config.configFilePath});
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

void GenericGamePage::onUninstall()
{
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
