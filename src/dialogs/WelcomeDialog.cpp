#include "WelcomeDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QSettings>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

namespace {

// Une « marche » du mini tuto : pastille numérotée + emoji + texte.
QFrame *makeStep(const QString &num, const QString &emoji, const QString &html)
{
    auto *row = new QFrame;
    row->setStyleSheet(R"(
        QFrame {
            background: rgba(255,255,255,0.06);
            border: 1px solid rgba(255,255,255,0.12);
            border-radius: 12px;
        }
    )");
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(12);

    auto *badge = new QLabel(num);
    badge->setFixedSize(34, 34);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
            stop:0 #ec4899, stop:1 #6366f1);
        color: #ffffff; font-size: 16px; font-weight: 900; border-radius: 17px;
    )");
    lay->addWidget(badge, 0, Qt::AlignTop);

    auto *txt = new QLabel(QString("%1  %2").arg(emoji, html));
    txt->setTextFormat(Qt::RichText);
    txt->setWordWrap(true);
    txt->setOpenExternalLinks(true);
    txt->setStyleSheet("background: transparent; color: #e2e8f0; font-size: 13px;");
    lay->addWidget(txt, 1);

    return row;
}

} // namespace

WelcomeDialog::WelcomeDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Bienvenue !");
    setMinimumWidth(540);
    setStyleSheet("QDialog { background-color: #1e1b4b; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Bannière d'accueil ultra kitsch ─────────────────────────────────
    auto *header = new QLabel(
        "🎉🐳🎮<br>BIENVENUE, GAME MASTER !<br>"
        "<span style='font-size:13px; font-weight:600; color:#fde68a'>"
        "Avant de lancer des serveurs, on adopte une baleine 🐳</span>");
    header->setTextFormat(Qt::RichText);
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet(R"(
        color: #ffffff; font-size: 22px; font-weight: 900; padding: 24px 16px;
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 #f59e0b, stop:0.5 #ec4899, stop:1 #8b5cf6);
    )");
    root->addWidget(header);

    auto *body = new QVBoxLayout();
    body->setContentsMargins(20, 18, 20, 8);
    body->setSpacing(12);

    auto *intro = new QLabel(
        "GameServer Manager fait tourner vos serveurs dans <b>Docker</b>. "
        "C'est gratuit, c'est magique, et il faut juste l'installer une fois. "
        "Suivez le guide 👇");
    intro->setWordWrap(true);
    intro->setStyleSheet("background: transparent; color: #cbd5e1; font-size: 13px;");
    body->addWidget(intro);

    body->addWidget(makeStep("1", "⬇️",
        "Téléchargez <b>Docker Desktop</b> sur "
        "<a style='color:#93c5fd' href='https://www.docker.com/products/docker-desktop/'>"
        "docker.com</a> (bouton ci-dessous)."));
    body->addWidget(makeStep("2", "🛠️",
        "Lancez l'installateur, cliquez sur <b>Suivant</b> partout, puis "
        "<b>redémarrez</b> le PC si on vous le demande."));
    body->addWidget(makeStep("3", "🐳",
        "Ouvrez Docker Desktop et attendez que la <b>baleine</b> devienne verte "
        "dans la barre des tâches. Ça mijote tranquille en fond."));
    body->addWidget(makeStep("4", "🚀",
        "Revenez ici, choisissez un jeu, et <b>déployez</b> ! "
        "Besoin d'aide plus tard ? L'icône <b>📖</b> en bas à gauche est là."));

    // ── Zone de vérification ────────────────────────────────────────────
    m_status = new QLabel("🤔  Docker est-il prêt ? Cliquez sur « Vérifier ».");
    m_status->setWordWrap(true);
    m_status->setStyleSheet(R"(
        background: rgba(255,255,255,0.04);
        border: 1px dashed rgba(255,255,255,0.2);
        border-radius: 10px; padding: 10px 12px;
        color: #fde68a; font-size: 13px; font-weight: 600;
    )");
    body->addWidget(m_status);

    root->addLayout(body);

    // ── Boutons ─────────────────────────────────────────────────────────
    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(20, 8, 20, 18);
    btnRow->setSpacing(10);

    auto *dlBtn = new QPushButton("🐳  Télécharger Docker");
    dlBtn->setCursor(Qt::PointingHandCursor);
    dlBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(56,189,248,0.18); color: #ffffff;
            border: 1px solid #38bdf8; border-radius: 10px;
            padding: 10px 16px; font-size: 13px; font-weight: 700;
        }
        QPushButton:hover { background: rgba(56,189,248,0.35); }
    )");
    connect(dlBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(
            QUrl("https://www.docker.com/products/docker-desktop/"));
    });
    btnRow->addWidget(dlBtn);

    auto *checkBtn = new QPushButton("🔍  Vérifier");
    checkBtn->setCursor(Qt::PointingHandCursor);
    checkBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.08); color: #ffffff;
            border: 1px solid rgba(255,255,255,0.2); border-radius: 10px;
            padding: 10px 16px; font-size: 13px; font-weight: 700;
        }
        QPushButton:hover { background: rgba(129,140,248,0.3); border-color: #818cf8; }
    )");
    connect(checkBtn, &QPushButton::clicked, this, &WelcomeDialog::checkDocker);
    btnRow->addWidget(checkBtn);

    btnRow->addStretch();

    auto *goBtn = new QPushButton("C'est parti !  🎮");
    goBtn->setCursor(Qt::PointingHandCursor);
    goBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ec4899, stop:1 #8b5cf6);
            color: #ffffff; border: none; border-radius: 10px;
            padding: 10px 22px; font-size: 13px; font-weight: 800;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #f472b6, stop:1 #a78bfa);
        }
    )");
    connect(goBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(goBtn);

    root->addLayout(btnRow);
}

void WelcomeDialog::checkDocker()
{
    m_status->setText("⏳  On regarde si la baleine est dans le coin…");

    QProcess proc;
    proc.start("docker", {"--version"});
    const bool ok = proc.waitForFinished(4000)
                    && proc.exitStatus() == QProcess::NormalExit
                    && proc.exitCode() == 0;

    if (ok) {
        const QString ver = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        m_status->setStyleSheet(
            "background: rgba(34,197,94,0.12); border: 1px solid #22c55e;"
            "border-radius: 10px; padding: 10px 12px; color: #bbf7d0;"
            "font-size: 13px; font-weight: 700;");
        m_status->setText(QString("🎉  Docker est là ! (%1) Vous pouvez foncer 🚀")
                              .arg(ver.isEmpty() ? "détecté" : ver));
    } else {
        m_status->setStyleSheet(
            "background: rgba(239,68,68,0.12); border: 1px solid #ef4444;"
            "border-radius: 10px; padding: 10px 12px; color: #fecaca;"
            "font-size: 13px; font-weight: 700;");
        m_status->setText(
            "😅  Pas de Docker détecté. Installez-le, lancez Docker Desktop, "
            "puis re-cliquez sur « Vérifier ».");
    }
}

void WelcomeDialog::showIfFirstLaunch(QWidget *parent)
{
    QSettings settings("GameServerManager", "App");
    if (settings.value("onboarding/welcomeSeen", false).toBool())
        return;

    WelcomeDialog dlg(parent);
    dlg.exec();
    settings.setValue("onboarding/welcomeSeen", true);
}
