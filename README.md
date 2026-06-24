<div align="center">

# 🎮 GameServer Manager

**Hébergez vos serveurs de jeux en quelques clics — sans ligne de commande.**

Une application de bureau Windows qui déploie, surveille et administre vos serveurs de jeux
via Docker, en local ou sur une VM distante, depuis une interface moderne et unifiée.

[![Qt](https://img.shields.io/badge/Qt-6.7-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![Docker](https://img.shields.io/badge/Docker-required-2496ED?logo=docker&logoColor=white)](https://www.docker.com/)
[![Platform](https://img.shields.io/badge/Windows-10%2F11-0078D6?logo=windows&logoColor=white)](#)

</div>

---

## ✨ Pourquoi GameServer Manager ?

Monter un serveur de jeu, c'est normalement : chercher la bonne image, mémoriser des dizaines
de variables d'environnement, jongler avec `docker run`, ouvrir des ports, surveiller la RAM à la
main et bricoler des sauvegardes. **GameServer Manager fait tout ça à votre place.**

- 🚀 **Déploiement en un clic** — choisissez un jeu, remplissez un court formulaire, cliquez sur *Installer*. Le conteneur Docker est créé, configuré et démarré pour vous.
- 🌍 **Local ou distant, au même endroit** — déployez sur votre PC, sur une VM que vous possédez (SSH), ou faites créer une VM cloud **Scaleway** directement depuis l'app. Le même formulaire, peu importe la cible.
- 📊 **Tableau de bord temps réel** — CPU, RAM, disque, nombre de joueurs connectés, le tout rafraîchi en continu. Start / Stop / Restart en un bouton.
- 💾 **Sauvegardes intégrées** — créez et restaurez des backups de vos mondes sans toucher au terminal.
- 🖥️ **Console & logs en direct** — envoyez des commandes RCON et lisez les logs du conteneur directement dans l'app, pour chaque serveur.
- 🧩 **Gestionnaire de plugins Minecraft** — installez, configurez et supprimez vos plugins Paper/Spigot depuis une interface dédiée.
- 🎯 **Plus de 35 jeux pris en charge** — de Minecraft à CS2, en passant par Valheim, Rust, ARK, Palworld, Factorio, Project Zomboid et bien d'autres.

---

## 🕹️ Jeux pris en charge

> Plus de **35 serveurs** prêts à l'emploi, classés par catégorie dans le menu latéral.

| Catégorie | Jeux |
|-----------|------|
| **Survie / Sandbox** | Minecraft · Valheim · ARK: Survival · Rust · Palworld · Enshrouded |
| **FPS / Tactique** | CS2 / CS:GO · Counter-Strike 1.6 · Team Fortress 2 · Squad · Insurgency: Sandstorm · Arma 3 / Reforger · Left 4 Dead 2 |
| **Action / RPG** | Conan Exiles · 7 Days to Die · DayZ · Sons of the Forest · V Rising · Terraria · Starbound |
| **Simulation** | Space Engineers · Satisfactory · Factorio · Astroneer · Stationeers |
| **Course / Sport** | Assetto Corsa / ACC · BeamNG.drive |
| **MMO** | Project Zomboid · Wurm Unlimited · Life is Feudal |
| **Niche** | Garry's Mod · Barotrauma · Vintage Story · Foxhole · Mordhau · Pavlov VR · Sauerbraten (Cube 2) |

Minecraft et CS2 disposent de pages dédiées avec options avancées (type de serveur, EULA, RCON, GSLT…).
Tous les autres jeux sont gérés par un moteur de configuration générique unifié.

---

## 📸 Aperçu

```
┌──────────────┬────────────────────────────────────────────────┐
│  🎮 GameServer │   🎮  Minecraft Server                          │
│     Manager   │   Configurez et déployez via Docker            │
│              │  ┌──────────────────────────────────────────┐  │
│ SURVIE       │  │ Nom du conteneur  [ mc-survie          ] │  │
│ ▸ Minecraft  │  │ Version           [ 1.21.4         ▾   ] │  │
│ ▸ Valheim    │  │ Type de serveur   [ PAPER          ▾   ] │  │
│ ▸ Rust       │  │ Mémoire allouée   [ 4G                 ] │  │
│ FPS          │  │ Port              [ 25565             ] │  │
│ ▸ CS2/CS:GO  │  │ ☑ J'accepte l'EULA Minecraft            │  │
│ ▸ TF2        │  │                          [ 🚀 Installer ] │  │
│ INFRA        │  └──────────────────────────────────────────┘  │
│ ▸ ☁ Infra    │   Déploiement : 🖥 Local  /  ☁ Ma-VM-Scaleway   │
└──────────────┴────────────────────────────────────────────────┘
```

---

## ⚡ Démarrage rapide

### Prérequis

- **Windows 10/11**
- **[Qt 6.7.3](https://www.qt.io/download)** (MinGW 64-bit) — attendu dans `C:/Qt/6.7.3/mingw_64/`
- **CMake 3.16+**
- **[Docker](https://www.docker.com/products/docker-desktop/)** et **`ssh`** sur le `PATH` (en local et/ou sur la machine distante)

### Build

```bash
cmake -B build && cmake --build build
```

Puis lancez `build/app-qt.exe`.

> Connexion par défaut : **`admin` / `1234`** (page de login locale, sans serveur).

### En 3 étapes

1. **Choisissez votre jeu** dans le menu de gauche.
2. **Remplissez le formulaire** d'installation (nom, version, port…) et sélectionnez votre cible de déploiement.
3. **Cliquez sur Installer** — surveillez ensuite votre serveur depuis le tableau de bord.

📖 Le guide complet se trouve dans **[docs/](docs/README.md)**.

---

## 🛠️ Comment ça marche

GameServer Manager ne réinvente rien : il **orchestre les outils standards** que vous utiliseriez à la main.

- Chaque serveur de jeu est un **conteneur Docker** issu d'une image publique éprouvée (`itzg/minecraft-server`, `lloesche/valheim-server`, etc.).
- Les opérations se font en **shellant les CLI `docker` et `ssh`** via `QProcess`.
- En mode distant, les commandes Docker passent par un **tunnel SSH** (authentification par clé) vers la VM.
- Le provider **Scaleway** crée la VM via l'API cloud, y installe Docker automatiquement, puis l'app la pilote comme n'importe quelle cible distante.

Pas de magie, pas de lock-in : vos serveurs restent de simples conteneurs Docker que vous gardez sous contrôle.

---

## 📚 Documentation

| Document | Contenu |
|----------|---------|
| [Guide d'utilisation](docs/guide-utilisation.md) | Installer, surveiller, sauvegarder un serveur pas à pas |
| [Déploiement & cibles](docs/deploiement.md) | Local, VM personnalisée (SSH), Scaleway |
| [Référence des jeux](docs/jeux.md) | Liste des jeux, images Docker, ports et options |
| [Architecture](docs/architecture.md) | Structure du code, pour les contributeurs |
| [Build & développement](docs/build.md) | Compiler, ajouter un jeu, conventions |
| [FAQ & dépannage](docs/faq.md) | Problèmes courants et solutions |

---

## ⚠️ Statut du projet

Projet en développement actif. Certaines fonctionnalités (planification des sauvegardes,
snapshots, monitoring Prometheus/Grafana, VPN/firewall pour VM distantes) sont sur la feuille
de route. Voir [`todo`](todo). Il n'y a pas encore de tests automatisés : les changements se
vérifient en compilant et en exerçant l'interface.

## 🤝 Contribuer

Les contributions sont les bienvenues ! Créez une branche depuis `main` et ouvrez une *pull request*.
Consultez [docs/architecture.md](docs/architecture.md) et [docs/build.md](docs/build.md) pour démarrer.

## 📄 Licence

Ce projet est distribué sous licence **[PolyForm Noncommercial 1.0.0](LICENSE)**.

- ✅ **Libre d'utilisation, de modification et de redistribution** pour tout usage **non commercial** (usage personnel, projets amateurs, éducation, organismes à but non lucratif…).
- ❌ **Tout usage commercial est interdit** sans l'accord écrit de l'auteur.

Pour une licence commerciale, contactez l'auteur du projet.

> ℹ️ Il ne s'agit pas d'une licence « open source » au sens de l'OSI (qui imposerait d'autoriser l'usage commercial), mais d'une licence *source-available* : le code est public et librement utilisable, à l'exception de l'exploitation commerciale.
