#include "GameLogsWidget.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QTimer>
#include <QGuiApplication>
#include <QClipboard>
#include <QThread>

GameLogsWidget::GameLogsWidget(DockerManager *docker,
                               const QString &containerName,
                               QWidget       *parent)
    : QWidget(parent)
    , m_docker(docker)
    , m_container(containerName)
{
    // ── Output area ──────────────────────────────────────────────────────────
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(5000);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_output->setStyleSheet(R"(
        QPlainTextEdit {
            background: #0d1117;
            color: #c9d1d9;
            font-family: Consolas, "Courier New", monospace;
            font-size: 12px;
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 8px;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: #161b22;
            width: 8px;
            height: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: #30363d;
            border-radius: 4px;
        }
    )");

    // ── Toolbar ──────────────────────────────────────────────────────────────
    auto *tailLbl = new QLabel("Lignes :", this);
    tailLbl->setStyleSheet("font-size: 12px; color: #64748b; background: transparent;");

    m_tailCombo = new QComboBox(this);
    m_tailCombo->addItems({"100", "200", "500", "1000"});
    m_tailCombo->setCurrentText("200");
    m_tailCombo->setStyleSheet(R"(
        QComboBox {
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 12px;
            background: #f8fafc;
            color: #1e293b;
        }
    )");
    connect(m_tailCombo, &QComboBox::currentTextChanged, this, [this]{ refresh(); });

    auto *btnRefresh = new QPushButton("⟳ Rafraîchir", this);
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet(R"(
        QPushButton {
            background: #6366f1; color: white; border: none;
            border-radius: 6px; padding: 6px 14px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #4f46e5; }
    )");
    connect(btnRefresh, &QPushButton::clicked, this, &GameLogsWidget::refresh);

    auto *btnCopy = new QPushButton("⧉ Copier", this);
    btnCopy->setCursor(Qt::PointingHandCursor);
    btnCopy->setStyleSheet(R"(
        QPushButton {
            background: #f1f5f9; color: #475569; border: none;
            border-radius: 6px; padding: 6px 14px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #e2e8f0; }
    )");
    connect(btnCopy, &QPushButton::clicked, this, [this]{
        QGuiApplication::clipboard()->setText(m_output->toPlainText());
    });

    auto *btnClear = new QPushButton("Effacer", this);
    btnClear->setCursor(Qt::PointingHandCursor);
    btnClear->setStyleSheet(R"(
        QPushButton {
            background: #f1f5f9; color: #475569; border: none;
            border-radius: 6px; padding: 6px 14px;
            font-size: 12px; font-weight: 600;
        }
        QPushButton:hover { background: #e2e8f0; }
    )");
    connect(btnClear, &QPushButton::clicked, m_output, &QPlainTextEdit::clear);

    m_autoRefresh = new QCheckBox("Auto", this);
    m_autoRefresh->setStyleSheet("font-size: 12px; color: #374151; background: transparent;");

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    toolbar->addWidget(tailLbl);
    toolbar->addWidget(m_tailCombo);
    toolbar->addWidget(btnRefresh);
    toolbar->addWidget(m_autoRefresh);
    toolbar->addStretch();
    toolbar->addWidget(btnCopy);
    toolbar->addWidget(btnClear);

    // ── Root layout ──────────────────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(10);
    root->addLayout(toolbar);
    root->addWidget(m_output, 1);

    // ── Auto-refresh timer ─────────────────────────────────────────────────────
    m_timer = new QTimer(this);
    m_timer->setInterval(3000);
    connect(m_timer, &QTimer::timeout, this, &GameLogsWidget::refresh);
    connect(m_autoRefresh, &QCheckBox::toggled, this, [this](bool on){
        if (on) { refresh(); m_timer->start(); }
        else      m_timer->stop();
    });

    setNote("Chargement des logs…");
    refresh();
}

void GameLogsWidget::setNote(const QString &text) {
    m_output->setPlainText(text);
}

void GameLogsWidget::refresh() {
    if (m_loading) return;
    m_loading = true;

    const int tail = m_tailCombo->currentText().toInt();
    DockerManager *docker    = m_docker;
    QString        container = m_container;

    auto *thread = QThread::create([this, docker, container, tail]() {
        QString logs = docker->containerLogs(container, tail);
        QMetaObject::invokeMethod(this, [this, logs]() {
            // Preserve scroll-to-bottom only when the user is already at the bottom.
            QScrollBar *bar = m_output->verticalScrollBar();
            const bool atBottom = bar->value() >= bar->maximum() - 4;

            const QString trimmed = logs.trimmed();
            m_output->setPlainText(trimmed.isEmpty()
                ? "(aucun log — le conteneur n'a peut-être pas encore démarré)"
                : trimmed);

            if (atBottom) bar->setValue(bar->maximum());
            m_loading = false;
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}
