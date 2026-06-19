#include "VMAdminPage.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QTimer>
#include <QThread>
#include <QProcess>
#include <QScrollArea>

namespace {
const char *CARD_STYLE = "QFrame { background: #ffffff; border-radius: 12px; }";
const char *BTN_PRIMARY = R"(
    QPushButton {
        background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
            stop:0 #6366f1, stop:1 #818cf8);
        color: white; border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 16px; min-height: 34px;
    }
    QPushButton:hover {
        background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
            stop:0 #4f46e5, stop:1 #6366f1);
    }
    QPushButton:disabled { background: #94a3b8; }
)";
const char *BTN_SECONDARY = R"(
    QPushButton {
        background: #f1f5f9; color: #374151;
        border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 16px; min-height: 34px;
    }
    QPushButton:hover { background: #e2e8f0; }
)";
const char *BTN_DANGER = R"(
    QPushButton {
        background: #fef2f2; color: #ef4444;
        border: none; border-radius: 8px;
        font-size: 12px; font-weight: 600; padding: 0 16px; min-height: 34px;
    }
    QPushButton:hover { background: #fee2e2; }
)";
} // namespace

VMAdminPage::VMAdminPage(const VMInstance &vm, QWidget *parent)
    : QDialog(parent), m_vm(vm)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);
    setMinimumSize(820, 580);
    setWindowTitle("Administration — " + vm.name);

    m_docker = new DockerManager(this);
    if (vm.provider != VMProvider::Local) {
        m_docker->setRemoteMode(vm.host, vm.sshPort, vm.sshUser, vm.sshKeyPath);
    }

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    // Header
    auto *headerRow = new QHBoxLayout;
    auto *titleLbl  = new QLabel("🖥  " + vm.name, this);
    titleLbl->setStyleSheet("font-size: 20px; font-weight: 700; color: #1e293b;");

    QString provStr;
    switch (vm.provider) {
    case VMProvider::Local:    provStr = "Local Docker"; break;
    case VMProvider::Scaleway: provStr = "☁ Scaleway · " + vm.region + " · " + vm.instanceType; break;
    default:                   provStr = "🌐 VM distante · " + vm.host; break;
    }
    auto *provLbl = new QLabel(provStr, this);
    provLbl->setStyleSheet(
        "font-size: 11px; font-weight: 600; color: #6366f1;"
        "background: #eef2ff; border-radius: 6px; padding: 4px 10px;");

    headerRow->addWidget(titleLbl);
    headerRow->addStretch();
    headerRow->addWidget(provLbl);
    root->addLayout(headerRow);

    // Tabs
    auto *tabs = new QTabWidget(this);
    tabs->setStyleSheet(R"(
        QTabWidget::pane { border: none; }
        QTabBar::tab {
            background: #f1f5f9; color: #64748b;
            border: none; border-radius: 6px;
            padding: 8px 16px; margin-right: 4px;
            font-size: 12px; font-weight: 600;
        }
        QTabBar::tab:selected { background: #6366f1; color: white; }
    )");

    tabs->addTab(buildOverviewTab(), "📋  Vue d'ensemble");
    tabs->addTab(buildServersTab(),  "🎮  Serveurs");
    tabs->addTab(buildMetricsTab(),  "📊  Métriques");
    tabs->addTab(buildConsoleTab(),  "💻  Console");

    root->addWidget(tabs, 1);

    // Start metrics polling
    m_metricsTimer = new QTimer(this);
    m_metricsTimer->setInterval(8000);
    connect(m_metricsTimer, &QTimer::timeout, this, &VMAdminPage::refreshMetrics);
    connect(tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 1) refreshServers();
        if (idx == 2) {
            refreshMetrics();
            m_metricsTimer->start();
        } else {
            m_metricsTimer->stop();
        }
    });
    refreshServers();
}

QWidget *VMAdminPage::buildOverviewTab()
{
    auto *w    = new QWidget;
    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(8, 16, 8, 8);
    root->setSpacing(12);

    auto *card = new QFrame(w);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(CARD_STYLE);

    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(24, 20, 24, 20);
    grid->setSpacing(12);

    auto addRow = [&](int row, const QString &label, const QString &value) {
        auto *lbl = new QLabel(label, card);
        lbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b;");
        auto *val = new QLabel(value, card);
        val->setStyleSheet("font-size: 13px; color: #1e293b;");
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(lbl, row, 0);
        grid->addWidget(val, row, 1);
    };

    addRow(0, "Nom",           m_vm.name);
    addRow(1, "IP / Hôte",    m_vm.host.isEmpty() ? "—" : m_vm.host);
    addRow(2, "Port SSH",      QString::number(m_vm.sshPort));
    addRow(3, "Utilisateur",   m_vm.sshUser);
    addRow(4, "Région",        m_vm.region.isEmpty() ? "—" : m_vm.region);
    addRow(5, "Type",          m_vm.instanceType.isEmpty() ? "—" : m_vm.instanceType);
    addRow(6, "Clé SSH",       m_vm.sshKeyPath.isEmpty() ? "—" : m_vm.sshKeyPath);

    m_vmStatusLabel = new QLabel("⚡ Statut inconnu", card);
    m_vmStatusLabel->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b;");
    grid->addWidget(new QLabel("Statut Docker", card), 7, 0);
    grid->addWidget(m_vmStatusLabel, 7, 1);

    root->addWidget(card);

    auto *testBtn = new QPushButton("🔌  Vérifier la connexion", w);
    testBtn->setFixedHeight(38);
    testBtn->setCursor(Qt::PointingHandCursor);
    testBtn->setStyleSheet(BTN_SECONDARY);
    connect(testBtn, &QPushButton::clicked, this, [this]() {
        if (m_vmStatusLabel) {
            m_vmStatusLabel->setStyleSheet("font-size: 12px; color: #6366f1;");
            m_vmStatusLabel->setText("⏳ Test en cours…");
        }
        DockerManager *d = m_docker;
        QLabel *lbl = m_vmStatusLabel;
        auto *t = QThread::create([d, lbl]() {
            QString info = d->dockerInfo();
            bool ok = !info.isEmpty() && !info.startsWith("Error");
            QMetaObject::invokeMethod(lbl, [lbl, ok]() {
                if (ok) {
                    lbl->setStyleSheet("font-size: 12px; color: #22c55e; font-weight: 600;");
                    lbl->setText("✅ Docker opérationnel");
                } else {
                    lbl->setStyleSheet("font-size: 12px; color: #ef4444;");
                    lbl->setText("❌ Connexion échouée");
                }
            }, Qt::QueuedConnection);
        });
        t->start();
        connect(t, &QThread::finished, t, &QThread::deleteLater);
    });
    root->addWidget(testBtn, 0, Qt::AlignLeft);
    root->addStretch();
    return w;
}

QWidget *VMAdminPage::buildServersTab()
{
    auto *w    = new QWidget;
    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(8, 16, 8, 8);
    root->setSpacing(10);

    auto *infoLbl = new QLabel("Conteneurs Docker détectés sur cette VM :", w);
    infoLbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b;");
    root->addWidget(infoLbl);

    m_serverList = new QListWidget(w);
    m_serverList->setStyleSheet(R"(
        QListWidget {
            background: #ffffff; border-radius: 10px; border: none;
            font-size: 13px; color: #1e293b;
        }
        QListWidget::item { padding: 10px 16px; border-bottom: 1px solid #f1f5f9; }
        QListWidget::item:selected {
            background: #eef2ff; color: #4f46e5; border-radius: 6px;
        }
    )");
    root->addWidget(m_serverList, 1);

    auto *btnRow = new QHBoxLayout;
    auto *startBtn   = new QPushButton("▶  Démarrer", w);
    auto *stopBtn    = new QPushButton("⏹  Arrêter",  w);
    auto *restartBtn = new QPushButton("🔄  Redémarrer", w);
    auto *refreshBtn = new QPushButton("↻  Actualiser", w);

    for (auto *b : {startBtn, stopBtn, restartBtn, refreshBtn}) {
        b->setFixedHeight(36);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(BTN_SECONDARY);
        btnRow->addWidget(b);
    }
    startBtn->setStyleSheet(BTN_PRIMARY);
    stopBtn->setStyleSheet(BTN_DANGER);

    connect(startBtn,   &QPushButton::clicked, this, &VMAdminPage::onStart);
    connect(stopBtn,    &QPushButton::clicked, this, &VMAdminPage::onStop);
    connect(restartBtn, &QPushButton::clicked, this, &VMAdminPage::onRestart);
    connect(refreshBtn, &QPushButton::clicked, this, &VMAdminPage::refreshServers);

    btnRow->addStretch();
    root->addLayout(btnRow);
    return w;
}

QWidget *VMAdminPage::buildMetricsTab()
{
    auto *w    = new QWidget;
    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(8, 16, 8, 8);
    root->setSpacing(12);

    auto makeMetricCard = [&](const QString &title, QProgressBar *&bar, QLabel *&lbl) {
        auto *card = new QFrame(w);
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setStyleSheet(CARD_STYLE);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(20, 16, 20, 16);
        cl->setSpacing(8);

        auto *titleRow = new QHBoxLayout;
        auto *tl = new QLabel(title, card);
        tl->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151;");
        lbl = new QLabel("—", card);
        lbl->setStyleSheet("font-size: 12px; color: #64748b;");
        titleRow->addWidget(tl);
        titleRow->addStretch();
        titleRow->addWidget(lbl);
        cl->addLayout(titleRow);

        bar = new QProgressBar(card);
        bar->setRange(0, 100);
        bar->setValue(0);
        bar->setTextVisible(false);
        bar->setFixedHeight(8);
        bar->setStyleSheet(R"(
            QProgressBar {
                background: #f1f5f9; border-radius: 4px; border: none;
            }
            QProgressBar::chunk {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #6366f1, stop:1 #818cf8);
                border-radius: 4px;
            }
        )");
        cl->addWidget(bar);
        return card;
    };

    root->addWidget(makeMetricCard("CPU",  m_cpuBar,  m_cpuLbl));
    root->addWidget(makeMetricCard("RAM",  m_ramBar,  m_ramLbl));
    root->addWidget(makeMetricCard("Disk", m_diskBar, m_diskLbl));

    auto *refreshBtn = new QPushButton("↻  Actualiser les métriques", w);
    refreshBtn->setFixedHeight(36);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(BTN_SECONDARY);
    connect(refreshBtn, &QPushButton::clicked, this, &VMAdminPage::refreshMetrics);
    root->addWidget(refreshBtn, 0, Qt::AlignLeft);
    root->addStretch();
    return w;
}

QWidget *VMAdminPage::buildConsoleTab()
{
    auto *w    = new QWidget;
    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(8, 16, 8, 8);
    root->setSpacing(10);

    auto *infoLbl = new QLabel(
        "Console SSH — les commandes sont exécutées directement sur la VM.", w);
    infoLbl->setStyleSheet("font-size: 12px; color: #64748b;");
    root->addWidget(infoLbl);

    m_consoleOutput = new QTextEdit(w);
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setStyleSheet(R"(
        QTextEdit {
            background: #0f172a; color: #a5f3fc;
            font-family: "Courier New", monospace; font-size: 12px;
            border-radius: 10px; border: none; padding: 12px;
        }
    )");
    root->addWidget(m_consoleOutput, 1);

    auto *inputRow = new QHBoxLayout;
    auto *promptLbl = new QLabel("$", w);
    promptLbl->setStyleSheet(
        "font-family: monospace; font-size: 13px; font-weight: 700; color: #6366f1;");

    m_consoleInput = new QLineEdit(w);
    m_consoleInput->setPlaceholderText("Entrez une commande…");
    m_consoleInput->setStyleSheet(R"(
        QLineEdit {
            border: 2px solid #e2e8f0; border-radius: 8px;
            padding: 8px 12px; font-size: 13px;
            color: #1e293b; background: #f8fafc;
        }
        QLineEdit:focus { border-color: #6366f1; background: #ffffff; }
    )");
    connect(m_consoleInput, &QLineEdit::returnPressed,
            this, &VMAdminPage::onSendConsoleCmd);

    auto *sendBtn = new QPushButton("Envoyer", w);
    sendBtn->setFixedHeight(38);
    sendBtn->setCursor(Qt::PointingHandCursor);
    sendBtn->setStyleSheet(BTN_PRIMARY);
    connect(sendBtn, &QPushButton::clicked, this, &VMAdminPage::onSendConsoleCmd);

    inputRow->addWidget(promptLbl);
    inputRow->addWidget(m_consoleInput, 1);
    inputRow->addWidget(sendBtn);
    root->addLayout(inputRow);
    return w;
}

void VMAdminPage::refreshServers()
{
    if (!m_serverList) return;
    m_serverList->clear();
    m_serverList->addItem("⏳ Chargement…");

    DockerManager *d    = m_docker;
    QListWidget   *list = m_serverList;
    auto *t = QThread::create([d, list]() {
        QString out = d->runRaw({"ps", "-a",
                                  "--format", "{{.Names}}\\t{{.Status}}\\t{{.Image}}"});
        QMetaObject::invokeMethod(list, [list, out]() {
            list->clear();
            if (out.trimmed().isEmpty()) {
                list->addItem("Aucun conteneur trouvé");
                return;
            }
            for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
                QStringList parts = line.split('\t');
                QString name   = parts.value(0);
                QString status = parts.value(1);
                QString image  = parts.value(2);
                bool running   = status.startsWith("Up");
                QString icon   = running ? "🟢  " : "🔴  ";
                list->addItem(icon + name + "  ·  " + status + "  ·  " + image);
            }
        }, Qt::QueuedConnection);
    });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
}

void VMAdminPage::refreshMetrics()
{
    if (!m_cpuBar) return;

    DockerManager *d    = m_docker;
    QProgressBar  *cpuB = m_cpuBar,  *ramB = m_ramBar,  *dskB = m_diskBar;
    QLabel        *cpuL = m_cpuLbl,  *ramL = m_ramLbl,  *dskL = m_diskLbl;

    auto *t = QThread::create([d, cpuB, ramB, dskB, cpuL, ramL, dskL]() {
        // Aggregate stats for all running containers
        QString statsOut = d->runRaw(
            {"stats", "--no-stream", "--format",
             "{{.CPUPerc}}\\t{{.MemUsage}}"});

        float totalCpu = 0.f;
        float memUsed  = 0.f, memTotal = 0.f;

        for (const QString &line : statsOut.split('\n', Qt::SkipEmptyParts)) {
            QStringList p = line.split('\t');
            if (p.size() < 2) continue;
            float cpu = p[0].replace('%', "").toFloat();
            totalCpu += cpu;

            // MemUsage format: "1.5GiB / 8GiB"
            QString mem = p[1];
            QStringList mparts = mem.split(" / ");
            auto parseVal = [](const QString &s) -> float {
                QString v = s.trimmed();
                float mult = 1.f;
                if (v.endsWith("GiB")) { mult = 1024.f; v.chop(3); }
                else if (v.endsWith("MiB")) { mult = 1.f; v.chop(3); }
                else if (v.endsWith("KiB")) { mult = 1.f/1024.f; v.chop(3); }
                return v.toFloat() * mult;
            };
            if (mparts.size() >= 2) {
                memUsed  += parseVal(mparts[0]);
                float tot = parseVal(mparts[1]);
                if (tot > memTotal) memTotal = tot;
            }
        }

        int cpuPct = qMin(100, (int)totalCpu);
        int ramPct = (memTotal > 0) ? (int)(memUsed / memTotal * 100.f) : 0;

        // Disk via df
        QString dfOut = d->runRaw({"exec", "-i",
                                    "$(docker ps -q | head -1)",
                                    "df", "-h", "/"});
        Q_UNUSED(dfOut); // simplified — just show RAM/CPU for now

        QMetaObject::invokeMethod(cpuB, [cpuB, ramB, cpuL, ramL, cpuPct, ramPct,
                                          memUsed, memTotal]() {
            cpuB->setValue(cpuPct);
            ramB->setValue(ramPct);
            cpuL->setText(QString::number(cpuPct) + "%");
            ramL->setText(QString::number((int)memUsed) + " MiB / "
                          + QString::number((int)memTotal) + " MiB");
        }, Qt::QueuedConnection);
    });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
}

static QString selectedContainer(QListWidget *list)
{
    QListWidgetItem *item = list ? list->currentItem() : nullptr;
    if (!item) return {};
    QString text = item->text();
    // Format: "🟢  name  ·  status  ·  image"
    text = text.mid(4); // strip icon
    return text.split("  ·  ").value(0).trimmed();
}

void VMAdminPage::onStart()
{
    QString name = selectedContainer(m_serverList);
    if (name.isEmpty()) return;
    DockerManager *d = m_docker;
    auto *t = QThread::create([d, name]() { d->startContainer(name); });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
    connect(t, &QThread::finished, this, &VMAdminPage::refreshServers);
}

void VMAdminPage::onStop()
{
    QString name = selectedContainer(m_serverList);
    if (name.isEmpty()) return;
    DockerManager *d = m_docker;
    auto *t = QThread::create([d, name]() { d->stopContainer(name); });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
    connect(t, &QThread::finished, this, &VMAdminPage::refreshServers);
}

void VMAdminPage::onRestart()
{
    QString name = selectedContainer(m_serverList);
    if (name.isEmpty()) return;
    DockerManager *d = m_docker;
    auto *t = QThread::create([d, name]() { d->restartContainer(name); });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
    connect(t, &QThread::finished, this, &VMAdminPage::refreshServers);
}

void VMAdminPage::onSendConsoleCmd()
{
    if (!m_consoleInput || !m_consoleOutput) return;
    QString cmd = m_consoleInput->text().trimmed();
    if (cmd.isEmpty()) return;
    m_consoleInput->clear();

    m_consoleOutput->append("<span style='color:#818cf8'>$ " + cmd + "</span>");

    DockerManager *d      = m_docker;
    QTextEdit     *output = m_consoleOutput;
    VMInstance     vm     = m_vm;

    auto *t = QThread::create([d, vm, cmd, output]() {
        QProcess proc;
        QStringList args = {
            "-o", "StrictHostKeyChecking=no",
            "-p", QString::number(vm.sshPort),
        };
        if (!vm.sshKeyPath.isEmpty())
            args << "-i" << vm.sshKeyPath;
        args << QString("%1@%2").arg(vm.sshUser, vm.host);
        args << cmd;

        proc.start("ssh", args);
        proc.waitForFinished(30000);
        QString out = proc.readAllStandardOutput();
        QString err = proc.readAllStandardError();
        QString combined = out + err;

        QMetaObject::invokeMethod(output, [output, combined]() {
            output->append("<span style='color:#e2e8f0'>"
                           + combined.toHtmlEscaped() + "</span>");
        }, Qt::QueuedConnection);
    });
    t->start();
    connect(t, &QThread::finished, t, &QThread::deleteLater);
}
