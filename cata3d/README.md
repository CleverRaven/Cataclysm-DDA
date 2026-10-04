# Cataclysm-DDA-FP / Cata3D live bridge

This branch adds the first live 2.5D first-person client bridge.

## Architecture

CDDA remains authoritative for world generation, map/terrain/furniture, monsters/NPCs, items, vehicles, weather, visibility, simulation, combat, actions, turns and saves.

Cata3D is only a presentation + input client.

    CDDA simulation
          |
          | localhost UDP snapshots
          v
       Cata3D
          |
          | localhost UDP actions
          v
    normal CDDA action pipeline

## Protocol

Command port: 127.0.0.1:7777
State port:   127.0.0.1:7778

Cata3D sends REQUEST_STATE to get an immediate snapshot.

Commands currently supported:
MOVE_FWD, MOVE_BACK, MOVE_LEFT, MOVE_RIGHT, MOVE_FWD_LEFT, MOVE_FWD_RIGHT, MOVE_BACK_LEFT, MOVE_BACK_RIGHT, OPEN, CLOSE, SMASH, PICKUP, PICKUP_ALL, INTERACT, INVENTORY, FIRE, WAIT, MOVE_UP, MOVE_DOWN

Snapshots contain CDDA turn, player position, terrain ID, furniture ID, impassability/outdoor flags, ground item type IDs, and live monster type IDs/positions/HP.

Snapshot coordinates are relative to the player, so the Cata3D camera remains at local origin.

## Build

Build the normal tiles target from this fork. src/CMakeLists.txt already globs src/*.cpp, so the bridge sources are included automatically.

Then open cata3d/project.godot in Godot 4.7.2 and run it.

## Current limitations

- UDP snapshots are intentionally small and limited to a local radius.
- Rendering uses placeholder materials; CDDA asset/tileset extraction is the next layer.
- Interaction currently uses normal CDDA actions; directional targeting comes next.
- No interpolation/network prediction yet. CDDA remains turn-based and authoritative.