# Build & développement

## Prérequis

| Outil | Détail |
|-------|--------|
| **OS** | Windows 10/11 (application Windows-only) |
| **Qt** | 6.7.3, MinGW 64-bit, attendu dans `C:/Qt/6.7.3/mingw_64/` |
| **CMake** | 3.16+ |
| **Composants Qt** | `Qt6::Widgets`, `Qt6::Network` |
| **Runtime** | `docker` et `ssh` sur le `PATH` (localement et/ou sur la machine distante) |

> Le chemin Qt est codé en dur dans la configuration CMake. Une autre installation/un autre
> chemin nécessite de modifier la configuration CMake.

## Compiler

```bash
cmake -B build && cmake --build build
```

Puis lancez l'exécutable :

```
build/app-qt.exe
```

> Il n'y a pas encore de tests automatisés. On vérifie un changement en **compilant** puis en
> **exerçant l'interface**.

## Conventions de code

À respecter pour rester cohérent avec l'existant (il n'y a pas de configuration de formateur ;
un `.clang-format` est présent à titre indicatif) :

- En-têtes protégés par `#pragma once`.
- Classes en **PascalCase** (`DockerManager`), fonctions en **camelCase**
  (`isContainerRunning()`), membres préfixés `m_`.
- Architecture **signaux/slots** Qt avec `Q_OBJECT` ; gestion mémoire manuelle via
  `new` / `deleteLater()` (pas de smart pointers).
- Chevrons (`<>`) pour les includes Qt/système, guillemets (`""`) pour les includes locaux.
- UI **basée sur des pages** : pages de jeu dans un `QStackedWidget`, classes manager pour les
  opérations (conteneurs/SSH).

## Ajouter un nouveau jeu

Dans l'immense majorité des cas, **aucune nouvelle classe n'est nécessaire** : on réutilise
`GenericGamePage` paramétrée par un `GamePageConfig`.

1. **Vérifiez la documentation officielle** du serveur de jeu et choisissez une **image Docker**
   adaptée (idéalement maintenue et populaire sur Docker Hub).
2. Dans `src/app/MainWindow.cpp`, ajoutez une fonction qui construit un `GamePageConfig` :
   - `icon`, `title`, `description` ;
   - `dockerImage`, `defaultPort`, `ports` (mappings) ;
   - `fields` — les champs du formulaire (`GameFieldConfig`), chacun pouvant être lié à une
     variable d'environnement (`envVar`) ;
   - `btnColorStart` / `btnColorEnd` — un dégradé **aux couleurs du jeu** (réutilisé pour la
     bannière).
3. Instanciez une `GenericGamePage` avec cette config et ajoutez-la au `QStackedWidget`.
4. Ajoutez l'entrée correspondante dans la `SideBar` (`src/app/SideBar.cpp`), dans la bonne
   catégorie, en respectant l'ordre des index de page.
5. Mettez à jour la [référence des jeux](jeux.md).

### Règles transverses (voir `CLAUDE.md`)

- **Toutes les options doivent rester éditables après l'installation**, sauf celles
  intrinsèquement requises à l'installation (IP, port, nom du serveur).
- **Chaque page de jeu doit afficher une bannière** en haut de son formulaire, via le helper
  partagé `buildGameBanner(icon, title, subtitle, colorStart, colorEnd, container)` de
  `src/widgets/GameBanner.h` — pas de labels icône/titre ad hoc.
- **Chaque serveur doit exposer un onglet Logs** dans sa vue « serveur en cours ». Il est fourni
  automatiquement par `ServerDashboard` (`GameLogsWidget` adossé à `DockerManager::containerLogs`).
  Ne le retirez pas ; si un nouveau type de serveur contourne `ServerDashboard`, répliquez-y
  l'onglet Logs.
- Quand on ajoute une fonctionnalité pour **un** serveur, on l'ajoute à **tous** les serveurs de
  jeux (sauf si l'option n'existe pas pour un jeu donné — vérifiez la doc de chacun).

## Workflow Git

- Branchez depuis `main` et ouvrez une **pull request** pour relecture avant de merger.
- Ne committez pas directement sur `main`.

## Pistes d'évolution

Voir le fichier [`../todo`](../todo) à la racine : sauvegardes planifiées, snapshots,
renommage de serveurs, rechargement automatique des versions depuis les tags du registry,
monitoring (Prometheus/Grafana), chat vocal (TS3/Mumble), site web/forum, sécurité des VMs
distantes (WireGuard, firewall, fail2ban), CGU et choix de licence, etc.
