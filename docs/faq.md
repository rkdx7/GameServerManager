# FAQ & dépannage

## Général

### Quels sont les identifiants par défaut ?
`admin` / `1234`. C'est une authentification **locale** (aucun serveur), uniquement destinée à
protéger l'accès à l'interface.

### Sur quels systèmes l'application fonctionne-t-elle ?
Windows uniquement (10/11). Les serveurs de jeux, eux, tournent dans des conteneurs Docker —
en local sur votre PC, ou sur une VM Linux distante.

### Mes données de serveur sont-elles enfermées dans l'application ?
Non. Chaque serveur est un **conteneur Docker standard** avec ses données dans `/data`. Vous
gardez le contrôle via Docker directement si besoin.

## Installation & build

### Le build échoue : Qt introuvable
Le chemin Qt est codé en dur (`C:/Qt/6.7.3/mingw_64/`). Installez exactement cette version
MinGW 64-bit, ou modifiez la configuration CMake pour pointer vers votre installation. Voir
[build.md](build.md).

### `docker` ou `ssh` introuvable
Les deux binaires doivent être sur le `PATH`. Installez Docker Desktop (qui fournit `docker`) et
assurez-vous que `ssh` (client OpenSSH de Windows) est disponible. Testez dans un terminal :
`docker --version` et `ssh -V`.

## Déploiement local

### Mon serveur ne démarre pas en local
- Vérifiez que **Docker Desktop est lancé** (et non simplement installé).
- Vérifiez qu'aucun autre service n'occupe déjà le **port** choisi.
- Consultez l'onglet **Logs** du serveur pour voir le message d'erreur du conteneur.

### « Port already in use »
Un autre conteneur ou service utilise déjà ce port. Choisissez un autre port, ou arrêtez le
service en conflit. Pour plusieurs instances d'un même jeu générique, le port est incrémenté
automatiquement.

## Déploiement distant (VM / SSH)

### Le serveur distant ne répond pas / l'action reste sans effet
Les commandes SSH utilisent `BatchMode=yes` et **échouent silencieusement en cas de délai
dépassé**. Vérifiez :
- la **connectivité** vers la VM (`ssh user@host` fonctionne-t-il manuellement ?) ;
- que le **chemin de la clé privée** renseigné est correct ;
- que **Docker est installé et démarré** sur la VM ;
- que l'utilisateur SSH a le droit d'exécuter `docker`.

### L'authentification SSH par mot de passe ne marche pas
C'est normal : seule l'**authentification par clé** est prise en charge. Configurez une clé SSH
et renseignez son chemin lors de l'ajout de la VM.

### Erreur de host key
Les connexions utilisent `StrictHostKeyChecking=no`, donc la validation de host key ne devrait
pas bloquer. Si un problème persiste, vérifiez votre fichier `known_hosts` local.

## Scaleway

### La création de VM échoue
- Vérifiez que votre **clé API Scaleway** est valide et a les droits de création d'instances.
- Vérifiez la **zone/région** et le **type d'instance** choisis (disponibilité, quotas).
- La création passe par plusieurs étapes (image → création → attente *Running* → installation de
  Docker) : laissez le temps au provisioning de se terminer (statut 🟠 *Provisioning*).

### Vais-je être facturé ?
Oui. Une VM Scaleway est une ressource cloud **payante**. Supprimez-la depuis la page
**Infrastructure** quand vous ne l'utilisez plus.

## Serveurs & administration

### Le nombre de joueurs reste à zéro
Le comptage des joueurs n'est disponible que pour **Minecraft** et **CS2** (via RCON). Pour CS2,
vérifiez que le **mot de passe RCON** est correctement configuré.

### Mes sauvegardes
- Une sauvegarde est une archive `backup_<date>.tar.gz` du dossier `/data`, stockée dans
  `/data/backups/` du conteneur.
- La restauration ré-extrait l'archive sélectionnée dans `/data`.
- La **planification automatique** et la gestion par **snapshots** ne sont pas encore
  disponibles (voir [`../todo`](../todo)).

### Où sont mes plugins Minecraft ?
L'onglet **Plugins** n'apparaît que pour les serveurs Minecraft de type **Paper** ou **Spigot**.
Il permet de lister, installer (`.jar`), configurer et supprimer les plugins.

### J'ai désinstallé un serveur par erreur
La désinstallation supprime le conteneur **et son volume** : l'opération est irréversible.
Sauvegardez et récupérez vos données avant toute désinstallation si vous souhaitez les conserver.

## Une image Docker ne fonctionne plus

Les images des jeux génériques sont maintenues par des tiers sur Docker Hub. Si l'une d'elles
disparaît ou change, ajustez la configuration du jeu (image/tag) — voir [build.md](build.md) et
[jeux.md](jeux.md).
