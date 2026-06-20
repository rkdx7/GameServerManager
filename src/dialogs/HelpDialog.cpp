#include "HelpDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

const char *HelpDialog::kRepoUrl = "https://github.com/rkdx7/GameServerManager";

namespace {

// Petite carte « section » avec un gros emoji, un titre néon et du texte HTML.
QFrame *makeCard(const QString &emoji, const QString &title,
                 const QString &html, const QString &accent)
{
    auto *card = new QFrame;
    card->setStyleSheet(QString(R"(
        QFrame {
            background: rgba(255,255,255,0.05);
            border: 1px solid %1;
            border-radius: 14px;
        }
    )").arg(accent));

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(18, 16, 18, 16);
    lay->setSpacing(8);

    auto *head = new QLabel(QString("%1  <span style='color:%2'>%3</span>")
                                .arg(emoji, accent, title));
    head->setTextFormat(Qt::RichText);
    head->setStyleSheet("background: transparent; font-size: 16px; font-weight: 800;");
    lay->addWidget(head);

    auto *body = new QLabel(html);
    body->setTextFormat(Qt::RichText);
    body->setWordWrap(true);
    body->setOpenExternalLinks(true);
    body->setStyleSheet(
        "background: transparent; color: #e2e8f0; font-size: 13px; line-height: 150%;");
    lay->addWidget(body);

    return card;
}

} // namespace

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Centre d'aide");
    setMinimumSize(560, 640);
    setStyleSheet(R"(
        QDialog { background-color: #1e1b4b; }
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
        QScrollBar::handle:vertical {
            background: rgba(129,140,248,0.6); border-radius: 4px; min-height: 30px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── En-tête kitsch arc-en-ciel ──────────────────────────────────────
    auto *header = new QLabel(
        "📖  CENTRE D'AIDE  🎮<br>"
        "<span style='font-size:13px; font-weight:600; color:#fde68a'>"
        "✨ Tout pour dompter vos serveurs de jeu ✨</span>");
    header->setTextFormat(Qt::RichText);
    header->setAlignment(Qt::AlignCenter);
    header->setStyleSheet(R"(
        color: #ffffff;
        font-size: 24px;
        font-weight: 900;
        padding: 22px 16px;
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
            stop:0 #ec4899, stop:0.5 #8b5cf6, stop:1 #6366f1);
    )");
    root->addWidget(header);

    // ── Contenu défilant ────────────────────────────────────────────────
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto *content = new QWidget;
    content->setStyleSheet("background: transparent;");
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(18, 18, 18, 18);
    col->setSpacing(14);

    col->addWidget(makeCard("🚀", "C'est quoi ce truc ?",
        "GameServer Manager déploie et pilote vos serveurs de jeu "
        "(Minecraft, CS2, Valheim… <b>40+ jeux</b>) dans des conteneurs "
        "<b>Docker</b>, en local ou sur une machine distante / le cloud. "
        "Pas de ligne de commande à retenir : tout se fait en quelques clics.",
        "#f472b6"));

    col->addWidget(makeCard("🐳", "Étape 0 — Installer Docker",
        "Rien ne marche sans Docker, c'est le moteur qui fait tourner les serveurs.<br>"
        "👉 Téléchargez <b>Docker Desktop</b> sur "
        "<a style='color:#93c5fd' href='https://www.docker.com/products/docker-desktop/'>"
        "docker.com</a>, installez, <b>redémarrez</b>, et laissez la petite baleine "
        "🐳 tourner dans la barre des tâches. Le tuto de bienvenue vous guide pas à pas.",
        "#38bdf8"));

    col->addWidget(makeCard("🎯", "Démarrage express",
        "<b>1.</b> Choisissez un jeu dans la barre de gauche 🕹️<br>"
        "<b>2.</b> Remplissez le formulaire (nom, port, mot de passe…) 📝<br>"
        "<b>3.</b> Cliquez sur <b>Installer / Démarrer</b> 🟢<br>"
        "<b>4.</b> Suivez le tout dans l'onglet <b>Logs</b> 📜<br>"
        "Astuce : la plupart des options restent modifiables <i>après</i> "
        "l'installation.",
        "#a78bfa"));

    col->addWidget(makeCard("☁️", "Infra & Cloud",
        "Le bouton <b>☁ Infra & Cloud</b> (en bas, toujours visible) gère vos cibles "
        "de déploiement : machine locale, VM personnalisée via SSH, ou provisionnement "
        "d'une VM Scaleway. Sélectionnez ensuite la cible dans le formulaire du jeu.",
        "#34d399"));

    col->addWidget(makeCard("🆘", "Ça coince ?",
        "• Serveur qui ne démarre pas → vérifiez que Docker tourne (baleine 🐳).<br>"
        "• Personne ne se connecte → ouvrez/redirigez les ports indiqués dans la note.<br>"
        "• Besoin de plus → ouvrez une <i>issue</i> sur GitHub, on adore les retours !",
        "#fbbf24"));

    col->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    // ── Pied : GitHub + Fermer ──────────────────────────────────────────
    auto *footer = new QHBoxLayout();
    footer->setContentsMargins(18, 12, 18, 16);
    footer->setSpacing(10);

    auto *ghBtn = new QPushButton("🐙  Voir sur GitHub");
    ghBtn->setCursor(Qt::PointingHandCursor);
    ghBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.08);
            color: #ffffff;
            border: 1px solid rgba(255,255,255,0.2);
            border-radius: 10px;
            padding: 10px 18px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover { background: rgba(129,140,248,0.3); border-color: #818cf8; }
    )");
    connect(ghBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(kRepoUrl));
    });
    footer->addWidget(ghBtn);

    footer->addStretch();

    auto *closeBtn = new QPushButton("✨  Compris !");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #ec4899, stop:1 #8b5cf6);
            color: #ffffff;
            border: none;
            border-radius: 10px;
            padding: 10px 22px;
            font-size: 13px;
            font-weight: 800;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #f472b6, stop:1 #a78bfa);
        }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(closeBtn);

    root->addLayout(footer);
}
