# Documentation — GameServer Manager

Bienvenue dans la documentation de **GameServer Manager**, l'application de bureau Windows
qui déploie et administre vos serveurs de jeux via Docker, en local ou à distance.

## Sommaire

### Pour les utilisateurs
- **[Guide d'utilisation](guide-utilisation.md)** — Premiers pas : se connecter, installer un serveur, le surveiller, le sauvegarder.
- **[Déploiement & cibles](deploiement.md)** — Déployer en local, sur une VM personnelle (SSH) ou sur une VM cloud Scaleway.
- **[Référence des jeux](jeux.md)** — Liste complète des jeux supportés, leurs images Docker, ports et options.
- **[FAQ & dépannage](faq.md)** — Problèmes courants et leurs solutions.

### Pour les développeurs
- **[Architecture](architecture.md)** — Organisation du code, classes principales, flux de données.
- **[Build & développement](build.md)** — Compiler le projet, ajouter un nouveau jeu, conventions de code.

## En une phrase

GameServer Manager orchestre les CLI `docker` et `ssh` standards depuis une interface Qt :
vous choisissez un jeu, remplissez un court formulaire, et l'app crée et pilote le conteneur
Docker correspondant — sur votre machine ou sur une VM distante.

## Concepts clés

| Terme | Signification |
|-------|---------------|
| **Cible de déploiement** | L'endroit où tourne le conteneur : `Local` (votre PC), `Custom` (votre VM via SSH), ou `Scaleway` (VM cloud créée par l'app). |
| **Serveur (instance)** | Un conteneur Docker correspondant à un serveur de jeu configuré. Plusieurs instances d'un même jeu sont possibles. |
| **Page de jeu** | L'écran d'un jeu : formulaire d'installation + tableau de bord une fois le serveur déployé. |
| **Tableau de bord** | Vue temps réel d'un serveur en cours d'exécution (stats, joueurs, backups, console, logs). |
| **VM / Infra** | Une machine (locale ou distante) capable d'exécuter Docker, gérée depuis la page *Infrastructure*. |
