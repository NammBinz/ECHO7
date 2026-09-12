# ECHO7

ECHO7 is a 3D sci-fi puzzle game built with Unreal Engine 5.8 using C++ and Blueprint. The player explores a damaged research facility, restores its systems, and escapes while completing a timed sequence of interconnected challenges.

## Gameplay

The run progresses from `0/3` to `3/3` through three main challenges:

1. Complete a security verification quiz.
2. Synchronize a sequence of frequency generators.
3. Survive a movement-scanning corridor, then solve a final 3x3 drag-and-drop image reconstruction puzzle.

## Features

- Reusable contextual interaction system and `[E]` prompts
- Objective, challenge-progress, guidance, and story HUD
- Dynamic facility lighting and flashing scan-warning lights
- Sequential puzzle and door progression
- Speedrun timer with a frozen completion time
- Native C++ ending, fade, typewriter, and restart flow

## Controls

| Input | Action |
| --- | --- |
| `WASD` | Move |
| Mouse | Look |
| `E` | Interact |
| `Esc` | Close UI |

## Requirements and setup

- Unreal Engine 5.8
- Visual Studio 2022 with the Game development with C++ workload and a compatible Windows SDK
- Git LFS

After cloning, run `git lfs install`, open `ECHO7.uproject`, and allow Unreal Engine to generate project files if requested. Build the `ECHO7Editor` target for `Win64 Development`, then open the project in Unreal Editor.

## Third-party assets

The environment includes third-party Underground Sci-Fi assets acquired through Fab. Those assets remain subject to their original license. A private repository is recommended when preserving the complete project; for a public portfolio repository, verify redistribution rights and consider excluding licensed environment content while documenting how collaborators can reacquire it.

