# ECHO7

**ECHO7** is a 3D sci-fi puzzle game developed with **Unreal Engine 5.8**, using **C++** and **Blueprint**.  
Players explore a damaged space station and complete three system-recovery challenges before returning to the central control room.

## Features

- Contextual interaction system using **Line Trace** and the `E` key
- Native HUD/UI built with **UMG + C++**
- Challenge progress system: **0/3 → 3/3**
- Dynamic doors, terminals, generators, lighting and warning systems
- Speedrun-style completion timer
- Glitch, story overlay and ending effects
- Full playable flow from start to ending

## Challenges

### 1. Security Verification
Interact with the security terminal and complete a 3-question quiz.

### 2. Frequency Synchronization
Restore the station by synchronizing multiple frequency generators and unlocking connected areas.

### 3. Movement Scan
Move only when permitted and remain completely still while the AI scanning system is active.

After clearing the scan corridor, restore the corrupted system data through a **3×3 drag-and-drop image reconstruction puzzle**.

## Controls

| Key | Action |
|---|---|
| `W A S D` | Move |
| `Mouse` | Look |
| `E` | Interact |
| `ESC` | Close supported UI / puzzle |
| `Enter` | Restart after the final ending, if enabled |

## Technology

- Unreal Engine 5.8
- C++
- Blueprint
- UMG
- Native Unreal interaction and gameplay systems
- Git + Git LFS

## Project Structure

```text
ECHO7/
├── Config/
├── Content/
├── Source/
│   └── ECHO7/
│       ├── Game/
│       ├── UI/
│       ├── Interaction/
│       ├── Environment/
│       └── Power/
└── ECHO7.uproject
```

## Run the Project

1. Install **Unreal Engine 5.8**.
2. Clone the repository.
3. Pull Git LFS assets:

```bash
git lfs install
git lfs pull
```

4. Open `ECHO7.uproject`.
5. If Unreal requests a C++ rebuild, allow it to compile the project.
6. Open the main gameplay map and press **Play**.

## Build

For a normal editor build:

```text
ECHO7Editor Win64 Development
```

For sharing a playable Windows build, package the project through Unreal Engine using the **Shipping** configuration.

## Notes

- Generated folders such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` are intentionally excluded from Git.
- Large Unreal assets are managed with **Git LFS**.
- Some environment assets may originate from third-party/Fab packages and remain subject to their original licenses.

## Author

**NAMM BINZ**

> MỘT SẢN PHẨM CỦA NAMM BINZ
