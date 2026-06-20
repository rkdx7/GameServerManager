#include "ConfigEditorWidget.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QThread>

namespace {

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
    QScrollBar:vertical, QScrollBar:horizontal {
        background: #0f172a;
        width: 8px; height: 8px;
        border-radius: 4px;
    }
    QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
        background: #334155;
        border-radius: 4px;
    }
)";

const char *BTN_PRIMARY = R"(
    QPushButton {
        background: #6366f1; color: white; border: none;
        border-radius: 8px; padding: 7px 16px;
        font-size: 12px; font-weight: 600;
    }
    QPushButton:hover { background: #4f46e5; }
    QPushButton:disabled { background: #94a3b8; }
)";

const char *BTN_GHOST = R"(
    QPushButton {
        background: #f1f5f9; color: #475569; border: none;
        border-radius: 8px; padding: 7px 14px;
        font-size: 12px; font-weight: 600;
    }
    QPushButton:hover { background: #e2e8f0; }
    QPushButton:disabled { color: #94a3b8; }
)";

} // namespace

ConfigEditorWidget::ConfigEditorWidget(DockerManager *docker,
                                       const QString &containerName,
                                       const QString &filePath,
                                       QWidget       *parent)
    : QWidget(parent)
    , m_docker(docker)
    , m_container(containerName)
    , m_filePath(filePath)
{
    auto *titleLbl = new QLabel(QString("📄  %1").arg(m_filePath), this);
    titleLbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #1e293b; background: transparent;");
    titleLbl->setWordWrap(true);

    auto *subLbl = new QLabel(
        "Modifiez la configuration du serveur puis sauvegardez. "
        "Le serveur doit être démarré pour lire/écrire ce fichier.", this);
    subLbl->setStyleSheet("font-size: 11px; color: #94a3b8; background: transparent;");
    subLbl->setWordWrap(true);

    m_editor = new QPlainTextEdit(this);
    m_editor->setStyleSheet(EDITOR_STYLE);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setReadOnly(true);
    m_editor->setPlaceholderText("Chargement…");

    m_reloadBtn = new QPushButton("⟳  Recharger", this);
    m_reloadBtn->setStyleSheet(BTN_GHOST);
    m_reloadBtn->setCursor(Qt::PointingHandCursor);

    m_saveBtn = new QPushButton("💾  Sauvegarder", this);
    m_saveBtn->setStyleSheet(BTN_PRIMARY);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setEnabled(false);

    m_saveRestartBtn = new QPushButton("💾  Sauvegarder & redémarrer", this);
    m_saveRestartBtn->setStyleSheet(BTN_PRIMARY);
    m_saveRestartBtn->setCursor(Qt::PointingHandCursor);
    m_saveRestartBtn->setEnabled(false);

    m_status = new QLabel("", this);
    m_status->setStyleSheet("font-size: 11px; color: #64748b; background: transparent;");

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);
    actionRow->addWidget(m_reloadBtn);
    actionRow->addWidget(m_saveBtn);
    actionRow->addWidget(m_saveRestartBtn);
    actionRow->addWidget(m_status, 1);

    auto *noteLbl = new QLabel(
        "⚠ Un redémarrage du serveur est nécessaire pour appliquer les changements.", this);
    noteLbl->setStyleSheet("font-size: 11px; color: #f59e0b; font-style: italic; background: transparent;");
    noteLbl->setWordWrap(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);
    root->addWidget(titleLbl);
    root->addWidget(subLbl);
    root->addWidget(m_editor, 1);
    root->addLayout(actionRow);
    root->addWidget(noteLbl);

    connect(m_reloadBtn,      &QPushButton::clicked, this, &ConfigEditorWidget::load);
    connect(m_saveBtn,        &QPushButton::clicked, this, &ConfigEditorWidget::save);
    connect(m_saveRestartBtn, &QPushButton::clicked, this, &ConfigEditorWidget::saveAndRestart);
}

void ConfigEditorWidget::ensureLoaded() {
    if (m_loadedOnce) return;
    load();
}

void ConfigEditorWidget::load() {
    if (m_busy) return;
    m_busy = true;
    m_loadedOnce = true;

    m_editor->setReadOnly(true);
    m_saveBtn->setEnabled(false);
    m_saveRestartBtn->setEnabled(false);
    m_reloadBtn->setEnabled(false);
    setStatus("Chargement…");

    DockerManager *docker = m_docker;
    QString        name   = m_container;
    QString        path   = m_filePath;

    auto *thread = QThread::create([this, docker, name, path]() {
        const bool running = docker->isContainerRunning(name);
        const QString content = running ? docker->readFile(name, path) : QString();
        QMetaObject::invokeMethod(this, [this, running, content]() {
            if (!running) {
                m_editor->setPlainText("");
                m_editor->setPlaceholderText(
                    "Le serveur est arrêté. Démarrez-le pour éditer la configuration.");
                setStatus("✗  Serveur arrêté.", true);
            } else {
                m_editor->setPlainText(content);
                m_editor->setReadOnly(false);
                m_saveBtn->setEnabled(true);
                m_saveRestartBtn->setEnabled(true);
                setStatus("✓  Fichier chargé.");
            }
            m_reloadBtn->setEnabled(true);
            m_busy = false;
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ConfigEditorWidget::save()          { doSave(false); }
void ConfigEditorWidget::saveAndRestart(){ doSave(true);  }

void ConfigEditorWidget::doSave(bool restart) {
    if (m_busy) return;
    m_busy = true;

    const QString content = m_editor->toPlainText();
    m_saveBtn->setEnabled(false);
    m_saveRestartBtn->setEnabled(false);
    m_reloadBtn->setEnabled(false);
    setStatus(restart ? "⏳  Sauvegarde et redémarrage…" : "⏳  Sauvegarde…");

    DockerManager *docker = m_docker;
    QString        name   = m_container;
    QString        path   = m_filePath;

    auto *thread = QThread::create([this, docker, name, path, content, restart]() {
        const bool ok = docker->writeFile(name, path, content);
        if (ok && restart) docker->restartContainer(name);
        QMetaObject::invokeMethod(this, [this, ok, restart]() {
            m_saveBtn->setEnabled(true);
            m_saveRestartBtn->setEnabled(true);
            m_reloadBtn->setEnabled(true);
            if (!ok)
                setStatus("✗  Échec de la sauvegarde.", true);
            else if (restart)
                setStatus("✓  Configuration sauvegardée. Serveur redémarré.");
            else
                setStatus("✓  Configuration sauvegardée. Redémarrez pour appliquer.");
            m_busy = false;
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ConfigEditorWidget::setStatus(const QString &msg, bool error) {
    m_status->setText(msg);
    m_status->setStyleSheet(
        error ? "font-size: 11px; color: #ef4444; background: transparent;"
              : "font-size: 11px; color: #64748b; background: transparent;");
}
