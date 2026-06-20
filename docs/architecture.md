# Architecture

Document destiné aux contributeurs. Il décrit l'organisation du code et les principaux
composants de GameServer Manager.

## Vue d'ensemble

GameServer Manager est une application **Qt 6 / C++17** pour Windows. Elle ne contient aucune
logique serveur propre : elle **orchestre les CLI `docker` et `ssh`** via `QProcess`. L'UI est
construite autour d'un `QStackedWidget` (pages) et d'une `SideBar` de navigation.

```
┌─────────────────────────────────────────────────────────┐
│ LoginWindow  ──(succès)──▶  MainWindow                   │
│                              ├── SideBar (navigation)    │
│                              └── QStackedWidget          │
│                                   ├── MinecraftPage      │
│                                   ├── CS2Page            │
│                                   ├── GenericGamePage ×N │
│                                   ├── DockerSettingsPage │
│                                   └── InfraPage          │
│                                                          │
│  Pages ──▶ DockerManager ──▶ docker CLI                  │
│                   │           (local)                    │
│                   └─▶ ssh ──▶ docker CLI (VM distante)   │
│  InfraPage ──▶ ScalewayProvider ──▶ API Scaleway         │
└─────────────────────────────────────────────────────────┘
```

## Arborescence du code

Tout le code source vit sous `src/`, organisé par responsabilité :

| Dossier | Rôle |
|---------|------|
| `src/app/` | Coquille de l'application : `LoginWindow`, `MainWindow`, `SideBar`. |
| `src/pages/` | Pages du `QStackedWidget` : pages de jeu (`MinecraftPage`, `CS2Page`, `GenericGamePage`), `DockerSettingsPage`, `InfraPage`, `VMAdminPage`. |
| `src/widgets/` | Widgets réutilisables : `ServerDashboard`, `PluginManagerWidget`, `GameConsoleWidget`, `GameLogsWidget`, `DeploymentTargetSelector`, `GameBanner`. |
| `src/dialogs/` | Boîtes de dialogue : `ImagePickerDialog`, `AddCustomVMDialog`, `CreateVMDialog`. |
| `src/docker/` | Exécution Docker / remote : `DockerManager`, `SshTunnel`. |
| `src/cloud/` | Providers cloud : `CloudProviderBase`, `ScalewayProvider`, `VMStorage`. |
| `src/models/` | Structures de données (header-only) : `GamePageConfig`, `ServerInstance`, `VMInstance`. |

> Les includes locaux se font par **nom de fichier seul** (`#include "DockerManager.h"`) ;
> chaque sous-dossier de `src/` est exposé sur l'include path par le `CMakeLists.txt`.

## Composants clés

### DockerManager (`src/docker/`)

Cœur de l'application. Encapsule toutes les opérations sur les conteneurs et bascule de façon
transparente entre exécution **locale** et **distante** :

- `setLocalMode()` / `setRemoteMode(host, sshPort, user, keyPath)` — choisit la cible.
- Cycle de vie : `containerExists`, `isContainerRunning`, `startContainer`, `stopContainer`,
  `restartContainer`, `removeContainer`, `runDetached`.
- Stats & joueurs : `getStats` (CPU/RAM/disque), `getMinecraftPlayerCount`,
  `getCS2PlayerCount` (RCON).
- Sauvegardes : `listBackups`, `createBackup`, `restoreBackup` (archives `tar.gz` de `/data`).
- Logs : `containerLogs(name, tailLines)`.
- Plugins Minecraft : `listPlugins`, `readPluginConfig`, `writePluginConfig`,
  `installPlugin`, `deletePlugin`, etc.
- Bas niveau : `run()` exécute `docker …` localement, ou `ssh user@host docker …` à distance.

En mode distant, **toutes** les commandes Docker sont préfixées par un appel SSH
(`StrictHostKeyChecking=no`, `BatchMode=yes`, auth par clé). Les délais dépassés provoquent un
échec silencieux.

### GamePageConfig (`src/models/`)

Structure de configuration **déclarative** qui décrit un jeu générique : icône, titre, image
Docker, volume de données, port par défaut, mappings de ports, champs de formulaire
(`GameFieldConfig` : Text / Password / Combo / Spin / Check, avec variable d'environnement
associée), couleurs de bannière, et URL/chemin de configuration.

C'est cette structure qui permet d'ajouter un jeu **sans écrire de nouvelle classe** : il suffit
de remplir un `GamePageConfig` et d'instancier une `GenericGamePage`.

### Pages de jeu (`src/pages/`)

- **`MinecraftPage` / `CS2Page`** — pages dédiées avec logique et options spécifiques.
- **`GenericGamePage`** — page paramétrée par un `GamePageConfig`. La plupart des jeux passent
  par elle. `MainWindow` construit un `GamePageConfig` par jeu et instancie une `GenericGamePage`.

Toutes les pages de jeu :
1. affichent une **bannière** en haut du formulaire (`buildGameBanner(...)` de
   `src/widgets/GameBanner.h`) ;
2. gèrent une **liste de serveurs** persistée via `QSettings` ;
3. délèguent l'affichage du serveur en cours d'exécution à `ServerDashboard`.

### ServerDashboard (`src/widgets/`)

Tableau de bord temps réel partagé par tous les jeux. Affiche statut, CPU/RAM/disque, joueurs,
backups, et expose les onglets **Console** (`GameConsoleWidget`), **Logs** (`GameLogsWidget`) et,
pour Minecraft, **Plugins** (`PluginManagerWidget`). Un `QTimer` rafraîchit périodiquement les
stats (`startPolling` / `stopPolling`).

> ⚠️ L'onglet **Logs** est fourni automatiquement par `ServerDashboard`. Tout nouveau jeu qui
> utilise `ServerDashboard` en bénéficie. Ne pas le retirer.

### Cibles de déploiement (`src/widgets/DeploymentTargetSelector.*`)

Le sélecteur lit la liste des VMs et, selon le choix de l'utilisateur, configure le
`DockerManager` en local ou distant. La cible `Local` correspond à `nullptr` (pas de VM).

### Infrastructure & cloud (`src/pages/InfraPage`, `src/cloud/`)

`InfraPage` gère les VMs (ajout d'une VM custom, création Scaleway, statut, suppression).
`ScalewayProvider` (dérive de `CloudProviderBase`) crée une instance via l'API REST Scaleway
(`QNetworkAccessManager`), attend l'état *Running*, puis **installe Docker** sur la VM par SSH.
`VMStorage` persiste les VMs localement.

## Modèle de mémoire & signaux

- Architecture **signaux/slots** Qt classique avec `Q_OBJECT`.
- Gestion mémoire **manuelle** : `new` / `deleteLater()`, pas de smart pointers.
- Persistance via `QSettings` (organisation `GameServerManager`, application `App`).

## Pour aller plus loin

- Ajouter un jeu, compiler, conventions → [build.md](build.md)
- Voir le `CLAUDE.md` à la racine pour les règles transverses (bannière, onglet Logs,
  options éditables après installation).
