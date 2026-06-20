# Guide d'utilisation

Ce guide vous accompagne de la première connexion jusqu'à l'administration d'un serveur en
production.

## 1. Lancer l'application

Après avoir compilé le projet (voir [build.md](build.md)), lancez `build/app-qt.exe`.

### Se connecter

Une fenêtre de connexion s'ouvre. Il s'agit d'une authentification **locale** (aucun serveur
distant), uniquement destinée à protéger l'accès à l'interface.

- **Identifiant :** `admin`
- **Mot de passe :** `1234`

> Ces identifiants sont des valeurs par défaut codées dans l'application.

## 2. Découvrir l'interface

L'application est organisée en deux zones :

- **Le menu latéral (gauche)** — la liste des jeux regroupés par catégorie (Survie, FPS,
  Action/RPG, Simulation, Course, MMO, Niche), suivie des pages *Système* (Docker) et
  *Infrastructure* (☁ Infra).
- **La zone principale (droite)** — la page du jeu ou de l'outil sélectionné.

Chaque page de jeu affiche en haut une **bannière** aux couleurs du jeu, puis soit le
**formulaire d'installation** (si aucun serveur n'existe), soit le **tableau de bord** (si un
serveur est déjà déployé).

## 3. Installer un serveur de jeu

Prenons l'exemple de **Minecraft** (la démarche est identique pour les autres jeux).

1. Cliquez sur **🎮 Minecraft** dans le menu latéral.
2. Remplissez le formulaire :
   - **Nom du conteneur** — un identifiant unique pour ce serveur (ex. `mc-survie`).
   - **Version** — `LATEST` ou une version précise (1.21.4, 1.20.1…).
   - **Type de serveur** — `PAPER`, `VANILLA`, `SPIGOT`, `FORGE` ou `FABRIC`.
   - **Mémoire allouée** — ex. `4G`.
   - **Port** — le port d'écoute (25565 par défaut).
   - **EULA** — cochez *J'accepte l'EULA Minecraft* (obligatoire pour démarrer).
3. (Optionnel) Ouvrez les **Paramètres avancés** pour coller un `server.properties` personnalisé.
4. Choisissez la **cible de déploiement** en bas (voir [deploiement.md](deploiement.md)).
5. Cliquez sur **🚀 Installer**.

L'application crée alors le conteneur Docker, applique votre configuration sous forme de
variables d'environnement, et démarre le serveur. Une zone de statut vous informe de la
progression.

> **CS2** propose des options spécifiques (mot de passe RCON, GSLT, type/mode de jeu, map de
> départ). Les autres jeux utilisent un formulaire généré automatiquement à partir de leur
> configuration — voir [jeux.md](jeux.md).

### Plusieurs serveurs d'un même jeu

Les pages Minecraft, CS2 et génériques gèrent une **liste de serveurs** : vous pouvez déployer
plusieurs instances (avec des noms et ports différents) et basculer de l'une à l'autre depuis
le panneau latéral *SERVEURS*. Le port est incrémenté automatiquement pour chaque nouvelle
instance générique.

## 4. Surveiller un serveur (tableau de bord)

Une fois un serveur installé, la page affiche son **tableau de bord**, rafraîchi en continu :

- **Statut** — point vert (en cours) ou rouge (arrêté).
- **CPU / RAM / Disque** — barres de progression en temps réel.
- **Joueurs** — nombre de joueurs connectés (Minecraft et CS2 via RCON).
- **Boutons d'action** — **Démarrer**, **Arrêter**, **Redémarrer**.

Le tableau de bord propose plusieurs **onglets** :

| Onglet | Description |
|--------|-------------|
| **Console** | Envoyez des commandes d'administration (RCON) au serveur. |
| **Logs** | Lisez les logs du conteneur en direct (stdout + stderr, dernières lignes). |
| **Plugins** | *(Minecraft Paper/Spigot uniquement)* Installez, configurez et supprimez vos plugins. |

## 5. Sauvegarder et restaurer

Depuis le tableau de bord :

- **Créer une sauvegarde** — l'app crée une archive `backup_<date>.tar.gz` du dossier `/data`
  du conteneur (en excluant le dossier des backups eux-mêmes).
- **Restaurer** — sélectionnez une archive dans la liste déroulante et restaurez-la dans `/data`.

Les sauvegardes sont stockées dans `/data/backups/` à l'intérieur du conteneur (donc sur le
volume Docker, qu'il soit local ou distant).

> ⚠️ La planification automatique des sauvegardes et la gestion par « snapshots » sont prévues
> mais pas encore disponibles (voir [`../todo`](../todo)).

## 6. Gérer les plugins Minecraft

Pour un serveur Minecraft de type **Paper** ou **Spigot**, l'onglet **Plugins** vous permet de :

- lister les plugins installés et leurs dossiers de configuration ;
- lire et modifier les fichiers de configuration d'un plugin ;
- installer un nouveau plugin en fournissant un fichier `.jar` local ;
- supprimer un plugin existant.

## 7. Désinstaller un serveur

Depuis le tableau de bord, l'action de désinstallation supprime le conteneur (et son volume de
données). Cette opération est **irréversible** : pensez à créer une sauvegarde et à la récupérer
au préalable si vous souhaitez conserver le monde.

## Étapes suivantes

- Déployer ailleurs que sur votre PC ? → [Déploiement & cibles](deploiement.md)
- Un problème ? → [FAQ & dépannage](faq.md)
