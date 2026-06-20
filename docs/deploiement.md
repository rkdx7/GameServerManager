# Déploiement & cibles

GameServer Manager peut faire tourner vos serveurs à trois endroits différents, **avec le même
formulaire d'installation**. La cible se choisit via le sélecteur *Déploiement* présent en bas
de chaque page de jeu.

## Les trois cibles

| Cible | Provider | Où tourne le conteneur | Mise en place |
|-------|----------|------------------------|---------------|
| **Local** | `Local` | Sur votre PC Windows | Docker Desktop installé et démarré |
| **VM personnelle** | `Custom` | Sur une machine que vous possédez | Accès SSH par clé + Docker sur la VM |
| **Scaleway** | `Scaleway` | Sur une VM cloud créée par l'app | Compte + clé API Scaleway |

Le sélecteur de déploiement bascule automatiquement le `DockerManager` en mode local
(`setLocalMode()`) ou distant (`setRemoteMode(host, sshPort, user, keyPath)`) selon la cible
choisie. En mode distant, toutes les commandes `docker` sont exécutées **à travers SSH** sur la
VM cible.

## 1. Déploiement local

C'est le mode le plus simple.

**Prérequis :**
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) installé et **en cours
  d'exécution** sur votre PC.
- Les commandes `docker` accessibles depuis le `PATH`.

Sélectionnez simplement **🖥 Local** comme cible, puis installez votre serveur. Les ports du
conteneur sont publiés sur `localhost`.

## 2. VM personnelle (SSH)

Déployez sur n'importe quelle machine Linux que vous administrez (serveur dédié, VPS, machine du
réseau local…).

**Prérequis sur la VM :**
- Docker installé et le démon démarré.
- Un accès **SSH par clé** (l'authentification par mot de passe n'est pas utilisée).
- Le client `ssh` disponible sur votre PC Windows.

**Ajouter la VM :**
1. Allez sur la page **☁ Infra** (Infrastructure).
2. Cliquez sur **🖥 Ajouter une VM**.
3. Renseignez :
   - **Nom** — un libellé pour identifier la VM.
   - **Host** — adresse IP ou nom d'hôte.
   - **Port SSH** — `22` par défaut.
   - **Utilisateur SSH** — `root` par défaut.
   - **Chemin de la clé privée** — fichier de clé SSH local utilisé pour l'authentification.

La VM apparaît alors comme cible sélectionnable dans le sélecteur de déploiement de chaque jeu.

> **Détails techniques SSH :** les connexions utilisent `StrictHostKeyChecking=no` et
> `BatchMode=yes`. En cas de délai dépassé, les commandes échouent silencieusement plutôt que de
> lever une erreur — pensez à vérifier la connectivité si un serveur ne répond pas.

## 3. VM cloud Scaleway

L'app peut **provisionner une VM cloud de A à Z** chez Scaleway, y installer Docker, puis la
piloter comme n'importe quelle VM distante.

**Prérequis :**
- Un compte [Scaleway](https://www.scaleway.com/).
- Une **clé API** Scaleway (token secret).

**Créer une VM :**
1. Page **☁ Infra** → **☁ Créer sur Scaleway**.
2. Renseignez la clé API, la zone/région et le type d'instance souhaité.
3. Validez. L'application va :
   - récupérer l'identifiant de l'image Ubuntu adéquate ;
   - créer le serveur via l'API Scaleway ;
   - attendre qu'il passe à l'état *Running* ;
   - **installer Docker** automatiquement sur la VM via SSH.

Une fois provisionnée, la VM apparaît dans la liste *Infra* avec son statut (Provisioning →
Running) et devient une cible de déploiement.

> 💡 **Facturation :** une VM Scaleway créée par l'app est une ressource cloud **payante**.
> Pensez à la supprimer (depuis la page Infra) lorsqu'elle n'est plus utilisée.

## Gérer l'infrastructure

La page **Infrastructure** liste toutes vos VMs sous forme de cartes, avec un indicateur de
statut :

- 🟢 **Running** — opérationnelle.
- 🟠 **Provisioning** — en cours de création.
- 🔴 **Stopped** — arrêtée.

Vous pouvez y ajouter, administrer et supprimer vos VMs. Les VMs et leur configuration sont
persistées localement (`VMStorage`).

## Choisir la bonne cible

| Vous voulez… | Cible recommandée |
|--------------|-------------------|
| Tester rapidement / jouer en LAN | **Local** |
| Un serveur public que vous hébergez déjà | **VM personnelle (SSH)** |
| Un serveur public sans gérer le matériel | **Scaleway** |
