#include "SideBar.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>

namespace {
const char *NAV_STYLE = R"(
    QPushButton {
        background: transparent;
        color: #a5b4fc;
        border: none;
        font-size: 12px;
        font-weight: 500;
        text-align: left;
        padding: 9px 20px;
    }
    QPushButton:hover {
        background: rgba(99, 102, 241, 0.18);
        color: #ffffff;
    }
)";
const char *SEL_STYLE = R"(
    QPushButton {
        background: rgba(99, 102, 241, 0.30);
        color: #ffffff;
        border: none;
        border-left: 3px solid #818cf8;
        font-size: 12px;
        font-weight: 700;
        text-align: left;
        padding: 9px 20px 9px 17px;
    }
)";
} // namespace

SideBar::SideBar(QWidget *parent) : QWidget(parent) {
    setFixedWidth(220);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Logo
    auto *logo = new QLabel("🎮 GameServer\nManager", this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(
        "color: #ffffff; font-size: 14px; font-weight: 700;"
        "padding: 20px 16px 14px; background: transparent;");
    root->addWidget(logo);

    auto makeSep = [this]() {
        auto *s = new QFrame(this);
        s->setFrameShape(QFrame::HLine);
        s->setFixedHeight(1);
        s->setStyleSheet("background: rgba(255,255,255,0.12); border: none;");
        return s;
    };
    root->addWidget(makeSep());

    // Scrollable game list
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical {
            background: transparent;
            width: 4px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: rgba(255,255,255,0.20);
            border-radius: 2px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0; }
    )");

    auto *scrollContent = new QWidget;
    scrollContent->setStyleSheet("background: transparent;");
    auto *nav = new QVBoxLayout(scrollContent);
    nav->setContentsMargins(0, 8, 0, 8);
    nav->setSpacing(0);

    // ── Survie / Sandbox / Construction ────────────────────────────────
    addCategory(nav, "SURVIE / SANDBOX");
    addGame(nav, "🎮  Minecraft");          // 0
    addGame(nav, "🏹  Valheim");            // 1
    addGame(nav, "🦕  ARK: Survival");      // 2
    addGame(nav, "🔧  Rust");               // 3
    addGame(nav, "🐾  Palworld");           // 4
    addGame(nav, "🌫  Enshrouded");         // 5

    // ── FPS / Tactique ──────────────────────────────────────────────────
    addCategory(nav, "FPS / TACTIQUE");
    addGame(nav, "🔫  CS2 / CS:GO");        // 6
    addGame(nav, "🎯  Team Fortress 2");    // 7
    addGame(nav, "⚔  Squad");              // 8
    addGame(nav, "🪖  Insurgency");         // 9
    addGame(nav, "🎖  Arma 3 / Reforger"); // 10
    addGame(nav, "🧟  Left 4 Dead 2");     // 11

    // ── Action / RPG ────────────────────────────────────────────────────
    addCategory(nav, "ACTION / RPG");
    addGame(nav, "⚔  Conan Exiles");       // 12
    addGame(nav, "🔨  7 Days to Die");      // 13
    addGame(nav, "🌾  DayZ");               // 14
    addGame(nav, "🌲  Sons of Forest");     // 15
    addGame(nav, "🧛  V Rising");           // 16
    addGame(nav, "⛏  Terraria");           // 17
    addGame(nav, "🚀  Starbound");          // 18

    // ── Simulation / Sandbox avancé ─────────────────────────────────────
    addCategory(nav, "SIMULATION");
    addGame(nav, "🚀  Space Engineers");    // 19
    addGame(nav, "🏭  Satisfactory");       // 20
    addGame(nav, "⚙  Factorio");           // 21
    addGame(nav, "🔭  Astroneer");          // 22
    addGame(nav, "🛸  Stationeers");        // 23

    // ── Course / Sport ──────────────────────────────────────────────────
    addCategory(nav, "COURSE / SPORT");
    addGame(nav, "🏎  Assetto Corsa");      // 24
    addGame(nav, "🚗  BeamNG.drive");       // 25

    // ── MMO / Semi-MMO ──────────────────────────────────────────────────
    addCategory(nav, "MMO");
    addGame(nav, "🧟  Project Zomboid");    // 26
    addGame(nav, "🏰  Wurm Unlimited");     // 27
    addGame(nav, "⚔  Life is Feudal");     // 28

    // ── Niche ───────────────────────────────────────────────────────────
    addCategory(nav, "NICHE");
    addGame(nav, "🔫  Garry's Mod");        // 29
    addGame(nav, "🌊  Barotrauma");         // 30
    addGame(nav, "⛏  Vintage Story");      // 31
    addGame(nav, "⚔  Foxhole");            // 32
    addGame(nav, "⚔  Mordhau");            // 33
    addGame(nav, "🎮  Pavlov VR");          // 34

    // ── Système ─────────────────────────────────────────────────────────
    addCategory(nav, "SYSTÈME");
    addGame(nav, "⚙  Docker");             // 35

    // ── Infrastructure ──────────────────────────────────────────────────
    addCategory(nav, "INFRASTRUCTURE");
    addGame(nav, "☁  Infra");              // 36

    nav->addStretch();

    scrollArea->setWidget(scrollContent);
    root->addWidget(scrollArea, 1);

    root->addWidget(makeSep());

    // Logout (outside scroll area)
    auto *btnLogout = new QPushButton("⏻  Déconnexion", this);
    btnLogout->setStyleSheet(NAV_STYLE);
    btnLogout->setCursor(Qt::PointingHandCursor);
    btnLogout->setFlat(true);
    connect(btnLogout, &QPushButton::clicked, this, &SideBar::logoutRequested);
    root->addWidget(btnLogout);
    root->addSpacing(8);
}

void SideBar::addCategory(QVBoxLayout *layout, const QString &name)
{
    auto *lbl = new QLabel(name);
    lbl->setStyleSheet(
        "font-size: 9px; font-weight: 700; color: #6366f1;"
        "padding: 10px 20px 3px; background: transparent; letter-spacing: 2px;");
    layout->addWidget(lbl);
}

void SideBar::addGame(QVBoxLayout *layout, const QString &label)
{
    int idx = m_navButtons.size();
    auto *btn = new QPushButton(label);
    btn->setStyleSheet(idx == 0 ? SEL_STYLE : NAV_STYLE);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFlat(true);
    connect(btn, &QPushButton::clicked, this, [this, idx]() {
        selectButton(idx);
        emit pageSelected(idx);
    });
    m_navButtons.push_back(btn);
    layout->addWidget(btn);
}

void SideBar::selectButton(int index)
{
    for (int i = 0; i < m_navButtons.size(); ++i)
        m_navButtons[i]->setStyleSheet(i == index ? SEL_STYLE : NAV_STYLE);
    m_currentIndex = index;
}
