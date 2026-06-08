# RCKTMN

A 2D tile-based survival and creative sandbox game built with raylib and C++17.

### [PLAY](https://geo0451.github.io/rcktmn/docs/)

## About

RCKTMN is a voxel-inspired sandbox where you can explore procedurally generated worlds, mine blocks, build structures, and switch between survival and creative modes. The game features dynamic world generation with caves and clouds, inventory management, and save/load functionality.
## Features

- Procedurally generated 2048x2048 tile worlds with terrain, caves, and clouds
- Survival mode: Mine blocks, manage inventory, track health with color-coded player
- Maker mode: Creative building with unlimited blocks and flight
- Save and load up to 5 world slots
- Dynamic camera with adjustable zoom (0.048x to 3.0x)
- Full-world visibility in maker mode
- Block color preservation in saves

## Building

Requirements:
- g++ (C++17)
- raylib 5.5
- Standard C libraries (pthreads, X11, GL)

Compile:
```bash
g++ -std=c++17 -O2 \
  rcktmn.cpp src/tilemap.cpp src/player.cpp src/maker.cpp src/gameplay.cpp \
  -o rcktmn \
  -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
```

Run:
```bash
./rcktmn
```

## Controls

### General
- **WASD**: Move left/right, down
- **W or Space**: Jump (survival mode) / Ascend (maker mode)
- **Scroll Wheel**: Zoom in/out
- **Ctrl+M**: Toggle between survival and maker mode
- **ESC**: Return to main menu or cancel action

### Survival Mode
- **LMB (hold)**: Mine blocks (hold to break)
- **RMB**: Place selected block (if you have any)
- **1-9**: Select inventory slot
- **TAB**: Open inventory grid

### Maker Mode
- **LMB (drag)**: Place blocks
- **RMB (drag)**: Erase blocks
- **1-3**: Quick select block type (Dirt, Stone, Erase)
- **B**: Open block selection menu
- **Ctrl+M**: Return to survival mode

### Menu/Save/Load
- **Ctrl+S**: Save world to slot
- **Ctrl+L**: Load world from slot
- **1-5**: Select save/load slot
- **Enter**: Start game from preview screen

## Game Modes

### Survival
Mine blocks to gather resources. Your health is indicated by your player color (black = 0 HP, red = full health). Switch between mining and placing modes to build. Access your inventory to manage collected blocks.

### Maker (Creative)
Build freely with unlimited blocks and the ability to fly. Use drag-to-place for efficient building or use the block menu for precise selection.

## Project Structure

```
rcktmn/
  rcktmn.cpp           - Main game loop and UI
  src/
    constants.h        - Game constants and settings
    ui.h               - Message system and console UI
    tilemap.h/.cpp     - World grid and terrain generation
    player.h/.cpp      - Player entity and inventory
    maker.h/.cpp       - Maker mode (creative building)
    gameplay.h/.cpp    - Collision, input, save/load
```

## Notes

- Clouds are generated using multi-puff clustering for a fluffy, organic appearance
- World generation uses Perlin noise for terrain and cellular automata for caves
- Save files are stored as plain text in the current directory (sf_1.dat, etc.)
- Health is purely visual in current version; no damage mechanics yet