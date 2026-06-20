#include "ServerDashboard.h"
#include "PluginManagerWidget.h"
#include "GameConsoleWidget.h"
#include "GameLogsWidget.h"
#include "ConfigEditorWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QTabWidget>
#include <QTimer>
#include <QThread>
#include <QMessageBox>

namespace {

const char *CARD_STYLE = R"(
    QFrame#statCard {
        background: #ffffff;
        border-radius: 12px;
    }
)";

const char *PROGRESS_STYLE = R"(
    QProgressBar {
        border: none;
        border-radius: 4px;
        background: #e2e8f0;
        height: 8px;
        text-align: center;
    }
    QProgressBar::chunk {
        border-radius: 4px;
        background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
            stop:0 #6366f1, stop:1 #8b5cf6);
    }
)";

QString fmtGB(float gb) {
    if (gb < 1.f) return QString("%1 MB").arg(int(gb * 1024));
    return QString("%1 GB").arg(QString::number(double(gb), 'f', 1));
}

} // namespace

QFrame *ServerDashboard::makeStatCard(const QString &title,
                                       QWidget *valueWidget,
                                       QWidget *extra) {
    auto *card = new QFrame(this);
    card->setObjectName("statCard");
    card->setStyleSheet(CARD_STYLE);
    card->setMinimumSize(200, 110);

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(8);

    auto *titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet("font-size: 11px; font-weight: 600; color: #94a3b8; background: transparent;");
    lay->addWidget(titleLbl);

    lay->addWidget(valueWidget);
    if (extra) lay->addWidget(extra);
    lay->addStretch();

    return card;
}

ServerDashboard::ServerDashboard(DockerManager *docker,
                                   const QString  &containerName,
                                   GameType        type,
                                   const QString  &rconPass,
                                   const QString  &configFilePath,
                                   QWidget        *parent)
    : QWidget(parent)
    , m_docker(docker)
    , m_containerName(containerName)
    , m_gameType(type)
    , m_rconPass(rconPass)
    , m_configFilePath(configFilePath)
{
    // ── Status row ──────────────────────────────────────────────────────
    m_statusDot  = new QLabel("●", this);
    m_statusText = new QLabel("Vérification…", this);

    auto statusLayout = [this]() {
        auto *w = new QWidget(this);
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);
        m_statusDot->setStyleSheet("font-size: 14px; color: #94a3b8; background: transparent;");
        m_statusText->setStyleSheet("font-size: 14px; font-weight: 600; color: #1e293b; background: transparent;");
        h->addWidget(m_statusDot);
        h->addWidget(m_statusText);
        h->addStretch();
        return w;
    }();

    // ── CPU ─────────────────────────────────────────────────────────────
    m_cpuVal = new QLabel("—", this);
    m_cpuVal->setStyleSheet("font-size: 22px; font-weight: 700; color: #1e293b; background: transparent;");
    m_cpuBar = new QProgressBar(this);
    m_cpuBar->setRange(0, 100);
    m_cpuBar->setValue(0);
    m_cpuBar->setTextVisible(false);
    m_cpuBar->setFixedHeight(8);
    m_cpuBar->setStyleSheet(PROGRESS_STYLE);

    // ── RAM ─────────────────────────────────────────────────────────────
    m_ramVal = new QLabel("—", this);
    m_ramVal->setStyleSheet("font-size: 22px; font-weight: 700; color: #1e293b; background: transparent;");
    m_ramBar = new QProgressBar(this);
    m_ramBar->setRange(0, 100);
    m_ramBar->setValue(0);
    m_ramBar->setTextVisible(false);
    m_ramBar->setFixedHeight(8);
    m_ramBar->setStyleSheet(PROGRESS_STYLE);

    // ── Disk ────────────────────────────────────────────────────────────
    m_diskVal = new QLabel("—", this);
    m_diskVal->setStyleSheet("font-size: 22px; font-weight: 700; color: #1e293b; background: transparent;");
    m_diskBar = new QProgressBar(this);
    m_diskBar->setRange(0, 100);
    m_diskBar->setValue(0);
    m_diskBar->setTextVisible(false);
    m_diskBar->setFixedHeight(8);
    m_diskBar->setStyleSheet(PROGRESS_STYLE);

    // ── Players ─────────────────────────────────────────────────────────
    m_playerVal = new QLabel("—", this);
    m_playerVal->setStyleSheet("font-size: 32px; font-weight: 700; color: #6366f1; background: transparent;");
    m_playerVal->setAlignment(Qt::AlignCenter);

    // ── Backups ─────────────────────────────────────────────────────────
    m_backupCount = new QLabel("0 backup(s)", this);
    m_backupCount->setStyleSheet("font-size: 13px; color: #64748b; background: transparent;");
    m_backupCombo = new QComboBox(this);
    m_backupCombo->setStyleSheet(R"(
        QComboBox {
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 12px;
            background: #f8fafc;
        }
    )");
    auto *btnBackup = new QPushButton("💾 Backup", this);
    btnBackup->setCursor(Qt::PointingHandCursor);
    btnBackup->setStyleSheet(R"(
        QPushButton {
            background: #6366f1;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 5px 12px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton:hover { background: #4f46e5; }
    )");
    auto *btnRestore = new QPushButton("↩ Restaurer", this);
    btnRestore->setCursor(Qt::PointingHandCursor);
    btnRestore->setStyleSheet(R"(
        QPushButton {
            background: #64748b;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 5px 12px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton:hover { background: #475569; }
    )");

    connect(btnBackup,  &QPushButton::clicked, this, &ServerDashboard::onBackup);
    connect(btnRestore, &QPushButton::clicked, this, &ServerDashboard::onRestore);

    // Backup card inner widget
    auto *backupInner = new QWidget(this);
    auto *blay = new QVBoxLayout(backupInner);
    blay->setContentsMargins(0, 0, 0, 0);
    blay->setSpacing(4);
    blay->addWidget(m_backupCount);
    blay->addWidget(m_backupCombo);
    auto *bbtns = new QHBoxLayout;
    bbtns->addWidget(btnBackup);
    bbtns->addWidget(btnRestore);
    blay->addLayout(bbtns);

    // ── Stat cards grid ─────────────────────────────────────────────────
    auto *cpuInner = new QWidget(this);
    auto *cpuLay = new QVBoxLayout(cpuInner);
    cpuLay->setContentsMargins(0,0,0,0); cpuLay->setSpacing(6);
    cpuLay->addWidget(m_cpuVal);
    cpuLay->addWidget(m_cpuBar);

    auto *ramInner = new QWidget(this);
    auto *ramLay = new QVBoxLayout(ramInner);
    ramLay->setContentsMargins(0,0,0,0); ramLay->setSpacing(6);
    ramLay->addWidget(m_ramVal);
    ramLay->addWidget(m_ramBar);

    auto *diskInner = new QWidget(this);
    auto *diskLay = new QVBoxLayout(diskInner);
    diskLay->setContentsMargins(0,0,0,0); diskLay->setSpacing(6);
    diskLay->addWidget(m_diskVal);
    diskLay->addWidget(m_diskBar);

    auto *grid = new QGridLayout;
    grid->setSpacing(12);
    grid->addWidget(makeStatCard("CPU", cpuInner),    0, 0);
    grid->addWidget(makeStatCard("RAM", ramInner),    0, 1);
    grid->addWidget(makeStatCard("DISQUE", diskInner),0, 2);
    grid->addWidget(makeStatCard("JOUEURS", m_playerVal), 1, 0);
    grid->addWidget(makeStatCard("BACKUPS", backupInner), 1, 1, 1, 2);

    // ── Action buttons ──────────────────────────────────────────────────
    auto makeActionBtn = [this](const QString &label, const QString &color,
                                 const QString &hover) {
        auto *b = new QPushButton(label, this);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(40);
        b->setStyleSheet(QString(R"(
            QPushButton { background:%1; color:white; border:none;
                          border-radius:8px; font-size:13px; font-weight:600; }
            QPushButton:hover { background:%2; }
        )").arg(color, hover));
        return b;
    };

    m_startBtn   = makeActionBtn("▶  Démarrer", "#22c55e", "#16a34a");
    m_stopBtn    = makeActionBtn("■  Arrêter",  "#ef4444", "#dc2626");
    m_restartBtn = makeActionBtn("↺  Redémarrer","#f59e0b","#d97706");

    auto *btnUninstall = new QPushButton("🗑  Désinstaller le serveur", this);
    btnUninstall->setCursor(Qt::PointingHandCursor);
    btnUninstall->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ef4444;
            border: 2px solid #ef4444;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 16px;
        }
        QPushButton:hover { background: #fef2f2; }
    )");

    connect(m_startBtn,   &QPushButton::clicked, this, [this]{ m_docker->startContainer(m_containerName); refresh(); });
    connect(m_stopBtn,    &QPushButton::clicked, this, [this]{ m_docker->stopContainer(m_containerName); refresh(); });
    connect(m_restartBtn, &QPushButton::clicked, this, [this]{ m_docker->restartContainer(m_containerName); refresh(); });
    connect(btnUninstall, &QPushButton::clicked, this, &ServerDashboard::uninstallRequested);

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(10);
    actionRow->addWidget(m_startBtn);
    actionRow->addWidget(m_stopBtn);
    actionRow->addWidget(m_restartBtn);
    actionRow->addStretch();
    actionRow->addWidget(btnUninstall);

    // ── Dashboard tab content ────────────────────────────────────────────
    auto *dashPage = new QWidget(this);
    auto *root = new QVBoxLayout(dashPage);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(16);
    root->addWidget(statusLayout);
    root->addLayout(grid);
    root->addLayout(actionRow);

    // ── Tab widget ───────────────────────────────────────────────────────
    auto *tabs = new QTabWidget(this);
    tabs->setStyleSheet(R"(
        QTabWidget::pane { border: none; background: transparent; }
        QTabBar::tab {
            background: transparent;
            padding: 10px 22px;
            font-size: 13px; font-weight: 600;
            color: #64748b;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:selected {
            color: #6366f1;
            border-bottom: 2px solid #6366f1;
        }
        QTabBar::tab:hover:!selected { color: #1e293b; background: #f1f5f9; }
    )");
    tabs->addTab(dashPage, "📊  Tableau de bord");

    if (type == GameType::Minecraft) {
        m_pluginManager = new PluginManagerWidget(docker, containerName, tabs);
        auto *pluginScroll = new QWidget(tabs);
        auto *pluginLay = new QVBoxLayout(pluginScroll);
        pluginLay->setContentsMargins(24, 20, 24, 20);
        pluginLay->addWidget(m_pluginManager);
        tabs->addTab(pluginScroll, "🔌  Plugins");
        connect(tabs, &QTabWidget::currentChanged, this, [this, tabs](int idx) {
            if (m_pluginManager && tabs->widget(idx)->findChild<PluginManagerWidget *>())
                m_pluginManager->refresh();
        });
    }

    // ── Console tab (all games) ──────────────────────────────────────────────
    m_console = new GameConsoleWidget(docker, containerName, type, rconPass, tabs);
    tabs->addTab(m_console, "⌨️  Console");

    // ── Logs tab (all games) ─────────────────────────────────────────────────
    m_logs = new GameLogsWidget(docker, containerName, tabs);
    tabs->addTab(m_logs, "📄  Logs");
    connect(tabs, &QTabWidget::currentChanged, this, [this, tabs](int idx) {
        if (m_logs && tabs->widget(idx) == m_logs)
            m_logs->refresh();
    });

    // ── Configuration tab (games with a known config path) ───────────────────
    // Lets the user edit the server's main config file at any time after
    // installation — it is loaded lazily the first time the tab is opened.
    if (!m_configFilePath.isEmpty()) {
        m_configEditor = new ConfigEditorWidget(docker, containerName, m_configFilePath, tabs);
        tabs->addTab(m_configEditor, "⚙️  Configuration");
        connect(tabs, &QTabWidget::currentChanged, this, [this, tabs](int idx) {
            if (m_configEditor && tabs->widget(idx) == m_configEditor)
                m_configEditor->ensureLoaded();
        });
    }

    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->addWidget(tabs);

    // ── Timer ───────────────────────────────────────────────────────────
    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &ServerDashboard::refresh);
}

ServerDashboard::~ServerDashboard() {
    m_timer->stop();
}

void ServerDashboard::startPolling() {
    refresh();
    m_timer->start();
}

void ServerDashboard::stopPolling() {
    m_timer->stop();
}

void ServerDashboard::refresh() {
    if (m_refreshing) return;
    m_refreshing = true;

    DockerManager *docker = m_docker;
    QString        name   = m_containerName;
    GameType       type   = m_gameType;
    QString        rcon   = m_rconPass;

    auto *thread = QThread::create([this, docker, name, type, rcon]() {
        ServerStats  stats   = docker->getStats(name);
        int          players = 0;
        QStringList  backups;

        if (stats.running) {
            if (type == GameType::Minecraft)
                players = docker->getMinecraftPlayerCount(name);
            else if (type == GameType::CS2)
                players = docker->getCS2PlayerCount(name, rcon);
            // GameType::Generic: player count not supported, stays 0
            backups = docker->listBackups(name);
        }

        QMetaObject::invokeMethod(this, [this, stats, players, backups]() {
            updateDisplay(stats, players, backups);
            m_refreshing = false;
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ServerDashboard::updateDisplay(const ServerStats &s, int players,
                                     const QStringList &backups) {
    // Status
    if (s.running) {
        m_statusDot->setStyleSheet("font-size: 14px; color: #22c55e; background: transparent;");
        m_statusText->setText("En ligne");
    } else {
        m_statusDot->setStyleSheet("font-size: 14px; color: #ef4444; background: transparent;");
        m_statusText->setText("Hors ligne");
    }

    // CPU
    int cpuInt = qBound(0, int(s.cpu), 100);
    m_cpuVal->setText(QString("%1%").arg(cpuInt));
    m_cpuBar->setValue(cpuInt);

    // RAM
    if (s.memTotal > 0) {
        int ramPct = qBound(0, int(s.memUsed / s.memTotal * 100.f), 100);
        m_ramVal->setText(QString("%1 / %2")
            .arg(fmtGB(s.memUsed), fmtGB(s.memTotal)));
        m_ramBar->setValue(ramPct);
    } else {
        m_ramVal->setText("—");
        m_ramBar->setValue(0);
    }

    // Disk
    if (s.diskTotal > 0) {
        int diskPct = qBound(0, int(s.diskUsed / s.diskTotal * 100.f), 100);
        m_diskVal->setText(QString("%1 / %2")
            .arg(fmtGB(s.diskUsed), fmtGB(s.diskTotal)));
        m_diskBar->setValue(diskPct);
    } else {
        m_diskVal->setText("—");
        m_diskBar->setValue(0);
    }

    // Players
    m_playerVal->setText(s.running ? QString::number(players) : "—");

    // Backups
    m_backupCount->setText(QString("%1 backup(s)").arg(backups.size()));
    m_backupCombo->clear();
    for (const auto &b : backups) m_backupCombo->addItem(b);

    // Button states
    m_startBtn->setEnabled(!s.running);
    m_stopBtn->setEnabled(s.running);
    m_restartBtn->setEnabled(s.running);
}

void ServerDashboard::onBackup() {
    m_stopBtn->setEnabled(false);
    m_restartBtn->setEnabled(false);

    DockerManager *docker = m_docker;
    QString name = m_containerName;
    auto *thread = QThread::create([this, docker, name]() {
        docker->createBackup(name);
        QMetaObject::invokeMethod(this, [this]() { refresh(); }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

void ServerDashboard::onRestore() {
    if (m_backupCombo->currentText().isEmpty()) {
        QMessageBox::information(this, "Restaurer", "Aucun backup sélectionné.");
        return;
    }
    QString backup = m_backupCombo->currentText();
    DockerManager *docker = m_docker;
    QString name = m_containerName;
    auto *thread = QThread::create([this, docker, name, backup]() {
        docker->restoreBackup(name, backup);
        QMetaObject::invokeMethod(this, [this]() { refresh(); }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}
