#include "GameConsoleWidget.h"
#include "DockerManager.h"
#include "ServerDashboard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QThread>
#include <QDateTime>

static const char *HELP_TEXT =
    "  Commandes disponibles :\n"
    "  ────────────────────────────────────────────────────\n"
    "  help               Afficher ce menu\n"
    "  list               Lister les joueurs connectés\n"
    "  say  &lt;message&gt;     Envoyer une annonce à tous\n"
    "  kick &lt;joueur&gt;      Expulser un joueur\n"
    "  ban  &lt;joueur&gt;      Bannir un joueur\n"
    "  unban &lt;joueur&gt;     Débannir un joueur\n"
    "  raw  &lt;commande&gt;    Commande brute → serveur\n"
    "  clear              Effacer la console\n"
    "  ────────────────────────────────────────────────────";

GameConsoleWidget::GameConsoleWidget(DockerManager *docker,
                                     const QString &containerName,
                                     GameType       gameType,
                                     const QString &rconPass,
                                     QWidget       *parent)
    : QWidget(parent)
    , m_docker(docker)
    , m_container(containerName)
    , m_gameType(gameType)
    , m_rconPass(rconPass)
{
    // ── Output area ──────────────────────────────────────────────────────────
    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setStyleSheet(R"(
        QTextEdit {
            background: #0d1117;
            color: #a0f0a0;
            font-family: Consolas, "Courier New", monospace;
            font-size: 12px;
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 8px;
        }
        QScrollBar:vertical {
            background: #161b22;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #30363d;
            border-radius: 4px;
        }
    )");

    // ── Input row ────────────────────────────────────────────────────────────
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Entrer une commande  (ex: help, list, say Bonjour !)");
    m_input->setStyleSheet(R"(
        QLineEdit {
            background: #161b22;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 7px 12px;
            font-family: Consolas, "Courier New", monospace;
            font-size: 12px;
        }
        QLineEdit:focus { border-color: #6366f1; }
    )");

    auto *btnSend = new QPushButton("Envoyer", this);
    btnSend->setCursor(Qt::PointingHandCursor);
    btnSend->setFixedHeight(34);
    btnSend->setStyleSheet(R"(
        QPushButton {
            background: #6366f1;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 0 18px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton:hover { background: #4f46e5; }
    )");

    connect(btnSend, &QPushButton::clicked, this, &GameConsoleWidget::onSendCommand);
    connect(m_input, &QLineEdit::returnPressed,  this, &GameConsoleWidget::onSendCommand);

    auto *prompt = new QLabel(">", this);
    prompt->setStyleSheet(
        "color:#6366f1; font-weight:bold; font-size:14px;"
        "font-family:Consolas,monospace; background:transparent;");

    auto *inputRow = new QHBoxLayout;
    inputRow->setSpacing(8);
    inputRow->addWidget(prompt);
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(btnSend);

    // ── Root layout ──────────────────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(10);
    root->addWidget(m_output, 1);
    root->addLayout(inputRow);

    // ── Welcome ──────────────────────────────────────────────────────────────
    appendOutput("  Console Admin — tapez 'help' pour la liste des commandes", "#6366f1");
    appendOutput("  " + QDateTime::currentDateTime().toString("dd/MM/yyyy  hh:mm:ss"), "#4a5568");
}

void GameConsoleWidget::appendOutput(const QString &line, const QString &colorHex) {
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat fmt;
    fmt.setForeground(QColor(colorHex));
    fmt.setFontFamilies({"Consolas", "Courier New", "monospace"});
    fmt.setFontPointSize(10);

    if (!m_output->document()->isEmpty())
        cursor.insertText("\n", fmt);
    cursor.insertText(line, fmt);

    m_output->moveCursor(QTextCursor::End);
    m_output->ensureCursorVisible();
}

void GameConsoleWidget::onSendCommand() {
    QString input = m_input->text().trimmed();
    if (input.isEmpty()) return;
    m_input->clear();
    appendOutput("> " + input, "#e6edf3");
    handleCommand(input);
}

void GameConsoleWidget::handleCommand(const QString &input) {
    int spaceIdx = input.indexOf(' ');
    QString cmd  = (spaceIdx < 0 ? input : input.left(spaceIdx)).toLower();
    QString rest = (spaceIdx < 0 ? QString() : input.mid(spaceIdx + 1).trimmed());

    // ── Local commands ───────────────────────────────────────────────────────
    if (cmd == "clear") {
        m_output->clear();
        return;
    }
    if (cmd == "help") {
        appendOutput(QString::fromUtf8(HELP_TEXT), "#7dd3fc");
        return;
    }

    // ── Build docker exec args ───────────────────────────────────────────────
    QStringList execArgs;
    const bool isMc  = (m_gameType == GameType::Minecraft);
    const bool isCs2 = (m_gameType == GameType::CS2);

    auto rconArgs = [&](const QString &gameCmd) -> QStringList {
        return {"rcon", "-H", "127.0.0.1", "-p", "27015", "-P", m_rconPass, gameCmd};
    };

    auto notSupported = [&]() {
        appendOutput("  Commande non disponible pour ce type de serveur.", "#f87171");
        appendOutput("  Utilisez:  raw <commande_native_du_jeu>", "#f87171");
    };

    if (cmd == "list") {
        if      (isMc)  execArgs = {"rcon-cli", "list"};
        else if (isCs2) execArgs = rconArgs("status");
        else { notSupported(); return; }
    }
    else if (cmd == "say") {
        if (rest.isEmpty()) { appendOutput("  Usage: say <message>", "#f87171"); return; }
        if      (isMc)  execArgs = {"rcon-cli", "say " + rest};
        else if (isCs2) execArgs = rconArgs("say " + rest);
        else { notSupported(); return; }
    }
    else if (cmd == "kick") {
        if (rest.isEmpty()) { appendOutput("  Usage: kick <joueur>", "#f87171"); return; }
        if      (isMc)  execArgs = {"rcon-cli", "kick " + rest};
        else if (isCs2) execArgs = rconArgs("kick " + rest);
        else { notSupported(); return; }
    }
    else if (cmd == "ban") {
        if (rest.isEmpty()) { appendOutput("  Usage: ban <joueur>", "#f87171"); return; }
        if      (isMc)  execArgs = {"rcon-cli", "ban " + rest};
        else if (isCs2) execArgs = rconArgs("banid 0 " + rest);
        else { notSupported(); return; }
    }
    else if (cmd == "unban") {
        if (rest.isEmpty()) { appendOutput("  Usage: unban <joueur>", "#f87171"); return; }
        if      (isMc)  execArgs = {"rcon-cli", "pardon " + rest};
        else if (isCs2) execArgs = rconArgs("removeid " + rest);
        else { notSupported(); return; }
    }
    else if (cmd == "raw") {
        if (rest.isEmpty()) { appendOutput("  Usage: raw <commande>", "#f87171"); return; }
        if      (isMc)  execArgs = {"rcon-cli", rest};
        else if (isCs2) execArgs = rconArgs(rest);
        else            execArgs = {"sh", "-c", rest};
    }
    else {
        appendOutput("  Commande inconnue: '" + cmd + "'.  Tapez 'help'.", "#f87171");
        return;
    }

    // ── Execute async ────────────────────────────────────────────────────────
    DockerManager *docker    = m_docker;
    QString        container = m_container;

    auto *thread = QThread::create([this, docker, container, execArgs]() {
        QString out = docker->execInContainer(container, execArgs, 10000);
        QString trimmed = out.trimmed();
        QMetaObject::invokeMethod(this, [this, trimmed]() {
            if (trimmed.isEmpty()) {
                appendOutput("  (aucune réponse du serveur)", "#4a5568");
            } else {
                for (const QString &line : trimmed.split('\n', Qt::SkipEmptyParts))
                    appendOutput("  " + line);
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}
