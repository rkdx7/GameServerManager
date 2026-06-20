# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

GameServerManager: a Qt 6 / C++17 Windows desktop app for managing game servers (Minecraft, CS2, generic) over Docker via local or SSH-tunneled execution. Single project, no submodules.

## Build & run

Use the committed **`build.bat`** at the repo root — do not retype the toolchain
commands. It runs the full pipeline (CMake configure → build → `windeployqt`) and
builds in whatever checkout it lives in (via `%~dp0`):

```
build.bat
```

It must run from a native `cmd.exe` (the Qt MinGW `g++` fails to spawn `cc1plus`
when invoked directly from WSL). From a WSL shell:

```
cmd.exe /c "C:\path\to\repo\build.bat"
```

Then run `build/app-qt.exe`. There are no automated tests yet — verify changes by building and exercising the UI.

- Qt is hardcoded to `C:/Qt/6.7.3/mingw_64/` (MinGW 64-bit, CMake 3.16+). Toolchain paths (CMake, MinGW, Qt) are set at the top of `build.bat`; a different Qt install requires editing them there.
- Required Qt components: `Qt6::Widgets`, `Qt6::Network`.
- If CMake errors that the cache directory differs from where it was created (e.g. after switching checkout/worktree), delete `build/` and re-run `build.bat`.

## Platform & runtime constraints

- Windows-only. Server operations shell out via `QProcess` to the `docker` and `ssh` CLIs — both must be on PATH (locally or on the remote).
- SSH uses key-based auth with `StrictHostKeyChecking=no` and `BatchMode=yes`; commands fail silently on timeout rather than raising.
- Remote modes: local, custom VM, and Scaleway cloud provider.

## Code style

Match the surrounding code — there is no formatter config:

- `#pragma once` header guards.
- PascalCase classes (`DockerManager`), camelCase functions (`isContainerRunning()`), `m_` prefix for member variables.
- Qt signal/slot architecture with `Q_OBJECT`; manual memory management via `new` / `deleteLater()` (no smart pointers).
- Angle brackets for Qt/system includes, quotes for local includes.
- UI is page-based: game-specific pages (`MinecraftPage`, `CS2Page`, `GenericGamePage`) in a stacked widget; manager classes (e.g. `DockerManager`) encapsulate container/SSH operations.

## Adding features across game servers

When asked to add a feature for one game server, add it to **all** game servers (`MinecraftPage`, `CS2Page`, `GenericGamePage`, and any others), unless the option genuinely isn't available for a given server. Check each server's documentation every time to confirm whether and how the feature applies before implementing.

Configuration options must remain editable **after** installation, except for options that are inherently required at install time (e.g. IP, port, server name).

Every game page must show a banner at the top of its install form. Build it with the shared `buildGameBanner(icon, title, subtitle, colorStart, colorEnd, imagePath, container)` helper in `src/widgets/GameBanner.h` (a gradient header card), not with ad-hoc icon/title/subtitle labels. When adding a new game server, give its banner a gradient that matches the game's identity (`GenericGamePage` reuses `GamePageConfig::btnColorStart`/`btnColorEnd`).

Each banner should show an image of the game: pass an `imagePath` (e.g. `":/games/minecraft.png"`) bundled via `resources/games.qrc`. Drop the artwork in `resources/games/` and add a `<file alias="...">` entry to the `.qrc`. If the image is missing or fails to load, the banner falls back to the emoji `icon`. For `GenericGamePage`, set `GamePageConfig::bannerImage` (it defaults to `":/games/generic.png"`).

Every game server must expose a **Logs** tab in its running-server view. This is provided automatically by the shared `ServerDashboard` (the `GameLogsWidget` tab backed by `DockerManager::containerLogs`), so any new game page that uses `ServerDashboard` gets it for free — do not remove it, and if a new server type bypasses `ServerDashboard`, replicate the Logs tab there.

## Git workflow

Branch off `main` and open a PR for review before merging — do not commit directly to `main`.

For each new feature, create a dedicated branch, commit the changes on it, and merge it into `dev` once the feature is finished.
