# GameServerManager

> Application de bureau Windows (Qt 6 / C++17) pour **installer, déployer et administrer des serveurs de jeux** — Minecraft, Counter-Strike 2 (CS2) et serveurs génériques — via **Docker**, en local ou à distance par **tunnel SSH**, avec prise en charge du cloud **Scaleway** et de VMs personnalisées.

GameServerManager est un panneau de gestion de serveurs de jeux tout-en-un : une interface graphique moderne qui automatise le déploiement de conteneurs Docker, la configuration des serveurs, la gestion des plugins, la console RCON et le suivi des logs en temps réel — sans ligne de commande.

## Fonctionnalités

- 🎮 **Multi-jeux** — Minecraft, CS2 (Counter-Strike 2) et serveurs de jeux génériques, avec une page dédiée par jeu.
- 🐳 **Déploiement Docker** — installation et orchestration des serveurs dans des conteneurs Docker en un clic.
- 🔐 **Exécution locale ou distante** — gestion en local ou sur serveur distant via tunnel SSH (authentification par clé).
- ☁️ **Cloud & VMs** — provisioning sur le cloud Scaleway et connexion à des machines virtuelles personnalisées.
- ⚙️ **Configuration éditable** — paramètres du serveur modifiables avant et après l'installation.
- 🧩 **Gestion des plugins** — installation et administration des plugins/mods depuis l'interface.
- 🖥️ **Console & logs** — console de commandes interactive et visualisation des logs des conteneurs en direct.
- 📊 **Tableau de bord** — vue unifiée de l'état des serveurs (démarrage, arrêt, supervision).

## Mots-clés

`game server manager` · `gestionnaire serveur de jeu` · `serveur Minecraft` · `serveur CS2` · `Counter-Strike 2` · `panel serveur de jeu` · `game server panel` · `Docker` · `conteneurs` · `SSH` · `déploiement serveur` · `Scaleway` · `cloud gaming` · `auto-hébergement` · `self-hosted` · `Qt 6` · `C++17` · `application Windows` · `RCON` · `gestion de plugins` · `hébergement serveur de jeu`

## Stack technique

- **Langage** : C++17
- **Framework UI** : Qt 6 (`Qt6::Widgets`, `Qt6::Network`), MinGW 64-bit
- **Build** : CMake 3.16+, `windeployqt`
- **Runtime** : Windows, CLIs `docker` et `ssh` sur le PATH (local ou distant)

## Build & exécution

Utilisez le script `build.bat` fourni à la racine du dépôt (configure CMake → compile → `windeployqt`) :

```cmd
build.bat
```

Le binaire généré est `build/app-qt.exe`. Qt est attendu dans `C:/Qt/6.7.3/mingw_64/` ; ajustez les chemins en tête de `build.bat` pour une autre installation.

## Architecture

Interface organisée en pages dans un `QStackedWidget` : pages spécifiques par jeu (`MinecraftPage`, `CS2Page`, `GenericGamePage`) et classes managers (`DockerManager`, `SshTunnel`) encapsulant les opérations sur les conteneurs et le SSH. Voir [`CLAUDE.md`](CLAUDE.md) pour les détails d'implémentation.
