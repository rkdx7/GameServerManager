#include "PluginManagerWidget.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QRegularExpression>
#include <QScrollArea>

namespace {

const char *BTN_PRIMARY = R"(
    QPushButton {
        background: #6366f1; color: white; border: none;
        border-radius: 8px; padding: 7px 16px;
        font-size: 12px; font-weight: 600;
    }
    QPushButton:hover { background: #4f46e5; }
    QPushButton:disabled { background: #94a3b8; }
)";

const char *BTN_DANGER = R"(
    QPushButton {
        background: transparent; color: #ef4444;
        border: 1.5px solid #ef4444; border-radius: 8px;
        padding: 7px 16px; font-size: 12px; font-weight: 600;
    }
    QPushButton:hover { background: #fef2f2; }
    QPushButton:disabled { color: #94a3b8; border-color: #94a3b8; }
)";

const char *BTN_GHOST = R"(
    QPushButton {
        background: #f1f5f9; color: #475569; border: none;
        border-radius: 8px; padding: 7px 14px;
        font-size: 12px; font-weight: 600;
    }
    QPushButton:hover { background: #e2e8f0; }
)";

const char *EDITOR_STYLE = R"(
    QPlainTextEdit {
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 12px;
        background: #1e293b;
        color: #e2e8f0;
        border: none;
        border-radius: 8px;
        padding: 12px;
    }
)";

const char *LIST_STYLE = R"(
    QListWidget {
        background: #f8fafc;
        border: 1.5px solid #e2e8f0;
        border-radius: 8px;
        outline: none;
    }
    QListWidget::item {
        padding: 8px 12px;
        color: #334155;
        font-size: 12px;
        border-radius: 6px;
        margin: 2px 4px;
    }
    QListWidget::item:selected {
        background: #ede9fe;
        color: #4f46e5;
    }
    QListWidget::item:hover:!selected {
        background: #f1f5f9;
    }
)";

const char *COMBO_STYLE = R"(
    QComboBox {
        border: 1.5px solid #e2e8f0;
        border-radius: 8px;
        padding: 6px 10px;
        font-size: 12px;
        background: #f8fafc;
        color: #1e293b;
    }
    QComboBox:focus { border-color: #6366f1; }
    QComboBox::drop-down { border: none; }
    QComboBox QAbstractItemView {
        background: #ffffff;
        color: #1e293b;
        selection-background-color: #6366f1;
        selection-color: #ffffff;
        border: 1px solid #e2e8f0;
        border-radius: 4px;
        outline: none;
    }
)";

} // namespace

PluginManagerWidget::PluginManagerWidget(DockerManager *docker,
                                          const QString  &containerName,
                                          QWidget        *parent)
    : QWidget(parent), m_docker(docker), m_containerName(containerName)
{
    // ── Toolbar ─────────────────────────────────────────────────────────
    auto *toolBar = new QHBoxLayout;
    toolBar->setContentsMargins(0, 0, 0, 0);
    toolBar->setSpacing(8);

    auto *titleLbl = new QLabel("🔌  Plugins Minecraft", this);
    titleLbl->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b;");

    auto *refreshBtn = new QPushButton("⟳  Actualiser", this);
    refreshBtn->setStyleSheet(BTN_GHOST);
    refreshBtn->setCursor(Qt::PointingHandCursor);

    auto *uploadBtn = new QPushButton("⬆  Installer un plugin", this);
    uploadBtn->setStyleSheet(BTN_PRIMARY);
    uploadBtn->setCursor(Qt::PointingHandCursor);

    toolBar->addWidget(titleLbl);
    toolBar->addStretch();
    toolBar->addWidget(refreshBtn);
    toolBar->addWidget(uploadBtn);

    connect(refreshBtn, &QPushButton::clicked, this, &PluginManagerWidget::refresh);
    connect(uploadBtn,  &QPushButton::clicked, this, &PluginManagerWidget::onUpload);

    // ── Left panel: plugin list ──────────────────────────────────────────
    auto *leftPanel = new QWidget(this);
    auto *leftLay   = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(8);

    auto *listTitle = new QLabel("Plugins installés", leftPanel);
    listTitle->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b;");

    m_jarList = new QListWidget(leftPanel);
    m_jarList->setStyleSheet(LIST_STYLE);
    m_jarList->setCursor(Qt::PointingHandCursor);

    m_deleteBtn = new QPushButton("🗑  Supprimer", leftPanel);
    m_deleteBtn->setStyleSheet(BTN_DANGER);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setEnabled(false);

    leftLay->addWidget(listTitle);
    leftLay->addWidget(m_jarList, 1);
    leftLay->addWidget(m_deleteBtn);

    connect(m_jarList,  &QListWidget::currentItemChanged,
            this, &PluginManagerWidget::onPluginSelected);
    connect(m_deleteBtn, &QPushButton::clicked,
            this, &PluginManagerWidget::onDeletePlugin);

    // ── Right panel: config editor ───────────────────────────────────────
    m_rightPanel = new QWidget(this);
    auto *rightLay = new QVBoxLayout(m_rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(10);

    m_configTitle = new QLabel("Sélectionnez un plugin pour voir sa configuration.", m_rightPanel);
    m_configTitle->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b;");
    m_configTitle->setWordWrap(true);

    auto *comboRow = new QHBoxLayout;
    comboRow->setSpacing(8);

    m_configCombo = new QComboBox(m_rightPanel);
    m_configCombo->setStyleSheet(COMBO_STYLE);
    m_configCombo->setEnabled(false);
    m_configCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    comboRow->addWidget(new QLabel("Fichier :", m_rightPanel));
    comboRow->addWidget(m_configCombo, 1);

    m_configEditor = new QPlainTextEdit(m_rightPanel);
    m_configEditor->setStyleSheet(EDITOR_STYLE);
    m_configEditor->setReadOnly(true);
    m_configEditor->setPlaceholderText("Aucun fichier de configuration sélectionné.");

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);

    m_saveBtn = new QPushButton("💾  Sauvegarder", m_rightPanel);
    m_saveBtn->setStyleSheet(BTN_PRIMARY);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setEnabled(false);

    m_statusLabel = new QLabel("", m_rightPanel);
    m_statusLabel->setStyleSheet("font-size: 11px; color: #64748b;");

    actionRow->addWidget(m_saveBtn);
    actionRow->addWidget(m_statusLabel, 1);

    auto *noteLabel = new QLabel("⚠ Redémarrez le serveur pour appliquer les changements.", m_rightPanel);
    noteLabel->setStyleSheet("font-size: 11px; color: #f59e0b; font-style: italic;");
    noteLabel->setWordWrap(true);

    rightLay->addWidget(m_configTitle);
    rightLay->addLayout(comboRow);
    rightLay->addWidget(m_configEditor, 1);
    rightLay->addLayout(actionRow);
    rightLay->addWidget(noteLabel);

    connect(m_configCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PluginManagerWidget::onConfigFileSelected);
    connect(m_saveBtn, &QPushButton::clicked,
            this, &PluginManagerWidget::onSaveConfig);

    // ── Splitter ─────────────────────────────────────────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftPanel);
    splitter->addWidget(m_rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setHandleWidth(12);
    splitter->setStyleSheet("QSplitter::handle { background: #e2e8f0; border-radius: 2px; }");

    // ── Root layout ───────────────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);
    root->addLayout(toolBar);
    root->addWidget(splitter, 1);
}

void PluginManagerWidget::refresh() {
    m_jarList->clear();
    m_configCombo->clear();
    m_configCombo->setEnabled(false);
    m_configEditor->clear();
    m_configEditor->setReadOnly(true);
    m_saveBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_configTitle->setText("Chargement…");
    setStatus("");

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;

    auto *thread = QThread::create([this, docker, name]() {
        QStringList jars = docker->listPlugins(name);
        QMetaObject::invokeMethod(this, [this, jars]() {
            if (jars.isEmpty()) {
                m_configTitle->setText(
                    "Aucun plugin trouvé. Assurez-vous que le serveur tourne "
                    "et que le type est PAPER ou SPIGOT.");
            } else {
                m_configTitle->setText("Sélectionnez un plugin pour voir sa configuration.");
                for (const QString &j : jars) {
                    auto *item = new QListWidgetItem("📦  " + j, m_jarList);
                    item->setData(Qt::UserRole, j); // raw jar name
                }
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

// static
QString PluginManagerWidget::guessConfigDir(const QString &jarName) {
    QString base = jarName;
    base.remove(QRegularExpression("\\.jar$", QRegularExpression::CaseInsensitiveOption));
    // Strip typical version suffixes: -2.20.1, -v6.4, -1.0-SNAPSHOT, -1.0-beta-3
    base.remove(QRegularExpression("-v?\\d[\\d._\\-]*$", QRegularExpression::CaseInsensitiveOption));
    return base;
}

void PluginManagerWidget::onPluginSelected(QListWidgetItem *current, QListWidgetItem *) {
    if (!current) {
        m_deleteBtn->setEnabled(false);
        m_configCombo->clear();
        m_configCombo->setEnabled(false);
        m_configEditor->clear();
        m_configEditor->setReadOnly(true);
        m_saveBtn->setEnabled(false);
        return;
    }

    m_deleteBtn->setEnabled(true);
    m_configCombo->blockSignals(true);
    m_configCombo->clear();
    m_configCombo->setEnabled(false);
    m_configCombo->blockSignals(false);
    m_configEditor->clear();
    m_configEditor->setReadOnly(true);
    m_saveBtn->setEnabled(false);

    QString jarName = current->data(Qt::UserRole).toString();
    QString guessed = guessConfigDir(jarName);

    m_configTitle->setText(QString("Configuration de : %1").arg(jarName));
    setStatus("Recherche des fichiers de configuration…");

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;

    auto *thread = QThread::create([this, docker, name, guessed]() {
        // Get all plugin directories and find the best match
        QStringList dirs = docker->listPluginDirs(name);
        QString matched;
        for (const QString &d : dirs) {
            if (d.compare(guessed, Qt::CaseInsensitive) == 0) { matched = d; break; }
        }
        // Fallback: partial match
        if (matched.isEmpty()) {
            for (const QString &d : dirs) {
                if (d.startsWith(guessed, Qt::CaseInsensitive) ||
                    guessed.startsWith(d, Qt::CaseInsensitive)) {
                    matched = d; break;
                }
            }
        }

        QStringList configs;
        if (!matched.isEmpty())
            configs = docker->listPluginConfigs(name, matched);

        QMetaObject::invokeMethod(this, [this, matched, configs]() {
            setStatus("");
            if (matched.isEmpty()) {
                m_configTitle->setText(
                    m_configTitle->text() +
                    "\n(Démarrez le serveur une fois pour générer les fichiers de configuration.)");
                return;
            }
            populateConfigs(matched);
            if (!configs.isEmpty()) {
                m_configCombo->blockSignals(true);
                for (const QString &f : configs)
                    m_configCombo->addItem(f, "/data/plugins/" + matched + "/" + f);
                m_configCombo->setEnabled(true);
                m_configCombo->blockSignals(false);
                // Load the first file
                if (m_configCombo->count() > 0)
                    loadConfigFile(m_configCombo->itemData(0).toString());
            } else {
                m_configTitle->setText(
                    m_configTitle->text() +
                    "\n(Aucun fichier de configuration trouvé dans /" + matched + "/)");
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void PluginManagerWidget::populateConfigs(const QString &pluginDir) {
    // Title already set by caller — just update it to include the dir name
    m_configTitle->setText(QString("📁  /data/plugins/%1/").arg(pluginDir));
}

void PluginManagerWidget::onConfigFileSelected(int index) {
    if (index < 0 || !m_configCombo->isEnabled()) return;
    QString path = m_configCombo->itemData(index).toString();
    if (!path.isEmpty())
        loadConfigFile(path);
}

void PluginManagerWidget::loadConfigFile(const QString &fullPath) {
    m_currentConfigPath = fullPath;
    m_configEditor->setReadOnly(true);
    m_saveBtn->setEnabled(false);
    setStatus("Chargement…");

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;

    auto *thread = QThread::create([this, docker, name, fullPath]() {
        QString content = docker->readPluginConfig(name, fullPath);
        QMetaObject::invokeMethod(this, [this, content]() {
            m_configEditor->setPlainText(content);
            m_configEditor->setReadOnly(false);
            m_saveBtn->setEnabled(true);
            setStatus("✓  Fichier chargé.");
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void PluginManagerWidget::onUpload() {
    QString path = QFileDialog::getOpenFileName(
        this, "Sélectionner un plugin (.jar)", QString(),
        "Plugins Minecraft (*.jar)");
    if (path.isEmpty()) return;

    setStatus("⏳  Installation en cours…");
    m_deleteBtn->setEnabled(false);

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;

    auto *thread = QThread::create([this, docker, name, path]() {
        bool ok = docker->installPlugin(name, path);
        QMetaObject::invokeMethod(this, [this, ok]() {
            if (ok)
                setStatus("✓  Plugin installé. Redémarrez le serveur pour l'activer.");
            else
                setStatus("✗  Échec de l'installation.", true);
            refresh();
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void PluginManagerWidget::onDeletePlugin() {
    auto *item = m_jarList->currentItem();
    if (!item) return;

    QString jarName = item->data(Qt::UserRole).toString();
    auto reply = QMessageBox::question(
        this, "Supprimer le plugin",
        QString("Supprimer %1 ?").arg(jarName),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (reply != QMessageBox::Yes) return;

    setStatus("⏳  Suppression…");
    DockerManager *docker = m_docker;
    QString        name   = m_containerName;

    auto *thread = QThread::create([this, docker, name, jarName]() {
        docker->deletePlugin(name, jarName);
        QMetaObject::invokeMethod(this, [this]() {
            setStatus("✓  Plugin supprimé.");
            refresh();
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void PluginManagerWidget::onSaveConfig() {
    if (m_currentConfigPath.isEmpty()) return;

    QString content = m_configEditor->toPlainText();
    setStatus("⏳  Sauvegarde…");
    m_saveBtn->setEnabled(false);

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;
    QString        path   = m_currentConfigPath;

    auto *thread = QThread::create([this, docker, name, path, content]() {
        docker->writePluginConfig(name, path, content);
        QMetaObject::invokeMethod(this, [this]() {
            m_saveBtn->setEnabled(true);
            setStatus("✓  Configuration sauvegardée. Redémarrez le serveur pour appliquer.");
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void PluginManagerWidget::setStatus(const QString &msg, bool error) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(
        error ? "font-size: 11px; color: #ef4444;"
              : "font-size: 11px; color: #64748b;");
}
