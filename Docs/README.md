<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=for-the-badge&logo=xbox&logoColor=white)
![FMOD](https://img.shields.io/badge/FMOD-000000?style=for-the-badge&logo=fmod&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
[![Apache 2.0 License][license-shield]][license-url]

<!-- PROJECT TITLE -->
<div align="center">
  <h1>Daemon Libra</h1>
  <p>A semi-procedural 2D top-down tank shooter with heat-map pathfinding and data-driven design</p>
</div>

<!-- TODO: Replace with actual gameplay GIF or screenshot -->
<!-- <div align="center">
  <img src="docs/images/demo.gif" alt="Daemon Libra Gameplay" width="720">
</div> -->

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [How to Install](#how-to-install)
- [How to Use](#how-to-use)
- [Project Structure](#project-structure)
- [License](#license)
- [Contact](#contact)

## Overview

Daemon Libra is a 2D top-down tank shooter where the player navigates procedurally generated maps, fighting through waves of three distinct enemy types — Scorpio turrets, Leo mobile tanks, and Aries melee rushers. The game features dual-stick tank controls (body + turret), heat-map-based AI pathfinding, and a fully data-driven design powered by XML configuration files.

Built as a course project at SMU Guildhall, the game runs on a custom [Daemon Engine](https://github.com/dadavidtseng/Engine) providing DirectX 11 rendering, FMOD audio, and a developer console with V8 JavaScript integration.

## Features

- **Procedural map generation** — Worm algorithm carves organic terrain across 3 maps of increasing difficulty
- **Heat-map AI pathfinding** — Dijkstra-style distance fields drive enemy navigation with line-of-sight detection
- **Data-driven design** — XML files define all game parameters: entity stats, map layouts, tile properties, audio paths
- **Dual-stick tank controls** — Independent body (WASD) and turret (IJKL) rotation with Xbox controller support
- **Bouncing bullet physics** — Projectiles ricochet off walls up to 3 times with sound effects

## How to Install

### Prerequisites

- Visual Studio 2022 (or 2019) with C++ desktop development workload
- Windows 10/11 (x64)
- DirectX 11 compatible GPU
- [Daemon Engine](https://github.com/dadavidtseng/Engine) cloned as a sibling directory

### Build

```bash
# Clone both repos side by side
git clone https://github.com/dadavidtseng/Engine.git
git clone https://github.com/dadavidtseng/DaemonLibra.git

# Directory layout should be:
# ├── Engine/
# └── DaemonLibra/
```

1. Open `Libra.sln` in Visual Studio
2. Set configuration to `Debug | x64`
3. Build the solution (the Engine project is referenced automatically)
4. The executable is deployed to `Run/` via post-build event

## How to Use

### Controls

| Action | Keyboard | Controller |
|--------|----------|------------|
| Move body | W / A / S / D | Left Trigger |
| Aim turret | I / J / K / L | Right Trigger |
| Shoot | Enter | — |
| Start / Pause | P | Start |
| Quit | ESC | Back |

### Debug Controls

| Action | Key |
|--------|-----|
| Debug rendering | F1 |
| Noclip | F3 |
| Full map camera | F4 |
| Heat map visualization | F6 |
| Hard restart | F8 |
| Skip to next map | F9 |
| Slow-mo (0.1×) / Fast-mo (4×) | T / Y |
| Frame step | O |
| Developer console | ` |

### Game Flow

1. **Attract Mode** — Title screen
2. **Map 1** (24×30) — 60% Scorpio, 20% Leo, 20% Aries
3. **Map 2** (50×20) — Balanced enemy distribution
4. **Map 3** (16×16) — Final arena, sparse but harder
5. **Victory** — Complete all 3 maps

### Running

Launch `Run/Libra_Debug_x64.exe` from the `Run/` directory (working directory must be `Run/` for asset loading).

## Project Structure

```
DaemonLibra/
├── Code/Game/                  # Game source (17 classes)
│   ├── Main_Windows.cpp        # WinMain entry point
│   ├── App.cpp/hpp             # Application lifecycle
│   ├── Game.cpp/hpp            # Core game logic, state machine
│   ├── Entity.cpp/hpp          # Base entity with AI behaviors
│   ├── PlayerTank.cpp/hpp      # Player tank (100 HP, dual-stick)
│   ├── Scorpio.cpp/hpp         # Stationary turret enemy (5 HP)
│   ├── Leo.cpp/hpp             # Mobile tank enemy (3 HP)
│   ├── Aries.cpp/hpp           # Melee rusher enemy (8 HP)
│   ├── Bullet.cpp/hpp          # Bouncing projectile system
│   ├── Map.cpp/hpp             # Procedural generation, pathfinding
│   ├── MapDefinition.cpp/hpp   # Data-driven map config
│   ├── Tile.cpp/hpp            # Tile-based world
│   └── TileDefinition.cpp/hpp  # Tile type definitions
├── Run/                        # Runtime directory
│   ├── Data/Audios/            # 15 sound files (MP3)
│   ├── Data/Images/            # 36 textures (sprite sheets)
│   ├── Data/Fonts/             # Bitmap fonts (PNG)
│   ├── Data/Shaders/           # HLSL shaders
│   └── Data/Definitions/       # XML config (GameConfig, Maps, Tiles)
├── Docs/                       # Documentation
└── Libra.sln                   # Visual Studio solution
```

## License

Copyright 2025 Yu-Wei Tseng

Licensed under the [Apache License, Version 2.0](../LICENSE).

## Contact

**Yu-Wei Tseng**
- Portfolio: [dadavidtseng.info](https://dadavidtseng.info)
- GitHub: [@dadavidtseng](https://github.com/dadavidtseng)
- LinkedIn: [dadavidtseng](https://www.linkedin.com/in/dadavidtseng/)
- Email: dadavidtseng@gmail.com

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- REFERENCE-STYLE LINKS -->
[license-shield]: https://img.shields.io/github/license/dadavidtseng/DaemonLibra.svg?style=for-the-badge
[license-url]: https://github.com/dadavidtseng/DaemonLibra/blob/main/LICENSE
