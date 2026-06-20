#include "MainWindow.h"
#include "SideBar.h"
#include "MinecraftPage.h"
#include "CS2Page.h"
#include "DockerManager.h"
#include "GenericGamePage.h"
#include "InfraPage.h"
#include "VMStorage.h"
#include "DeploymentTargetSelector.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QFrame>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QSettings>

// ── Game config helpers ───────────────────────────────────────────────────────

static GameFieldConfig tf(const QString &label, const QString &key,
                           const QString &def, const QString &envVar,
                           const QString &ph = {})
{
    GameFieldConfig f;
    f.label = label; f.key = key; f.type = GameFieldConfig::Text;
    f.defaultValue = def; f.envVar = envVar; f.placeholder = ph;
    return f;
}

static GameFieldConfig pf(const QString &label, const QString &key,
                           const QString &def, const QString &envVar,
                           const QString &ph = {})
{
    GameFieldConfig f;
    f.label = label; f.key = key; f.type = GameFieldConfig::Password;
    f.defaultValue = def; f.envVar = envVar; f.placeholder = ph;
    return f;
}

static GameFieldConfig cf(const QString &label, const QString &key,
                           const QStringList &opts, const QString &def,
                           const QString &envVar)
{
    GameFieldConfig f;
    f.label = label; f.key = key; f.type = GameFieldConfig::Combo;
    f.comboOptions = opts; f.defaultValue = def; f.envVar = envVar;
    return f;
}

static GameFieldConfig sf(const QString &label, const QString &key,
                           int val, int mn, int mx, const QString &envVar)
{
    GameFieldConfig f;
    f.label = label; f.key = key; f.type = GameFieldConfig::Spin;
    f.defaultValue = QString::number(val);
    f.spinMin = mn; f.spinMax = mx; f.envVar = envVar;
    return f;
}

// ── Page constructors ─────────────────────────────────────────────────────────

static GenericGamePage *makeValheim(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🏹"; c.title = "Valheim Server";
    c.description = "Survie coopérative viking — jusqu'à 10 joueurs (lloesche/valheim-server).";
    c.note = "ℹ Ports 2456–2458/UDP requis. Laissez le mot de passe vide pour un accès public sans code.";
    c.defaultContainerName = "valheim-server";
    c.dockerImage = "lloesche/valheim-server";
    c.dataVolumePath = "/opt/valheim";
    c.defaultPort = 2456;
    c.ports = { {2456,0,true,false},{2457,2457,true,false},{2458,2458,true,false} };
    c.btnColorStart = "#10b981"; c.btnColorEnd = "#0ea5e9";
    c.configDocUrl = "https://github.com/lloesche/valheim-server-docker";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",   "Mon Serveur Valheim", "SERVER_NAME"),
        tf("Nom du monde",    "WORLD_NAME",    "MyWorld",             "WORLD_NAME"),
        pf("Mot de passe",    "SERVER_PASS",   "",                    "SERVER_PASS",   "Laisser vide = sans mot de passe"),
        cf("Serveur public",  "SERVER_PUBLIC", {"0 — Privé","1 — Public"}, "1", "SERVER_PUBLIC"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeARK(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🦕"; c.title = "ARK: Survival Evolved";
    c.description = "Survie avec dinosaures — serveur dédié officiel (hermsi/ark-server).";
    c.note = "ℹ L'image est volumineuse (~20 GB). Ports 7777/UDP, 7778/UDP, 27015/UDP requis.";
    c.defaultContainerName = "ark-server";
    c.dockerImage = "hermsi/ark-server";
    c.dataVolumePath = "/home/steam/ark/ShooterGame/Saved";
    c.defaultPort = 7777;
    c.ports = { {7777,0,true,false},{7778,7778,true,false},{27015,27015,true,false} };
    c.btnColorStart = "#f97316"; c.btnColorEnd = "#ef4444";
    c.configDocUrl = "https://ark.wiki.gg/wiki/Server_configuration";
    c.fields = {
        tf("Nom de session",     "SESSIONNAME",      "Mon Serveur ARK",  "SESSIONNAME"),
        cf("Carte",              "SERVERMAP",
           {"TheIsland","TheCenter","Ragnarok","Aberration","Extinction","Valguero","Genesis"},
           "TheIsland", "SERVERMAP"),
        pf("Mot de passe",       "SERVERPASSWORD",   "",  "SERVERPASSWORD",   "Optionnel"),
        pf("Mot de passe admin", "ADMINPASSWORD",    "adminpass123", "ADMINPASSWORD"),
        sf("Joueurs max",        "MAXPLAYERS",       20, 1, 200, "MAXPLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeRust(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🔧"; c.title = "Rust Server";
    c.description = "Survie PvP hardcore — communautés très actives (didstopia/rust-server).";
    c.note = "ℹ Ports 28015/UDP, 28016/TCP requis. La seed génère la carte procédurale.";
    c.defaultContainerName = "rust-server";
    c.dockerImage = "didstopia/rust-server";
    c.dataVolumePath = "/steamcmd/rust";
    c.defaultPort = 28015;
    c.ports = { {28015,0,true,false},{28016,28016,false,true} };
    c.btnColorStart = "#b45309"; c.btnColorEnd = "#ef4444";
    c.configDocUrl = "https://wiki.facepunch.com/rust/Creating-a-server";
    c.fields = {
        tf("Nom du serveur",  "RUST_SERVER_NAME",        "Mon Serveur Rust",   "RUST_SERVER_NAME"),
        tf("Description",     "RUST_SERVER_DESCRIPTION", "",                   "RUST_SERVER_DESCRIPTION", "Optionnel"),
        cf("Carte",           "RUST_SERVER_MAP",
           {"Procedural Map","Barren","HapisIsland","SavasIsland"},
           "Procedural Map", "RUST_SERVER_MAP"),
        tf("Seed de carte",   "RUST_SERVER_SEED",        "42",                 "RUST_SERVER_SEED"),
        sf("Joueurs max",     "RUST_SERVER_MAXPLAYERS",  100, 1, 500,          "RUST_SERVER_MAXPLAYERS"),
        sf("Taille du monde", "RUST_SERVER_WORLDSIZE",   3000, 1000, 6000,     "RUST_SERVER_WORLDSIZE"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makePalworld(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🐾"; c.title = "Palworld Server";
    c.description = "Survie + craft + créatures — jusqu'à 32 joueurs (thijsvanloef/palworld-server-docker).";
    c.note = "ℹ Ports 8211/UDP et 27015/UDP requis.";
    c.defaultContainerName = "palworld-server";
    c.dockerImage = "thijsvanloef/palworld-server-docker";
    c.dataVolumePath = "/palworld";
    c.defaultPort = 8211;
    c.ports = { {8211,0,true,false},{27015,27015,true,false} };
    c.btnColorStart = "#8b5cf6"; c.btnColorEnd = "#ec4899";
    c.configDocUrl  = "https://tech.palworldgame.com/optimize-game-balance";
    c.configFilePath = "/palworld/Pal/Saved/Config/LinuxServer/PalWorldSettings.ini";
    c.fields = {
        tf("Nom du serveur",     "SERVERNAME",     "Mon Serveur Palworld", "SERVERNAME"),
        pf("Mot de passe",       "SERVER_PASSWORD","",                     "SERVER_PASSWORD",  "Optionnel"),
        pf("Mot de passe admin", "ADMIN_PASSWORD", "adminpass123",         "ADMIN_PASSWORD"),
        sf("Joueurs max",        "PLAYERS",        32, 1, 32,              "PLAYERS"),
        tf("Description",        "SERVER_DESC",    "",                     "SERVER_DESCRIPTION","Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeEnshrouded(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🌫"; c.title = "Enshrouded Server";
    c.description = "Survie/exploration coopérative — jusqu'à 16 joueurs (sknnr/enshrouded-server).";
    c.note = "ℹ Ports 15636/UDP et 15637/UDP requis.";
    c.defaultContainerName = "enshrouded-server";
    c.dockerImage = "sknnr/enshrouded-server";
    c.dataVolumePath = "/opt/enshrouded";
    c.defaultPort = 15636;
    c.ports = { {15636,0,true,false},{15637,15637,true,false} };
    c.btnColorStart = "#6366f1"; c.btnColorEnd = "#a855f7";
    c.configDocUrl = "https://enshrouded.zendesk.com/hc/en-us/articles/18886538566417";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur Enshrouded", "SERVER_NAME"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                       "SERVER_PASSWORD", "Optionnel"),
        sf("Joueurs max",     "SERVER_SLOTS",    16, 1, 16,                "SERVER_SLOTS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeCS16(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "💣"; c.title = "Counter-Strike 1.6";
    c.description = "FPS tactique GoldSrc culte — serveur HLDS classique (hlds/server).";
    c.note = "ℹ Aucun token GSLT requis (moteur GoldSrc). Port 27015/UDP+TCP requis. "
             "La configuration fine se fait via server.cfg dans les paramètres avancés.";
    c.defaultContainerName = "cs16-server";
    c.dockerImage = "hlds/server";
    c.dataVolumePath = "/opt/hlds";
    c.defaultPort = 27015;
    c.ports = { {27015,0,true,true} };
    c.btnColorStart = "#b45309"; c.btnColorEnd = "#f59e0b";
    c.configDocUrl   = "https://developer.valvesoftware.com/wiki/Counter-Strike_Dedicated_Server";
    c.configFilePath = "/opt/hlds/cstrike/server.cfg";
    c.fields = {
        tf("Nom du serveur",      "HOSTNAME",      "Mon Serveur CS 1.6", "HOSTNAME"),
        cf("Carte de départ",     "MAP",
           {"de_dust2","de_dust","de_inferno","de_nuke","de_train","cs_assault","cs_office"},
           "de_dust2", "MAP"),
        sf("Joueurs max",         "MAXPLAYERS",    16, 2, 32, "MAXPLAYERS"),
        pf("Mot de passe RCON",   "RCON_PASSWORD", "rcon123", "RCON_PASSWORD"),
        pf("Mot de passe serveur","SV_PASSWORD",   "",        "SV_PASSWORD", "Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeTF2(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🎯"; c.title = "Team Fortress 2 Server";
    c.description = "FPS multijoueur emblématique — très moddable (cm2network/tf2).";
    c.note = "ℹ Token GSLT requis pour serveur public (store.steampowered.com/account/gameserveraccount).";
    c.defaultContainerName = "tf2-server";
    c.dockerImage = "cm2network/tf2";
    c.dataVolumePath = "/home/steam/tf-dedicated";
    c.defaultPort = 27015;
    c.ports = { {27015,0,true,true},{27020,27020,true,false} };
    c.btnColorStart = "#f59e0b"; c.btnColorEnd = "#ef4444";
    c.configDocUrl   = "https://developer.valvesoftware.com/wiki/Team_Fortress_2/Dedicated_Servers";
    c.configFilePath = "/home/steam/tf-dedicated/tf/cfg/server.cfg";
    c.fields = {
        tf("Token GSLT",       "SRCDS_TOKEN",     "",           "SRCDS_TOKEN",     "Requis pour serveur public"),
        pf("Mot de passe RCON","SRCDS_RCONPW",    "rcon123",    "SRCDS_RCONPW"),
        pf("Mot de passe jeu", "SRCDS_PW",        "",           "SRCDS_PW",        "Optionnel"),
        cf("Map de départ",    "SRCDS_STARTMAP",
           {"cp_badlands","pl_badwater","ctf_2fort","cp_dustbowl","koth_nucleus"},
           "cp_badlands", "SRCDS_STARTMAP"),
        sf("Joueurs max",      "SRCDS_MAXPLAYERS",24, 2, 32,   "SRCDS_MAXPLAYERS"),
        cf("Mode de jeu",      "SRCDS_GAMEMODE",
           {"0 — Normal","1 — Arena","2 — Payload"},
           "0 — Normal","SRCDS_GAMEMODE"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeSquad(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚔"; c.title = "Squad Server";
    c.description = "FPS militaire tactique — serveurs communautaires (cm2network/squad).";
    c.note = "ℹ Ports 7787/UDP, 27165/UDP, 21114/TCP requis.";
    c.defaultContainerName = "squad-server";
    c.dockerImage = "cm2network/squad";
    c.dataVolumePath = "/home/steam/squad-dedicated";
    c.defaultPort = 7787;
    c.ports = { {7787,0,true,false},{27165,27165,true,false},{21114,21114,false,true} };
    c.btnColorStart = "#475569"; c.btnColorEnd = "#334155";
    c.configDocUrl = "https://squad.wiki/Dedicated_Server_Guide";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur Squad", "SERVER_NAME"),
        sf("Joueurs max",        "MAX_PLAYERS",    80, 10, 100,         "MAX_PLAYERS"),
        pf("Mot de passe",       "SERVER_PASSWORD","",                  "SERVER_PASSWORD", "Optionnel"),
        pf("Mot de passe admin", "ADMIN_PASSWORD", "adminpass123",      "ADMIN_PASSWORD"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeInsurgency(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🪖"; c.title = "Insurgency: Sandstorm";
    c.description = "FPS tactique moddable — support Steam Workshop (cm2network/insurgency-sandstorm).";
    c.note = "ℹ Port 27102/UDP+TCP requis.";
    c.defaultContainerName = "insurgency-server";
    c.dockerImage = "cm2network/insurgency-sandstorm";
    c.dataVolumePath = "/home/steam/insurgency-dedicated";
    c.defaultPort = 27102;
    c.ports = { {27102,0,true,true} };
    c.btnColorStart = "#78350f"; c.btnColorEnd = "#b45309";
    c.configDocUrl = "https://sandstorm.featureupvote.com/suggestions/17616/dedicated-server-guide";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur Insurgency", "SERVER_NAME"),
        cf("Scénario",           "GAME_SCENARIO",
           {"Checkpoint_Security","Skirmish_Security","Push_Security","Firefight_East"},
           "Checkpoint_Security","GAME_SCENARIO"),
        sf("Joueurs max",        "MAX_PLAYERS",    20, 2, 28,              "MAX_PLAYERS"),
        pf("Mot de passe",       "SERVER_PASSWORD","",                     "SERVER_PASSWORD","Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeArma(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🎖"; c.title = "Arma 3 / Arma Reforger";
    c.description = "Simulation militaire — mods via Steam Workshop (muttley71/arma3server).";
    c.note = "ℹ Port 2302/UDP requis. Pour Arma Reforger, changez l'image en ghcr.io/maca134/arma-reforger-docker.";
    c.defaultContainerName = "arma-server";
    c.dockerImage = "muttley71/arma3server";
    c.dataVolumePath = "/arma3";
    c.defaultPort = 2302;
    c.ports = { {2302,0,true,false},{2303,2303,true,false},{2304,2304,true,false},{2305,2305,true,false} };
    c.btnColorStart = "#166534"; c.btnColorEnd = "#15803d";
    c.configDocUrl = "https://community.bistudio.com/wiki/Arma_3:_Dedicated_Server";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur Arma",  "SERVER_NAME"),
        pf("Mot de passe",       "SERVER_PASSWORD","",                  "SERVER_PASSWORD","Optionnel"),
        pf("Mot de passe admin", "ADMIN_PASSWORD", "adminpass123",      "ADMIN_PASSWORD"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeL4D2(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🧟"; c.title = "Left 4 Dead 2 Server";
    c.description = "Coopératif zombie — serveurs dédiés officiels (cm2network/l4d2).";
    c.note = "ℹ Token GSLT requis pour serveur public. Port 27015/UDP+TCP requis.";
    c.defaultContainerName = "l4d2-server";
    c.dockerImage = "cm2network/l4d2";
    c.dataVolumePath = "/home/steam/l4d2-dedicated";
    c.defaultPort = 27015;
    c.ports = { {27015,0,true,true} };
    c.btnColorStart = "#7f1d1d"; c.btnColorEnd = "#ef4444";
    c.configDocUrl   = "https://developer.valvesoftware.com/wiki/Left_4_Dead_2_Dedicated_Servers";
    c.configFilePath = "/home/steam/l4d2-dedicated/left4dead2/cfg/server.cfg";
    c.fields = {
        tf("Token GSLT",       "SRCDS_TOKEN",    "",         "SRCDS_TOKEN",    "Requis pour serveur public"),
        pf("Mot de passe RCON","SRCDS_RCONPW",   "rcon123",  "SRCDS_RCONPW"),
        pf("Mot de passe jeu", "SRCDS_PW",       "",         "SRCDS_PW",       "Optionnel"),
        cf("Map de départ",    "SRCDS_STARTMAP",
           {"c1m1_hotel","c2m1_highway","c3m1_plankcountry","c4m1_milltown_a","c5m1_waterfront"},
           "c1m1_hotel","SRCDS_STARTMAP"),
        sf("Joueurs max",      "SRCDS_MAXPLAYERS",8, 2, 8,  "SRCDS_MAXPLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeConan(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚔"; c.title = "Conan Exiles Server";
    c.description = "Survie/RPG dans l'univers Conan — très moddable (alinmear/conan-exiles).";
    c.note = "ℹ Ports 7777/UDP+TCP, 7778/UDP, 27015/UDP requis.";
    c.defaultContainerName = "conan-server";
    c.dockerImage = "alinmear/conan-exiles";
    c.dataVolumePath = "/conan-exiles-dedicated";
    c.defaultPort = 7777;
    c.ports = { {7777,0,true,true},{7778,7778,true,false},{27015,27015,true,false} };
    c.btnColorStart = "#92400e"; c.btnColorEnd = "#b45309";
    c.configDocUrl = "https://conanexiles.fandom.com/wiki/Server_Setup";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur Conan", "SERVER_NAME"),
        pf("Mot de passe",       "SERVERPASSWORD", "",                  "SERVERPASSWORD", "Optionnel"),
        pf("Mot de passe admin", "ADMINPASSWORD",  "adminpass123",      "ADMINPASSWORD"),
        sf("Joueurs max",        "MAX_PLAYERS",    40, 1, 80,           "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *make7DTD(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🔨"; c.title = "7 Days to Die Server";
    c.description = "Survie zombie avec construction et crafting (didstopia/7dtd-server).";
    c.note = "ℹ Ports 26900/UDP+TCP, 26901/UDP, 26902/UDP requis.";
    c.defaultContainerName = "7dtd-server";
    c.dockerImage = "didstopia/7dtd-server";
    c.dataVolumePath = "/steamcmd/7dtd";
    c.defaultPort = 26900;
    c.ports = { {26900,0,true,true},{26901,26901,true,false},{26902,26902,true,false} };
    c.btnColorStart = "#78350f"; c.btnColorEnd = "#dc2626";
    c.configDocUrl = "https://7daystodie.fandom.com/wiki/Server";
    c.fields = {
        tf("Nom du serveur",  "SDTD_SERVER_NAME",     "Mon Serveur 7DTD","SDTD_SERVER_NAME"),
        pf("Mot de passe",    "SDTD_SERVER_PASSWORD", "",                "SDTD_SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",     "SDTD_MAX_PLAYERS",     8, 1, 30,          "SDTD_MAX_PLAYERS"),
        cf("Difficulté",      "SDTD_GAME_DIFFICULTY",
           {"0 — Survivant","1 — Fouilleur","2 — Guerrier","3 — Nomade","4 — Insane","5 — Nightmare"},
           "2 — Guerrier","SDTD_GAME_DIFFICULTY"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeDayZ(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🌾"; c.title = "DayZ Server";
    c.description = "Survie zombie open-world (immodal/dayz-server).";
    c.note = "ℹ Port 2302/UDP requis. Licence Steam requise pour télécharger les fichiers serveur.";
    c.defaultContainerName = "dayz-server";
    c.dockerImage = "immodal/dayz-server";
    c.dataVolumePath = "/dayz";
    c.defaultPort = 2302;
    c.ports = { {2302,0,true,false},{2305,2305,true,false} };
    c.btnColorStart = "#4b5563"; c.btnColorEnd = "#1f2937";
    c.configDocUrl = "https://community.bistudio.com/wiki/DayZ:Server_Configuration";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur DayZ", "SERVER_NAME"),
        pf("Mot de passe",       "SERVER_PASSWORD","",                 "SERVER_PASSWORD","Optionnel"),
        pf("Mot de passe admin", "ADMIN_PASSWORD", "adminpass123",     "ADMIN_PASSWORD"),
        sf("Joueurs max",        "MAX_PLAYERS",    60, 1, 100,         "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeSotF(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🌲"; c.title = "Sons of the Forest";
    c.description = "Horreur/survie coopérative — jusqu'à 8 joueurs (jammsen/sons-of-the-forest-dedicated-server).";
    c.note = "ℹ Ports 8766/UDP, 27016/UDP, 9700/UDP requis.";
    c.defaultContainerName = "sotf-server";
    c.dockerImage = "jammsen/sons-of-the-forest-dedicated-server";
    c.dataVolumePath = "/sonsoftheforest";
    c.defaultPort = 8766;
    c.ports = { {8766,0,true,false},{27016,27016,true,false},{9700,9700,true,false} };
    c.btnColorStart = "#166534"; c.btnColorEnd = "#15803d";
    c.configDocUrl = "https://github.com/jammsen/docker-sons-of-the-forest-dedicated-server";
    c.fields = {
        tf("Nom du serveur",  "SOTF_SERVER_NAME",     "Mon Serveur SotF", "SOTF_SERVER_NAME"),
        pf("Mot de passe",    "SOTF_SERVER_PASSWORD", "",                 "SOTF_SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",     "SOTF_MAX_PLAYERS",     8, 1, 8,            "SOTF_MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeVRising(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🧛"; c.title = "V Rising Server";
    c.description = "RPG vampire coopératif/PvP (trueosiris/vrising).";
    c.note = "ℹ Ports 9876/UDP et 9877/UDP requis.";
    c.defaultContainerName = "vrising-server";
    c.dockerImage = "trueosiris/vrising";
    c.dataVolumePath = "/mnt/vrising/server";
    c.defaultPort = 9876;
    c.ports = { {9876,0,true,false},{9877,9877,true,false} };
    c.btnColorStart = "#7f1d1d"; c.btnColorEnd = "#9d174d";
    c.configDocUrl = "https://github.com/StunlockStudios/vrising-dedicated-server-instructions";
    c.fields = {
        tf("Nom du serveur",  "NAME",     "Mon Serveur V Rising", "NAME"),
        pf("Mot de passe",    "PASSWORD", "",                     "PASSWORD","Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeTerraria(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⛏"; c.title = "Terraria (TShock)";
    c.description = "Sandbox 2D — serveur communautaire via TShock (ryshe/tshock).";
    c.note = "ℹ Port 7777/TCP requis.";
    c.defaultContainerName = "terraria-server";
    c.dockerImage = "ryshe/tshock";
    c.dataVolumePath = "/tshock";
    c.defaultPort = 7777;
    c.ports = { {7777,0,false,true} };
    c.btnColorStart = "#0369a1"; c.btnColorEnd = "#7c3aed";
    c.configDocUrl = "https://ikebukuro.tshock.org/";
    c.fields = {
        tf("Nom du monde",   "worldname",  "MyWorld",  "worldname"),
        pf("Mot de passe",   "password",   "",         "password",   "Optionnel"),
        sf("Joueurs max",    "maxplayers", 8, 1, 255,  "maxplayers"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeStarbound(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🚀"; c.title = "Starbound Server";
    c.description = "Exploration spatiale 2D — multijoueur coopératif (didstopia/starbound-server).";
    c.note = "ℹ Port 21025/TCP requis. Licence Steam requise.";
    c.defaultContainerName = "starbound-server";
    c.dockerImage = "didstopia/starbound-server";
    c.dataVolumePath = "/steamcmd/starbound";
    c.defaultPort = 21025;
    c.ports = { {21025,0,false,true} };
    c.btnColorStart = "#1d4ed8"; c.btnColorEnd = "#7c3aed";
    c.configDocUrl = "https://starbounder.org/Multiplayer";
    c.fields = {
        pf("Mot de passe", "SERVER_PASSWORD", "", "SERVER_PASSWORD", "Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeSpaceEngineers(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🚀"; c.title = "Space Engineers Server";
    c.description = "Sandbox spatial/physique — ingénierie et survie (mmmaxwwwell/space-engineers-dedicated-docker).";
    c.note = "ℹ Fonctionne via Proton/Wine. Port 27016/UDP requis. Licence Steam requise.";
    c.defaultContainerName = "spaceeng-server";
    c.dockerImage = "mmmaxwwwell/space-engineers-dedicated-docker";
    c.dataVolumePath = "/appdata/SpaceEngineers";
    c.defaultPort = 27016;
    c.ports = { {27016,0,true,false} };
    c.btnColorStart = "#1e3a5f"; c.btnColorEnd = "#1d4ed8";
    c.configDocUrl = "https://www.spaceengineersgame.com/dedicated-servers.html";
    c.fields = {
        tf("Nom du serveur",  "SE_SERVER_NAME", "Mon Serveur SE",  "SE_SERVER_NAME"),
        tf("Nom du monde",    "SE_WORLD_NAME",  "MyWorld",         "SE_WORLD_NAME"),
        sf("Joueurs max",     "SE_MAX_PLAYERS", 16, 1, 32,         "SE_MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeSatisfactory(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🏭"; c.title = "Satisfactory Server";
    c.description = "Construction d'usines coopérative — supporte serveur dédié officiel (wolveix/satisfactory-server).";
    c.note = "ℹ Ports 7777/UDP, 15000/UDP, 15777/UDP requis.";
    c.defaultContainerName = "satisfactory-server";
    c.dockerImage = "wolveix/satisfactory-server";
    c.dataVolumePath = "/home/steam/.config/Epic/FactoryGame/Saved";
    c.defaultPort = 7777;
    c.ports = { {7777,0,true,false},{15000,15000,true,false},{15777,15777,true,false} };
    c.btnColorStart = "#f97316"; c.btnColorEnd = "#f59e0b";
    c.configDocUrl = "https://satisfactory.wiki.gg/wiki/Dedicated_servers";
    c.fields = {
        sf("Joueurs max",          "MAXPLAYERS",        4, 1, 4,     "MAXPLAYERS"),
        cf("Pause auto",           "AUTOPAUSE",         {"true","false"}, "true", "AUTOPAUSE"),
        sf("Intervalle autosave (s)","AUTOSAVEINTERVAL",300,60,3600,  "AUTOSAVEINTERVAL"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeFactorio(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚙"; c.title = "Factorio Server";
    c.description = "Automation et construction coopérative — serveur dédié intégré natif (factoriotools/factorio).";
    c.note = "ℹ Port 34197/UDP requis. Token optionnel pour synchronisation avec factorio.com.";
    c.defaultContainerName = "factorio-server";
    c.dockerImage = "factoriotools/factorio";
    c.dataVolumePath = "/factorio";
    c.defaultPort = 34197;
    c.ports = { {34197,0,true,false} };
    c.btnColorStart = "#a16207"; c.btnColorEnd = "#f59e0b";
    c.configDocUrl   = "https://wiki.factorio.com/Multiplayer";
    c.configFilePath = "/factorio/config/server-settings.json";
    c.fields = {
        tf("Nom de la partie",    "SAVENAME",       "ma-partie",   "SAVENAME"),
        pf("Mot de passe",        "PASSWORD",       "",            "PASSWORD",       "Optionnel"),
        pf("Mot de passe admin",  "ADMIN_PASSWORD", "",            "ADMIN_PASSWORD", "Optionnel"),
        tf("Token factorio.com",  "TOKEN",          "",            "TOKEN",          "Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeAstroneer(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🔭"; c.title = "Astroneer Server";
    c.description = "Exploration planétaire coopérative (jammsen/astroneer-dedicated-server).";
    c.note = "ℹ Port 8777/UDP requis.";
    c.defaultContainerName = "astroneer-server";
    c.dockerImage = "jammsen/astroneer-dedicated-server";
    c.dataVolumePath = "/astroneer";
    c.defaultPort = 8777;
    c.ports = { {8777,0,true,false} };
    c.btnColorStart = "#0369a1"; c.btnColorEnd = "#7c3aed";
    c.configDocUrl = "https://astroneer.fandom.com/wiki/Dedicated_Servers";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME", "Mon Serveur Astroneer", "SERVER_NAME"),
        sf("Joueurs max",     "MAX_PLAYERS", 8, 1, 8,                 "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeStationeers(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🛸"; c.title = "Stationeers Server";
    c.description = "Simulation de station spatiale réaliste (rocketwerkz/stationeers-dedicated).";
    c.note = "ℹ Ports 27500/UDP et 27015/TCP requis.";
    c.defaultContainerName = "stationeers-server";
    c.dockerImage = "rocketwerkz/stationeers-dedicated";
    c.dataVolumePath = "/stationeers";
    c.defaultPort = 27500;
    c.ports = { {27500,0,true,false},{27015,27015,false,true} };
    c.btnColorStart = "#1d4ed8"; c.btnColorEnd = "#4f46e5";
    c.configDocUrl = "https://stationeers-wiki.com/Hosting_a_Stationeers_server";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur Stationeers", "SERVER_NAME"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                        "SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",     "MAX_PLAYERS",     10, 1, 30,                 "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeAssettoCorsa(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🏎"; c.title = "Assetto Corsa / ACC";
    c.description = "Simulation automobile — très utilisé en ligue (klauxius/assettocorsa-server).";
    c.note = "ℹ Pour ACC, remplacez l'image par une image ACC compatible. Ports 9600/TCP+UDP requis.";
    c.defaultContainerName = "ac-server";
    c.dockerImage = "klauxius/assettocorsa-server";
    c.dataVolumePath = "/assettocorsa";
    c.defaultPort = 9600;
    c.ports = { {9600,0,true,true},{8081,8081,false,true} };
    c.btnColorStart = "#7c3aed"; c.btnColorEnd = "#ec4899";
    c.configDocUrl = "https://www.assettocorsa.net/assetto-corsa-dedicated-server-guide/";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",  "Mon Serveur AC", "SERVER_NAME"),
        sf("Joueurs max",     "MAX_CLIENTS",  16, 1, 32,        "MAX_CLIENTS"),
        pf("Mot de passe",    "PASSWORD",     "",               "PASSWORD",   "Optionnel"),
        pf("Mot de passe admin","ADMIN_PASSWORD","adminpass123","ADMIN_PASSWORD"),
        cf("Type de serveur", "SERVER_TYPE",
           {"Assetto Corsa","Assetto Corsa Competizione"},
           "Assetto Corsa","SERVER_TYPE"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeBeamNG(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🚗"; c.title = "BeamNG.drive Server";
    c.description = "Simulation physique automobile multijoueur (BeamMP — lbvehicles/beammp-server).";
    c.note = "ℹ Clé d'authentification BeamMP requise sur beammp.com. Port 48900/TCP requis.";
    c.defaultContainerName = "beamng-server";
    c.dockerImage = "lbvehicles/beammp-server";
    c.dataVolumePath = "/beammp";
    c.defaultPort = 48900;
    c.ports = { {48900,0,false,true} };
    c.btnColorStart = "#0369a1"; c.btnColorEnd = "#0ea5e9";
    c.configDocUrl = "https://wiki.beammp.com/en/home/server-installation";
    c.fields = {
        tf("Nom du serveur",   "SERVER_NAME",    "Mon Serveur BeamNG","SERVER_NAME"),
        tf("Clé BeamMP",       "AUTH_KEY",       "",                  "AUTH_KEY",     "Clé depuis beammp.com"),
        pf("Mot de passe",     "SERVER_PASSWORD","",                  "SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",      "MAX_PLAYERS",    8, 1, 64,            "MAX_PLAYERS"),
        tf("Description",      "SERVER_DESC",    "",                  "SERVER_DESC",  "Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeProjectZomboid(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🧟"; c.title = "Project Zomboid Server";
    c.description = "Survie zombie isométrique — très bon support serveur (alinmear/project-zomboid).";
    c.note = "ℹ Ports 16261/UDP et 16262/UDP requis.";
    c.defaultContainerName = "zomboid-server";
    c.dockerImage = "alinmear/project-zomboid";
    c.dataVolumePath = "/pzserver";
    c.defaultPort = 16261;
    c.ports = { {16261,0,true,false},{16262,16262,true,false} };
    c.btnColorStart = "#4d7c0f"; c.btnColorEnd = "#166534";
    c.configDocUrl = "https://pzwiki.net/wiki/Dedicated_server";
    c.fields = {
        tf("Nom du serveur",    "SERVER_NAME",     "Mon Serveur PZ",  "SERVER_NAME"),
        tf("Nom admin",         "ADMIN_USERNAME",  "admin",           "ADMIN_USERNAME"),
        pf("Mot de passe admin","ADMIN_PASSWORD",  "adminpass123",    "ADMIN_PASSWORD"),
        pf("Mot de passe jeu",  "SERVER_PASSWORD", "",                "SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",       "MAX_PLAYERS",     32, 1, 64,         "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeWurm(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🏰"; c.title = "Wurm Unlimited Server";
    c.description = "MMO médiéval sandbox hébergeable intégralement (ich777/wurmunlimited).";
    c.note = "ℹ Ports 3724/TCP et 7221/TCP requis. Licence Steam requise.";
    c.defaultContainerName = "wurm-server";
    c.dockerImage = "ich777/wurmunlimited";
    c.dataVolumePath = "/serverdata";
    c.defaultPort = 3724;
    c.ports = { {3724,0,false,true},{7221,7221,false,true} };
    c.btnColorStart = "#713f12"; c.btnColorEnd = "#92400e";
    c.configDocUrl = "https://www.wurmonline.com/wiki/index.php?title=Wurm_Unlimited";
    c.fields = {
        tf("Nom du serveur",     "SERVER_NAME",    "Mon Serveur Wurm","SERVER_NAME"),
        pf("Mot de passe admin", "ADMIN_PASSWORD", "adminpass123",    "ADMIN_PASSWORD"),
        sf("Joueurs max",        "MAX_PLAYERS",    50, 1, 200,        "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeLifeIsFeudal(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚔"; c.title = "Life is Feudal: Your Own";
    c.description = "Féodalisme sandbox — gestion d'un village médiéval (ich777/lifeisfeudal).";
    c.note = "ℹ Ports 4305/UDP et 4400/TCP requis. Licence Steam requise.";
    c.defaultContainerName = "lif-server";
    c.dockerImage = "ich777/lifeisfeudal";
    c.dataVolumePath = "/serverdata";
    c.defaultPort = 4305;
    c.ports = { {4305,0,true,false},{4400,4400,false,true} };
    c.btnColorStart = "#713f12"; c.btnColorEnd = "#a16207";
    c.configDocUrl = "https://lifeisfeudal.com/wiki/Server_configuration";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur LiF", "SERVER_NAME"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                "SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",     "MAX_PLAYERS",     64, 1, 64,         "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeGMod(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🔫"; c.title = "Garry's Mod Server";
    c.description = "Bac à sable Source — des milliers de mods (cm2network/garrysmod).";
    c.note = "ℹ Token GSLT requis pour serveur public. Port 27015/UDP+TCP requis.";
    c.defaultContainerName = "gmod-server";
    c.dockerImage = "cm2network/garrysmod";
    c.dataVolumePath = "/home/steam/gmod-dedicated";
    c.defaultPort = 27015;
    c.ports = { {27015,0,true,true} };
    c.btnColorStart = "#4338ca"; c.btnColorEnd = "#7c3aed";
    c.configDocUrl   = "https://wiki.facepunch.com/gmod/Hosting_a_Server";
    c.configFilePath = "/home/steam/gmod-dedicated/garrysmod/cfg/server.cfg";
    c.fields = {
        tf("Token GSLT",       "SRCDS_TOKEN",    "",           "SRCDS_TOKEN",    "Requis pour serveur public"),
        pf("Mot de passe RCON","SRCDS_RCONPW",   "rcon123",    "SRCDS_RCONPW"),
        pf("Mot de passe jeu", "SRCDS_PW",       "",           "SRCDS_PW",       "Optionnel"),
        cf("Mode de jeu",      "SRCDS_GAMEMODE",
           {"sandbox","darkrp","terrortown","prophunt","deathrun"},
           "sandbox","SRCDS_GAMEMODE"),
        cf("Map de départ",    "SRCDS_MAP",
           {"gm_flatgrass","gm_construct","rp_downtown_v2","ttt_mc_skyislands"},
           "gm_flatgrass","SRCDS_MAP"),
        sf("Joueurs max",      "SRCDS_MAXPLAYERS",16, 2, 100, "SRCDS_MAXPLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeBarotrauma(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🌊"; c.title = "Barotrauma Server";
    c.description = "Sous-marin coopératif/horreur — survie en équipe (barotraumaofficial/barotrauma).";
    c.note = "ℹ Port 27015/UDP requis.";
    c.defaultContainerName = "barotrauma-server";
    c.dockerImage = "barotraumaofficial/barotrauma";
    c.dataVolumePath = "/barotrauma";
    c.defaultPort = 27015;
    c.ports = { {27015,0,true,false} };
    c.btnColorStart = "#0c4a6e"; c.btnColorEnd = "#0369a1";
    c.configDocUrl = "https://barotraumagame.com/wiki/Hosting_a_server";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur Baro","SERVER_NAME"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                "SERVER_PASSWORD","Optionnel"),
        sf("Joueurs max",     "MAX_PLAYERS",     16, 1, 16,         "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeVintageStory(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⛏"; c.title = "Vintage Story Server";
    c.description = "Survie/craft hardcore — alternative Minecraft (devidian/vintage-story-server).";
    c.note = "ℹ Port 42420/TCP requis. Licence requise sur vintagestory.at.";
    c.defaultContainerName = "vs-server";
    c.dockerImage = "devidian/vintage-story-server";
    c.dataVolumePath = "/data";
    c.defaultPort = 42420;
    c.ports = { {42420,0,false,true} };
    c.btnColorStart = "#92400e"; c.btnColorEnd = "#78350f";
    c.configDocUrl = "https://wiki.vintagestory.at/index.php/Setting_up_a_Multiplayer_Server";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME", "Mon Serveur VS", "SERVER_NAME"),
        sf("Joueurs max",     "MAX_PLAYERS", 16, 1, 64,        "MAX_PLAYERS"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeFoxhole(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚔"; c.title = "Foxhole Server";
    c.description = "Guerre multijoueur persistante à grande échelle (community/foxhole-server).";
    c.note = "ℹ Ports 9000–9002/UDP requis. Serveur communautaire officiel requis.";
    c.defaultContainerName = "foxhole-server";
    c.dockerImage = "community/foxhole-server";
    c.dataVolumePath = "/foxhole";
    c.defaultPort = 9000;
    c.ports = { {9000,0,true,false},{9001,9001,true,false},{9002,9002,true,false} };
    c.btnColorStart = "#374151"; c.btnColorEnd = "#1f2937";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME", "Mon Serveur Foxhole", "SERVER_NAME"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeMordhau(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "⚔"; c.title = "Mordhau Server";
    c.description = "Combat médiéval compétitif — serveurs dédiés Steam (community/mordhau-server).";
    c.note = "ℹ Ports 7777/UDP, 15000/UDP, 27015/UDP requis. Licence Steam requise.";
    c.defaultContainerName = "mordhau-server";
    c.dockerImage = "community/mordhau-server";
    c.dataVolumePath = "/mordhau";
    c.defaultPort = 7777;
    c.ports = { {7777,0,true,false},{15000,15000,true,false},{27015,27015,true,false} };
    c.btnColorStart = "#78350f"; c.btnColorEnd = "#92400e";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur Mordhau","SERVER_NAME"),
        sf("Joueurs max",     "MAX_PLAYERS",     48, 2, 64,            "MAX_PLAYERS"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                   "SERVER_PASSWORD","Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makePavlov(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🎮"; c.title = "Pavlov VR Server";
    c.description = "FPS VR — serveurs dédiés communautaires (th0m/pavlov-server).";
    c.note = "ℹ Ports 7777/UDP et 8177/UDP requis.";
    c.defaultContainerName = "pavlov-server";
    c.dockerImage = "th0m/pavlov-server";
    c.dataVolumePath = "/home/steam/pavlovserver";
    c.defaultPort = 7777;
    c.ports = { {7777,0,true,false},{8177,8177,true,false} };
    c.btnColorStart = "#6366f1"; c.btnColorEnd = "#8b5cf6";
    c.fields = {
        tf("Nom du serveur",  "SERVER_NAME",     "Mon Serveur Pavlov","SERVER_NAME"),
        sf("Joueurs max",     "MAX_PLAYERS",     10, 2, 24,           "MAX_PLAYERS"),
        pf("Mot de passe",    "SERVER_PASSWORD", "",                  "SERVER_PASSWORD","Optionnel"),
    };
    return new GenericGamePage(d, c, vml, p);
}

static GenericGamePage *makeSauerbraten(DockerManager *d, const QVector<VMInstance> *vml, QWidget *p)
{
    GamePageConfig c;
    c.icon = "🔫"; c.title = "Sauerbraten (Cube 2)";
    c.description = "FPS arena rapide open-source — gratuit et léger (captaingeech/cube2srv).";
    c.note = "ℹ Ports 28785 et 28786 (UDP+TCP) requis. La configuration se fait via server-init.cfg "
             "dans les paramètres avancés.";
    c.defaultContainerName = "sauerbraten-server";
    c.dockerImage = "captaingeech/cube2srv";
    c.dataVolumePath = "/srv/sauerbraten";
    c.defaultPort = 28785;
    c.ports = { {28785,0,true,true},{28786,28786,true,true} };
    c.btnColorStart = "#dc2626"; c.btnColorEnd = "#f59e0b";
    c.configDocUrl   = "http://sauerbraten.org/docs/config.html";
    c.configFilePath = "/srv/sauerbraten/server-init.cfg";
    c.fields = {
        tf("Description du serveur", "serverdesc",     "Mon Serveur Sauerbraten", "serverdesc"),
        pf("Mot de passe",          "serverpassword", "",                        "serverpassword", "Optionnel"),
        sf("Joueurs max",           "maxclients",     8, 2, 24,                  "maxclients"),
        cf("Visibilité",            "publicserver",
           {"0 — Privé (LAN)","1 — Public (master server)"},
           "1 — Public (master server)", "publicserver"),
    };
    return new GenericGamePage(d, c, vml, p);
}

// ── MainWindow ────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Game Server Manager");
    setFixedSize(1100, 700);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    m_docker = new DockerManager(this);

    QSettings settings("GameServerManager", "App");
    if (settings.value("docker/remote", false).toBool()) {
        m_docker->setRemoteMode(
            settings.value("docker/host").toString(),
            settings.value("docker/sshPort", 22).toInt(),
            settings.value("docker/user").toString(),
            settings.value("docker/keyPath").toString()
        );
    }

    // Load VM list (always has Local at index 0)
    m_vmList = VMStorage::load();
    const QVector<VMInstance> *vml = &m_vmList;

    auto *sidebar = new SideBar(this);
    m_stack       = new QStackedWidget(this);

    // Helper to register selectors for later refresh
    auto regSelectors = [this](QWidget *page) {
        // Collect DeploymentTargetSelector children from the page
        for (auto *child : page->findChildren<DeploymentTargetSelector *>())
            m_targetSelectors.append(child);
    };

    // ── SURVIE / SANDBOX ────────────────────────────────────────────────
    auto *mcPage = new MinecraftPage(m_docker, vml, this);
    m_stack->addWidget(mcPage);                                 //  0
    regSelectors(mcPage);
    auto *valheim = makeValheim(m_docker, vml, this);
    m_stack->addWidget(valheim);                                //  1
    regSelectors(valheim);
    auto *ark = makeARK(m_docker, vml, this);
    m_stack->addWidget(ark);                                    //  2
    regSelectors(ark);
    auto *rust = makeRust(m_docker, vml, this);
    m_stack->addWidget(rust);                                   //  3
    regSelectors(rust);
    auto *palworld = makePalworld(m_docker, vml, this);
    m_stack->addWidget(palworld);                               //  4
    regSelectors(palworld);
    auto *enshrouded = makeEnshrouded(m_docker, vml, this);
    m_stack->addWidget(enshrouded);                             //  5
    regSelectors(enshrouded);

    // ── FPS / TACTIQUE ──────────────────────────────────────────────────
    auto *cs2Page = new CS2Page(m_docker, vml, this);
    m_stack->addWidget(cs2Page);                                //  6
    regSelectors(cs2Page);
    auto *cs16 = makeCS16(m_docker, vml, this);
    m_stack->addWidget(cs16);                                   //  7
    regSelectors(cs16);
    auto *tf2 = makeTF2(m_docker, vml, this);
    m_stack->addWidget(tf2);                                    //  8
    regSelectors(tf2);
    auto *squad = makeSquad(m_docker, vml, this);
    m_stack->addWidget(squad);                                  //  9
    regSelectors(squad);
    auto *insurgency = makeInsurgency(m_docker, vml, this);
    m_stack->addWidget(insurgency);                             // 10
    regSelectors(insurgency);
    auto *arma = makeArma(m_docker, vml, this);
    m_stack->addWidget(arma);                                   // 11
    regSelectors(arma);
    auto *l4d2 = makeL4D2(m_docker, vml, this);
    m_stack->addWidget(l4d2);                                   // 12
    regSelectors(l4d2);

    // ── ACTION / RPG ────────────────────────────────────────────────────
    auto *conan = makeConan(m_docker, vml, this);
    m_stack->addWidget(conan);                                  // 13
    regSelectors(conan);
    auto *d7td = make7DTD(m_docker, vml, this);
    m_stack->addWidget(d7td);                                   // 14
    regSelectors(d7td);
    auto *dayz = makeDayZ(m_docker, vml, this);
    m_stack->addWidget(dayz);                                   // 15
    regSelectors(dayz);
    auto *sotf = makeSotF(m_docker, vml, this);
    m_stack->addWidget(sotf);                                   // 16
    regSelectors(sotf);
    auto *vrising = makeVRising(m_docker, vml, this);
    m_stack->addWidget(vrising);                                // 17
    regSelectors(vrising);
    auto *terraria = makeTerraria(m_docker, vml, this);
    m_stack->addWidget(terraria);                               // 18
    regSelectors(terraria);
    auto *starbound = makeStarbound(m_docker, vml, this);
    m_stack->addWidget(starbound);                              // 19
    regSelectors(starbound);

    // ── SIMULATION ──────────────────────────────────────────────────────
    auto *spaceEng = makeSpaceEngineers(m_docker, vml, this);
    m_stack->addWidget(spaceEng);                               // 20
    regSelectors(spaceEng);
    auto *satisfactory = makeSatisfactory(m_docker, vml, this);
    m_stack->addWidget(satisfactory);                           // 21
    regSelectors(satisfactory);
    auto *factorio = makeFactorio(m_docker, vml, this);
    m_stack->addWidget(factorio);                               // 22
    regSelectors(factorio);
    auto *astroneer = makeAstroneer(m_docker, vml, this);
    m_stack->addWidget(astroneer);                              // 23
    regSelectors(astroneer);
    auto *stationeers = makeStationeers(m_docker, vml, this);
    m_stack->addWidget(stationeers);                            // 24
    regSelectors(stationeers);

    // ── COURSE / SPORT ──────────────────────────────────────────────────
    auto *assetto = makeAssettoCorsa(m_docker, vml, this);
    m_stack->addWidget(assetto);                                // 25
    regSelectors(assetto);
    auto *beamng = makeBeamNG(m_docker, vml, this);
    m_stack->addWidget(beamng);                                 // 26
    regSelectors(beamng);

    // ── MMO ─────────────────────────────────────────────────────────────
    auto *zomboid = makeProjectZomboid(m_docker, vml, this);
    m_stack->addWidget(zomboid);                                // 27
    regSelectors(zomboid);
    auto *wurm = makeWurm(m_docker, vml, this);
    m_stack->addWidget(wurm);                                   // 28
    regSelectors(wurm);
    auto *feudal = makeLifeIsFeudal(m_docker, vml, this);
    m_stack->addWidget(feudal);                                 // 29
    regSelectors(feudal);

    // ── NICHE ───────────────────────────────────────────────────────────
    auto *gmod = makeGMod(m_docker, vml, this);
    m_stack->addWidget(gmod);                                   // 30
    regSelectors(gmod);
    auto *baro = makeBarotrauma(m_docker, vml, this);
    m_stack->addWidget(baro);                                   // 31
    regSelectors(baro);
    auto *vintage = makeVintageStory(m_docker, vml, this);
    m_stack->addWidget(vintage);                                // 32
    regSelectors(vintage);
    auto *foxhole = makeFoxhole(m_docker, vml, this);
    m_stack->addWidget(foxhole);                                // 33
    regSelectors(foxhole);
    auto *mordhau = makeMordhau(m_docker, vml, this);
    m_stack->addWidget(mordhau);                                // 34
    regSelectors(mordhau);
    auto *pavlov = makePavlov(m_docker, vml, this);
    m_stack->addWidget(pavlov);                                 // 35
    regSelectors(pavlov);
    auto *sauerbraten = makeSauerbraten(m_docker, vml, this);
    m_stack->addWidget(sauerbraten);                            // 36
    regSelectors(sauerbraten);

    // ── INFRASTRUCTURE ──────────────────────────────────────────────────
    m_infraPage = new InfraPage(&m_vmList, this);
    m_stack->addWidget(m_infraPage);                            // 37
    connect(m_infraPage, &InfraPage::vmListChanged,
            this, &MainWindow::onVMListChanged);

    // Content area
    auto *contentFrame = new QFrame(this);
    contentFrame->setAttribute(Qt::WA_StyledBackground, true);
    contentFrame->setStyleSheet(R"(
        QFrame {
            background-color: #f1f5f9;
            border-top-right-radius: 16px;
            border-bottom-right-radius: 16px;
        }
    )");
    auto *cf = new QVBoxLayout(contentFrame);
    cf->setContentsMargins(0, 0, 0, 0);
    cf->addWidget(m_stack);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(sidebar);
    root->addWidget(contentFrame, 1);

    connect(sidebar, &SideBar::pageSelected, m_stack, &QStackedWidget::setCurrentIndex);
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0.0, QColor("#1e1b4b"));
    grad.setColorAt(1.0, QColor("#2d2a6e"));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 16, 16);
}

void MainWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(e);
}

void MainWindow::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton))
        move(e->globalPosition().toPoint() - m_dragPos);
    QWidget::mouseMoveEvent(e);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        m_dragging = false;
    QWidget::mouseReleaseEvent(e);
}

void MainWindow::onVMListChanged()
{
    for (auto *sel : m_targetSelectors)
        sel->refreshList();
}
