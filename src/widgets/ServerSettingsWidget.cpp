#include "ServerSettingsWidget.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QThread>

ServerSettingsWidget::ServerSettingsWidget(DockerManager *docker,
                                           const QString &containerName,
                                           const QString &configFilePath,
                                           const QString &configDocUrl,
                                           QWidget       *parent)
    : QWidget(parent)
    , m_docker(docker)
    , m_container(containerName)
    , m_configPath(configFilePath)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(10);

    // ── No editable config file for this server ──────────────────────────────
    if (m_configPath.isEmpty()) {
        auto *note = new QLabel(
            "ℹ Ce serveur n'expose pas de fichier de configuration éditable.\n"
            "Modifiez ses paramètres en réinstallant le serveur, ou via la console.",
            this);
        note->setWordWrap(true);
        note->setAlignment(Qt::AlignCenter);
        note->setStyleSheet("font-size: 13px; color: #64748b; background: transparent;");
        root->addStretch();
        root->addWidget(note);
        root->addStretch();
        return;
    }

    // ── Header ───────────────────────────────────────────────────────────────
    auto *title = new QLabel("Configuration du serveur", this);
    title->setStyleSheet("font-size: 14px; font-weight: 700; color: #1e293b; background: transparent;");
    root->addWidget(title);

    auto *subtitle = new QLabel(
        "Éditez le fichier <code>" + m_configPath + "</code>. "
        "L'enregistrement applique les changements et redémarre le serveur.", this);
    subtitle->setTextFormat(Qt::RichText);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("font-size: 12px; color: #64748b; background: transparent;");
    root->addWidget(subtitle);

    if (!configDocUrl.isEmpty()) {
        auto *docLink = new QLabel(
            "<a href='" + configDocUrl + "'"
            " style='color: #6366f1; font-weight: 600; text-decoration: none;'>"
            "📖 Documentation de configuration →</a>", this);
        docLink->setTextFormat(Qt::RichText);
        docLink->setOpenExternalLinks(true);
        docLink->setStyleSheet("font-size: 12px; background: transparent;");
        root->addWidget(docLink);
    }

    // ── Editor ───────────────────────────────────────────────────────────────
    m_editor = new QTextEdit(this);
    m_editor->setLineWrapMode(QTextEdit::NoWrap);
    m_editor->setPlaceholderText("Chargement de la configuration…");
    m_editor->setStyleSheet(R"(
        QTextEdit {
            background: #0d1117;
            color: #c9d1d9;
            font-family: Consolas, "Courier New", monospace;
            font-size: 12px;
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 8px;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: #161b22; width: 8px; height: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #30363d; border-radius: 4px;
        }
    )");
    root->addWidget(m_editor, 1);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    m_status = new QLabel("", this);
    m_status->setStyleSheet("font-size: 12px; color: #64748b; background: transparent;");

    m_reloadBtn = new QPushButton("↻ Recharger", this);
    m_reloadBtn->setCursor(Qt::PointingHandCursor);
    m_reloadBtn->setStyleSheet(R"(
        QPushButton {
            background: #f1f5f9; color: #475569; border: none;
            border-radius: 6px; padding: 8px 16px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #e2e8f0; }
        QPushButton:disabled { color: #cbd5e1; }
    )");
    connect(m_reloadBtn, &QPushButton::clicked, this, &ServerSettingsWidget::reload);

    m_applyBtn = new QPushButton("💾 Enregistrer & redémarrer", this);
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    m_applyBtn->setStyleSheet(R"(
        QPushButton {
            background: #6366f1; color: white; border: none;
            border-radius: 6px; padding: 8px 16px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #4f46e5; }
        QPushButton:disabled { background: #94a3b8; }
    )");
    connect(m_applyBtn, &QPushButton::clicked, this, &ServerSettingsWidget::apply);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    toolbar->addWidget(m_status);
    toolbar->addStretch();
    toolbar->addWidget(m_reloadBtn);
    toolbar->addWidget(m_applyBtn);
    root->addLayout(toolbar);

    reload();
}

void ServerSettingsWidget::setStatus(const QString &text, bool error) {
    if (!m_status) return;
    m_status->setStyleSheet(QString("font-size: 12px; color: %1; background: transparent;")
                                .arg(error ? "#ef4444" : "#64748b"));
    m_status->setText(text);
}

void ServerSettingsWidget::reload() {
    if (m_configPath.isEmpty() || m_busy) return;
    m_busy = true;
    m_reloadBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    setStatus("⏳ Chargement…");

    DockerManager *docker = m_docker;
    QString        name   = m_container;
    QString        path   = m_configPath;
    auto *thread = QThread::create([this, docker, name, path]() {
        QString content = docker->readContainerFile(name, path);
        QMetaObject::invokeMethod(this, [this, content]() {
            const QString trimmed = content;
            if (trimmed.trimmed().isEmpty())
                setStatus("Fichier vide ou introuvable — le serveur n'a peut-être pas encore généré sa configuration.");
            else
                setStatus("");
            m_editor->setPlainText(trimmed);
            m_busy = false;
            m_reloadBtn->setEnabled(true);
            m_applyBtn->setEnabled(true);
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ServerSettingsWidget::apply() {
    if (m_configPath.isEmpty() || m_busy) return;
    m_busy = true;
    m_reloadBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    setStatus("⏳ Enregistrement et redémarrage…");

    const QString content = m_editor->toPlainText();
    DockerManager *docker = m_docker;
    QString        name   = m_container;
    QString        path   = m_configPath;
    auto *thread = QThread::create([this, docker, name, path, content]() {
        docker->writeContainerFile(name, path, content);
        docker->restartContainer(name);
        QMetaObject::invokeMethod(this, [this]() {
            setStatus("✓ Configuration appliquée, serveur redémarré.");
            m_busy = false;
            m_reloadBtn->setEnabled(true);
            m_applyBtn->setEnabled(true);
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}
