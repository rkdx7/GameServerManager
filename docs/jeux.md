# Référence des jeux

GameServer Manager prend en charge **plus de 35 serveurs de jeux**. Deux d'entre eux
(Minecraft et CS2) ont une page dédiée avec des options spécifiques ; tous les autres sont
gérés par un moteur générique configurable (`GenericGamePage` + `GamePageConfig`).

> Les images Docker ci-dessous sont celles définies par défaut dans l'application. La version
> exacte de l'image peut être surchargée depuis le formulaire (sélecteur d'image / tag).

## Jeux à page dédiée

### Minecraft
- **Image :** `itzg/minecraft-server`
- **Port par défaut :** 25565
- **Options :** version, type de serveur (`PAPER`, `VANILLA`, `SPIGOT`, `FORGE`, `FABRIC`),
  mémoire allouée, EULA, `server.properties` personnalisé (paramètres avancés).
- **Extras :** gestionnaire de plugins (Paper/Spigot), comptage des joueurs via RCON.

### CS2 / CS:GO
- **Image :** `joedwards32/cs2`
- **Port par défaut :** 27015
- **Options :** nom du serveur, type/mode de jeu, map de départ, mot de passe RCON, token GSLT.
- **Extras :** comptage des joueurs via RCON.

## Jeux génériques

Chaque jeu ci-dessous est déployé à partir d'une image Docker publique. Le formulaire
d'installation (nom, port, options, variables d'environnement) est généré automatiquement à
partir de la configuration du jeu.

### Survie / Sandbox

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Valheim | `lloesche/valheim-server` | 2456 |
| ARK: Survival Evolved | `hermsi/ark-server` | 7777 |
| Rust | `didstopia/rust-server` | 28015 |
| Palworld | `thijsvanloef/palworld-server-docker` | 8211 |
| Enshrouded | `sknnr/enshrouded-server` | 15636 |

### FPS / Tactique

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Counter-Strike 1.6 | `hlds/server` | 27015 |
| Team Fortress 2 | `cm2network/tf2` | 27015 |
| Squad | `cm2network/squad` | 7787 |
| Insurgency: Sandstorm | `cm2network/insurgency-sandstorm` | 27102 |
| Arma 3 / Arma Reforger | `muttley71/arma3server` | 2302 |
| Left 4 Dead 2 | `cm2network/l4d2` | 27015 |

### Action / RPG

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Conan Exiles | `alinmear/conan-exiles` | 7777 |
| 7 Days to Die | `didstopia/7dtd-server` | 26900 |
| DayZ | `immodal/dayz-server` | 2302 |
| Sons of the Forest | `jammsen/sons-of-the-forest-dedicated-server` | 8766 |
| V Rising | `trueosiris/vrising` | 9876 |
| Terraria (TShock) | `ryshe/tshock` | 7777 |
| Starbound | `didstopia/starbound-server` | 21025 |

### Simulation

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Space Engineers | `mmmaxwwwell/space-engineers-dedicated-docker` | 27016 |
| Satisfactory | `wolveix/satisfactory-server` | 7777 |
| Factorio | `factoriotools/factorio` | 34197 |
| Astroneer | `jammsen/astroneer-dedicated-server` | 8777 |
| Stationeers | `rocketwerkz/stationeers-dedicated` | 27500 |

### Course / Sport

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Assetto Corsa / ACC | `klauxius/assettocorsa-server` | 9600 |
| BeamNG.drive (BeamMP) | `lbvehicles/beammp-server` | 48900 |

### MMO

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Project Zomboid | `alinmear/project-zomboid` | 16261 |
| Wurm Unlimited | `ich777/wurmunlimited` | 3724 |
| Life is Feudal: Your Own | `ich777/lifeisfeudal` | 4305 |

### Niche

| Jeu | Image Docker | Port |
|-----|--------------|------|
| Garry's Mod | `cm2network/garrysmod` | 27015 |
| Barotrauma | `barotraumaofficial/barotrauma` | 27015 |
| Vintage Story | `devidian/vintage-story-server` | 42420 |
| Foxhole | `community/foxhole-server` | 9000 |
| Mordhau | `community/mordhau-server` | 7777 |
| Pavlov VR | `th0m/pavlov-server` | 7777 |
| Sauerbraten (Cube 2) | `captaingeech/cube2srv` | 28785 |

## Notes

- **Volume de données :** par défaut, les données du serveur sont stockées dans `/data` à
  l'intérieur du conteneur (sur un volume Docker). C'est ce dossier qui est sauvegardé.
- **Ports :** lorsque vous déployez plusieurs instances génériques, le port est incrémenté
  automatiquement à partir du port par défaut.
- **Surcharge d'image :** vous pouvez pointer une instance vers un tag/une image spécifique
  depuis le formulaire, sans modifier le code.
- **Disponibilité des images :** ces images sont maintenues par des tiers sur Docker Hub. Si une
  image n'est plus disponible ou a changé de nom, ajustez la configuration du jeu (voir
  [build.md](build.md)).
