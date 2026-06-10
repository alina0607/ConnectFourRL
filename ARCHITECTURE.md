# ConnectFourRL — Architecture & Engineering Reference

Internal document for contributors. Covers project structure, class responsibilities, design decisions, conventions, and how to extend the codebase.

---

## Table of Contents

1. [Environment Setup](#environment-setup)
2. [Project Structure](#project-structure)
3. [Creating C++ Classes](#creating-c-classes)
4. [Class Reference](#class-reference)
5. [Board Coordinate System](#board-coordinate-system)
6. [Rules System Design](#rules-system-design)
7. [Win Detection Algorithm](#win-detection-algorithm)
8. [Move Execution Flow](#move-execution-flow)
9. [Player Input System](#player-input-system)
10. [Piece Spawning & Animation](#piece-spawning--animation)
11. [Debug Grid System](#debug-grid-system)
12. [Networking Plan](#networking-plan)
13. [Code Conventions](#code-conventions)
14. [Branch & Commit Convention](#branch--commit-convention)

---

## Environment Setup

| Tool | Version |
|------|---------|
| Unreal Engine | 5.7 |
| Visual Studio | 2022 Community v17.14 |
| JetBrains Rider | 2026.1 (primary IDE — zero false IntelliSense errors) |
| Git GUI | GitHub Desktop |

```bash
git clone https://github.com/alina0607/ConnectFourRL.git
git checkout dev

# Open UE5 project
Game/ConnectFourRL/ConnectFourRL.uproject

# Rebuild modules when prompted (always required after first clone)
# Open solution in Rider: File → Open → ConnectFourRL.sln
```

---

## Project Structure

```
ConnectFourRL/
│
├── README.md                          # Public showcase
├── ARCHITECTURE.md                    # This file
│
├── Game/ConnectFourRL/
│   └── Source/ConnectFourRL/          # ALL C++ files live here (flat — no subfolders)
│       ├── CFRBoardState.h            # USTRUCT — board data and cell index logic
│       ├── CFRGameRules.h/.cpp        # UINTERFACE — rule contract
│       ├── CFRGameRulesBase.h/.cpp    # Abstract UObject — shared logic
│       ├── CFRGameRules2D.h/.cpp      # Concrete 2D rule set (4 directions)
│       ├── CFRGameRules3D.h/.cpp      # Concrete 3D rule set (13 directions)
│       ├── CFRGameMode.h/.cpp         # AGameModeBase — session controller
│       ├── CFRPlayerController.h/.cpp # APlayerController — mouse input via ray trace
│       ├── CFRPiece.h/.cpp            # AActor — falling piece with animation
│       ├── CFRGameState.h/.cpp        # AGameStateBase — shell for networking
│       ├── CFRGameRecorder.h/.cpp     # UObject — shell for AI data recording
│       └── CFRColumnActor.h/.cpp      # AActor — superseded by PlayerController ray trace
│
└── AI/                                # Python pipeline (not yet implemented)
    ├── data/games/                    # Raw JSON game records
    ├── data/processed/                # NumPy tensors
    ├── models/checkpoints/            # PyTorch .pt files
    ├── models/exported/               # ONNX files for UE5
    ├── env/                           # Python board environment
    ├── mcts/                          # Monte Carlo Tree Search
    ├── training/                      # Training scripts
    └── selfplay/                      # Self-play pipeline
```

> **Flat file structure is mandatory.** Do not create subfolders inside `Source/ConnectFourRL/`.
> UE5's Unreal Header Tool (UHT) and include paths rely on all files being at the same level.

---

## Creating C++ Classes

**Always** use `Tools → New C++ Class` inside the UE5 editor.
Never create `.h` / `.cpp` files manually — UHT will not generate the required `.generated.h` file.

Steps:
1. `Tools → New C++ Class`
2. Select parent class from the list (or `None` if the parent is a custom class)
3. Name the class (prefix `CFR`, no `U`/`A` — UE5 adds that automatically)
4. Click **Create Class**
5. After the wizard finishes, replace the file contents as needed

If you selected the wrong parent:
- Change the parent in the `.h` file manually
- Add the correct `#include` for the real parent
- Rebuild

---

## Class Reference

### FCFRBoardState
**File:** `CFRBoardState.h`
**Type:** `USTRUCT(BlueprintType)`

Complete snapshot of the board at any moment. Designed for **value semantics** — copying is safe and cheap enough for MCTS branching.

```
Fields:
  int32 SizeX, SizeY, SizeZ            Board dimensions (set by Rules, not hardcoded)
  ECFRGameMode GameMode                 Mode2D or Mode3D
  TArray<ECFRCell> Cells                Flat 1D cell array
  int32 CurrentPlayer                   1 or 2
  int32 MoveCount                       Total moves this game
  int32 LastMoveX, LastMoveY, LastMoveZ Position of last placed piece (-1 if none)
```

```
Key methods:
  Init(Mode, SizeX, SizeY, SizeZ)      Reset board — called by CreateInitialBoard() only
  GetCell(X, Y, Z)                     Safe read — returns Empty if out of bounds
  SetCell(X, Y, Z, Value)              Safe write — no-op if out of bounds
  IsInBounds(X, Y, Z)                  Bounds check for all three axes
  GetDropRow(X, Z)                     Gravity — lowest empty Y in column, or -1 if full
```

Cell index formula:
```
index = X + (Y * SizeX) + (Z * SizeX * SizeY)
```

The out-of-bounds safety in `GetCell` / `SetCell` is intentional — it allows the win-detection
scanner to walk past board edges without explicit boundary checks in the loop.

---

### ICFRGameRules
**File:** `CFRGameRules.h`
**Type:** `UINTERFACE`

Pure interface. No logic lives here — only method signatures.
Any system that needs to call game rules holds an `ICFRGameRules*` pointer,
never a concrete type.

```
Pure virtual methods:
  GetGameMode()                        Returns Mode2D or Mode3D
  CreateInitialBoard()                 Fresh board, dimensions from the rule set
  IsLegalDrop(Board, X, Z)            In-bounds + column not full (3D gravity)
  GetLegalMoves(Board)                 All legal (X,Z) pairs as TArray<FIntPoint>
  ApplyDrop(Board, X, Z, OutBoard)    Copy board, gravity drop, advance turn
  CheckResult(Board)                   Ongoing / Player1Wins / Player2Wins / Draw
  HasPlayerWon(Board, Player)          Checks all directions for a run of 4
```

---

### UCFRGameRulesBase
**File:** `CFRGameRulesBase.h / .cpp`
**Type:** `UCLASS(Abstract)` — inherits `UObject`, implements `ICFRGameRules`

Implements every method that is identical between 2D and 3D.
Subclasses only override `GetGameMode()` and `HasPlayerWon()`.

```
Configurable UPROPERTY fields:
  int32 BoardSizeX = 7    (overridden to 4 in UCFRGameRules3D)
  int32 BoardSizeY = 6    (overridden to 4 in UCFRGameRules3D)
  int32 BoardSizeZ = 1    (overridden to 4 in UCFRGameRules3D)
```

```
Shared implementations:
  CreateInitialBoard()      Calls Board.Init() with the configured dimensions
  IsLegalDrop(X, Z)        IsInBounds + GetDropRow != -1 (gravity-based)
  IsLegalPlace(X, Y, Z)    IsInBounds + cell is empty (free placement, 2D)
  GetLegalMoves()           Scans all (X,Z), collects legal drop positions
  ApplyDrop(X, Z)           Deep copy, gravity, record LastMove, advance turn
  ApplyPlace(X, Y, Z)       Deep copy, direct placement, record LastMove, advance turn
  CheckResult()             Only checks last mover for win, then checks draw
```

```
Protected static helper:
  CheckConsecutive(Board, Player, Directions, ConnectN)
    — Bidirectional scan from Board.LastMove in each direction
    — Subclasses pass their direction array; loop body written once here
```

---

### UCFRGameRules2D
**File:** `CFRGameRules2D.h / .cpp`
**Type:** `UCLASS` — inherits `UCFRGameRulesBase`

Board size: 7 × 6 × 1. Player freely places a piece on any empty cell.

```cpp
ECFRGameMode GetGameMode() const override { return ECFRGameMode::Mode2D; }

bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const override
{
    static const FIntVector Directions[] = {
        { 1,  0, 0 },  // horizontal  (X axis)
        { 0,  1, 0 },  // vertical    (Y axis)
        { 1,  1, 0 },  // diagonal ↗
        { 1, -1, 0 },  // diagonal ↘
    };
    return CheckConsecutive(Board, Player, Directions);
}
```

---

### UCFRGameRules3D
**File:** `CFRGameRules3D.h / .cpp`
**Type:** `UCLASS` — inherits `UCFRGameRulesBase`

Board size: 4 × 4 × 4. Player selects a (X, Z) column; gravity places at lowest empty Y.

```cpp
UCFRGameRules3D::UCFRGameRules3D()
{
    BoardSizeX = 4;
    BoardSizeY = 4;
    BoardSizeZ = 4;
}
```

13 win directions cover every possible axis in a 3D grid:

| Group | Directions |
|-------|-----------|
| Face-to-face axes (3) | +X, +Y, +Z |
| Face diagonals — XY plane (2) | (1,1,0), (1,-1,0) |
| Face diagonals — XZ plane (2) | (1,0,1), (1,0,-1) |
| Face diagonals — YZ plane (2) | (0,1,1), (0,1,-1) |
| Space diagonals (4) | (1,1,1), (1,1,-1), (1,-1,1), (1,-1,-1) |

---

### ACFRGameMode
**File:** `CFRGameMode.h / .cpp`
**Type:** `UCLASS` — inherits `AGameModeBase`

Server-authoritative session controller.
Owns the active rule set and the live board state.
Spawns piece actors directly in C++ (no Blueprint event delegation).

```
Key UPROPERTY fields:
  UCFRGameRulesBase* Rules              Active rule set (2D or 3D)
  FCFRBoardState CurrentBoard           Live board updated after every move

  float CellSize       = 150.f          Horizontal cell spacing in world units (X/Y)
  float CellHeight     = 50.f           Vertical stacking step in 3D mode (World Z)
  float CellGap        = 10.f           Visual gap between debug grid boxes
  FVector BoardOrigin  = (0,0,0)        World position of board cell (0,0,0)

  TSubclassOf<ACFRPiece> PieceActorClassP1   Set to BP_CFRPiece_P1 in editor
  TSubclassOf<ACFRPiece> PieceActorClassP2   Set to BP_CFRPiece_P2 in editor
  float DropStartHeight = 800.f         Spawn height above ground (World Z offset)
  float PieceFallSpeed  = 600.f         Fall speed in units/second

  bool bDrawDebugGrid = false            Enable debug cell overlay in PIE

  int32 HoveredColumn = -1              Cell under cursor (X), -1 = none
  int32 HoveredRow    = 0               Cell under cursor (Y), 2D only
  int32 HoveredDepth  = 0               Cell under cursor (Z), 3D only
```

```
Key methods:
  BeginPlay()                Creates Rules (2D by default) + initial board
  TryDrop(X, Z)              3D: gravity drop into column (X,Z); spawns ACFRPiece
  TryPlace(X, Y, Z)          2D: direct placement at cell (X,Y,0); spawns ACFRPiece
  SwitchGameMode()           Destroys all pieces, toggles 2D↔3D, resets board
  GetCellWorldPosition(X,Y,Z) Board coords → world space (mode-aware, see below)
  SetHoveredCell(Col,Row,Depth) Called every frame by PlayerController
  DrawDebugGrid()            Internal — draws cell overlay when bDrawDebugGrid = true

Blueprint events (implement in BP_CFRGameMode):
  OnGameEnded(Result)        Show win/draw UI
```

---

### ACFRPlayerController
**File:** `CFRPlayerController.h / .cpp`
**Type:** `UCLASS` — inherits `APlayerController`

Handles all local player input. Uses **ray-plane intersection** so column detection
is accurate at any camera angle without requiring hit-box actors.

```
Setup:
  bShowMouseCursor  = true
  bEnableClickEvents = true
```

```
Per-frame Tick:
  1. TraceToBoard()        — ray-plane intersection → hovered cell
  2. Left click detected   → OnClickBoard()
  3. Space bar detected    → GameMode->SwitchGameMode()
```

**TraceToBoard — plane selection:**

| Mode | Intersection Plane | Extracts |
|------|-------------------|---------|
| 2D | Z = BoardOrigin.Z (flat ground) | Col (X), Row (Y) |
| 3D | Z = BoardOrigin.Z (flat ground) | Col (X), Depth (Y→BoardZ) |

Both modes intersect the horizontal ground plane because both boards have
their bottom face at World Z = BoardOrigin.Z.

**OnClickBoard:**

| Mode | Action |
|------|--------|
| 2D | `GameMode->TryPlace(HoveredColumn, HoveredRow, 0)` |
| 3D | `GameMode->TryDrop(HoveredColumn, HoveredDepth)` |

---

### ACFRPiece
**File:** `CFRPiece.h / .cpp`
**Type:** `UCLASS` — inherits `AActor`

Visually represents one placed piece. Falls from spawn height to target position.

```
Components:
  USceneComponent* SceneRoot       Root — isolates mesh pivot from spawn position
  UStaticMeshComponent* Mesh       Child — mesh and material set in Blueprint
```

```
Methods:
  StartFall(TargetLocation, FallSpeed)   Enables Tick, begins descent
  Tick(DeltaTime)                        VInterpConstantTo toward target; disables when arrived
```

Blueprint hierarchy:
```
ACFRPiece (C++)
  └── BP_CFRPiece          (parent Blueprint — base settings)
        ├── BP_CFRPiece_P1 (child — donut mesh A + material for Player 1)
        └── BP_CFRPiece_P2 (child — donut mesh B + material for Player 2)
```

Donut pivot offsets are corrected by adjusting the `Mesh` relative transform
inside each Blueprint child — no C++ changes needed.

---

### ACFRGameState
**File:** `CFRGameState.h / .cpp`
**Type:** `UCLASS` — inherits `AGameStateBase`

Shell class reserved for the networking phase.
Will hold a replicated `FCFRBoardState` when multiplayer is implemented.

---

### UCFRGameRecorder
**File:** `CFRGameRecorder.h / .cpp`
**Type:** `UCLASS` — inherits `UObject`

Shell class reserved for AI data collection.
Will serialize game records to JSON for Deep Learning training.

---

## Board Coordinate System

Board axes are shared by both modes:
```
X — column  (0 = left,   SizeX-1 = right)
Y — row     (0 = bottom, SizeY-1 = top)     ← gravity fills from 0 upward
Z — depth   (0 = front,  SizeZ-1 = back)    ← always 0 in 2D mode
```

**`GetCellWorldPosition(X, Y, Z)` is mode-aware:**

### 2D Mode (SizeZ == 1)

The board is a flat horizontal slab on the ground (World XY plane).
No vertical stacking — all pieces land at World Z = 0.

```
Board X (col, 0-6) → World X  (left / right)
Board Y (row, 0-5) → World Y  (front / back; gravity fills Y=0 first)
Board Z = 0        → World Z  = 0  (flat on ground)

Formula: BoardOrigin + FVector(X * CellSize, Y * CellSize, 0)
```

Piece animation: spawns at `(X*CellSize, Y*CellSize, BoardOrigin.Z + DropStartHeight)`
and falls straight down to `World Z = 0`.

### 3D Mode (SizeZ == 4)

The 4×4 base grid is flat on the ground; pieces stack upward along World Z.

```
Board X (col,   0-3) → World X  (left / right)
Board Z (depth, 0-3) → World Y  (front / back)
Board Y (row,   0-3) → World Z  (up; stacking, uses CellHeight not CellSize)

Formula: BoardOrigin + FVector(X * CellSize, Z * CellSize, Y * CellHeight)
```

`CellHeight` is **decoupled from `CellSize`** so the vertical stacking step
can be tuned to match the actual mesh height without affecting the horizontal grid.

Piece animation: spawns at `(X*CellSize, Z*CellSize, BoardOrigin.Z + DropStartHeight)`
and falls straight down to `World Z = Y * CellHeight`.

### Board Defaults

```
2D: SizeX=7  SizeY=6  SizeZ=1   →  42 cells
3D: SizeX=4  SizeY=4  SizeZ=4   →  64 cells
```

---

## Rules System Design

```
ICFRGameRules              (interface — pure contract)
        │
UCFRGameRulesBase          (abstract — shared logic)
        ├── UCFRGameRules2D    (4 directions, free placement)
        └── UCFRGameRules3D    (13 directions, gravity drop)
```

**Why interface + abstract base, not just a base class?**

- Any system holds `UCFRGameRulesBase*` — never a concrete type
- Runtime mode switching = swap the pointer, nothing else changes
- `UCFRGameRulesBase` removes all code duplication between modes
- Adding a new variant requires only one new subclass

**2D vs 3D placement mechanics:**

| | 2D | 3D |
|---|---|---|
| Player input | Click any empty (X, Y) cell | Click (X, Z) column base |
| Gravity | None — piece placed at exact (X, Y, 0) | Piece falls to lowest empty Y in (X, Z) |
| Legal check | `IsLegalPlace(X, Y, Z)` — cell empty? | `IsLegalDrop(X, Z)` — column not full? |
| Apply method | `ApplyPlace(X, Y, Z, OutBoard)` | `ApplyDrop(X, Z, OutBoard)` |

**Why ApplyDrop / ApplyPlace return a new board instead of mutating?**

MCTS explores thousands of branches simultaneously from the same root state.
Returning `OutBoard` keeps every branch fully independent without copies.

---

## Win Detection Algorithm

### Implementation

`CheckConsecutive` (protected static on `UCFRGameRulesBase`):

```
1. Guard: if LastMoveX == -1, return false (no move made yet)
2. For each direction in Directions[]:
   a. Walk +Dir from LastMove up to ConnectN-1 steps, count consecutive Target pieces
   b. Walk -Dir from LastMove up to ConnectN-1 steps, count consecutive Target pieces
   c. Total = forward_count + backward_count + 1 (center)
   d. If Total >= ConnectN → return true
3. return false
```

Complexity: **O(D × N)** where D = number of directions, N = ConnectN (4).
Naive full-board scan would be O(W × H × D × N × Z).

### Why Bidirectional

Walking only `+Dir` misses wins completed from the middle:

```
Board:  ■ ■ _ ■    Player drops at position 2:    ■ ■ X ■
+Dir only: sees 1 piece to the right → Count = 2 → MISS
Bidirectional: 2 left + 1 right + 1 center = 4 → WIN ✓
```

### Why Only Check Last Mover in CheckResult

A win requires placing a piece. The player who did not just move placed nothing new this turn,
so they cannot have formed a new winning run. `CheckResult` identifies the last mover as
`3 - CurrentPlayer` (since `ApplyDrop` / `ApplyPlace` already advanced the turn)
and checks only them.

---

## Move Execution Flow

### 2D Free Placement

```
ACFRPlayerController::Tick
  └─ TraceToBoard()                      Z-plane intersection → (Col, Row)
       └─ GameMode->SetHoveredCell()     update hover highlight

Left click detected:
  └─ OnClickBoard()
       └─ GameMode->TryPlace(Col, Row, 0)
              ├─ Rules->ApplyPlace()     IsLegalPlace check, deep copy, advance turn
              ├─ CurrentBoard = NextBoard
              ├─ GetCellWorldPosition(X, Y, 0)  → flat ground position
              ├─ SpawnActor<ACFRPiece>   spawn above, StartFall to ground
              └─ Rules->CheckResult()
                   └─ OnGameEnded()     BP event (only if terminal)
```

### 3D Gravity Drop

```
ACFRPlayerController::Tick
  └─ TraceToBoard()                      Z-plane intersection → (Col, Depth)
       └─ GameMode->SetHoveredCell()     update hover highlight

Left click detected:
  └─ OnClickBoard()
       └─ GameMode->TryDrop(Col, Depth)
              ├─ Rules->ApplyDrop()      IsLegalDrop check, gravity, deep copy, advance turn
              ├─ CurrentBoard = NextBoard
              ├─ GetCellWorldPosition(X, LastMoveY, Z)  → stacked position
              ├─ SpawnActor<ACFRPiece>   spawn above, StartFall to stack height
              └─ Rules->CheckResult()
                   └─ OnGameEnded()     BP event (only if terminal)
```

### Mode Switch (Space Bar)

```
ACFRPlayerController::Tick detects Space Bar
  └─ GameMode->SwitchGameMode()
         ├─ GetAllActorsOfClass(ACFRPiece) → Destroy() all pieces
         ├─ Toggle: NewObject<UCFRGameRules3D> or NewObject<UCFRGameRules2D>
         └─ CurrentBoard = Rules->CreateInitialBoard()
```

### Networked (planned)

```
Client: PlayerController click
  └─ Server RPC: TryDrop_Server(X, Z) or TryPlace_Server(X, Y, Z)
         └─ ACFRGameMode (server only)
                └─ ACFRGameState::Board updated + marked dirty
                       └─ Replication → all clients
                              └─ OnRep_Board() → each client spawns ACFRPiece
```

---

## Player Input System

Input is handled entirely through **ray-plane intersection** in `ACFRPlayerController::TraceToBoard`.
No hit-box actors (ACFRColumnActor) are used for input — the class exists but is superseded.

```
DeprojectMousePositionToWorld(RayOrigin, RayDirection)
  └─ Solve: T = (BoardOrigin.Z - RayOrigin.Z) / RayDirection.Z
       └─ HitPoint = RayOrigin + T * RayDirection
            ├─ Col   = floor((HitPoint.X - BoardOrigin.X) / CellSize)
            ├─ Row   = floor((HitPoint.Y - BoardOrigin.Y) / CellSize)  [2D only]
            └─ Depth = floor((HitPoint.Y - BoardOrigin.Y) / CellSize)  [3D only]
```

Degenerate cases guarded:
- `RayDirection.Z ≈ 0` → ray nearly parallel to ground → skip frame
- `T ≤ 0` → board is behind camera → skip frame

---

## Piece Spawning & Animation

Both `TryDrop` and `TryPlace` follow the same spawn pattern:

```cpp
// Spawn above the target, always at the same altitude regardless of stack height.
FVector SpawnLocation = FVector(
    TargetLocation.X,
    TargetLocation.Y,
    BoardOrigin.Z + DropStartHeight   // always the same absolute height
);

ACFRPiece* Piece = SpawnActor<ACFRPiece>(PieceClass, SpawnLocation, ...);
Piece->StartFall(TargetLocation, PieceFallSpeed);
```

`StartFall` enables `Tick`. Each tick:
```cpp
CurrentLocation = VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, FallSpeed);
if (FMath::IsNearlyEqual(CurrentLocation.Z, TargetLocation.Z, 0.5f)) { SetActorTickEnabled(false); }
```

**Important:** `ActivePlayer` must be captured **before** `ApplyDrop` / `ApplyPlace` —
those calls advance `CurrentPlayer` internally.

---

## Debug Grid System

Enabled by setting `bDrawDebugGrid = true` on `BP_CFRGameMode`.
Drawn every frame in `ACFRGameMode::Tick` → `DrawDebugGrid()`.

### 2D Debug Grid

- Draws all 7 × 6 = 42 cells as flat slabs in the World XY plane (World Z = 0)
- Box extent: `(VisualHalf, VisualHalf, SlabHalf)` — thin in Z
- Hovered cell `(HoveredColumn, HoveredRow)` → green (empty) or red (occupied)
- All other cells → cyan

### 3D Debug Grid

- Draws only the base layer (Board Y = 0): 4 × 4 = 16 cells
- Box extent: `(VisualHalf, VisualHalf, CellHeight * 0.5f)` — height matches donut mesh
- Hovered cell `(HoveredColumn, HoveredDepth)` → green (column has space) or red (full)
- All other cells → cyan

### Parameters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `CellSize` | 150 | Horizontal spacing between cell centres (X/Y) |
| `CellHeight` | 50 | Vertical stacking step in 3D; also debug box Z height |
| `CellGap` | 10 | Shrinks debug box relative to spacing — makes gap visible |

Visual box size = `CellSize - CellGap`. A gap of 10 on a 150-unit cell = 6.7% gap.
Set `CellGap` to 20–30 for clearly visible separation.

### DrawDebugString Lifetime

`DrawDebugString` with lifetime `-1.f` is **permanent** (never cleared).
All calls use `0.f` (single-frame draw) to prevent string accumulation across mode switches.
`DrawDebugBox` with `-1.f` correctly draws for one frame only — different behaviour.

---

## Networking Plan

Classes to implement:

| Class | Type | Role |
|-------|------|------|
| `ACFRGameState` | `AGameState` | Holds replicated `FCFRBoardState`; replicates board to all clients |

Changes to existing classes:

| Class | Change |
|-------|--------|
| `ACFRPlayerController` | Replace direct `TryDrop` / `TryPlace` with Server RPCs |
| `ACFRGameMode` | Add `HasAuthority()` guard to `TryDrop` / `TryPlace` |
| Blueprint layer | Move piece spawn from game mode to `OnRep_Board` callback on clients |

`FCFRBoardState` requires no changes — `TArray` replicates natively in UE5.

---

## Code Conventions

### Naming

| Item | Convention | Example |
|------|-----------|---------|
| UObject class | `U` prefix | `UCFRGameRulesBase` |
| Actor class | `A` prefix | `ACFRGameMode` |
| Struct | `F` prefix | `FCFRBoardState` |
| Interface | `I` prefix (native), `U` prefix (UObject) | `ICFRGameRules` |
| Enum | `E` prefix | `ECFRCell` |
| Project prefix | `CFR` | all classes |

### Comments

- All Doxygen comments go on **declarations** in `.h` files
- Inside `.cpp` function bodies: only short single-line `// reason` comments where non-obvious
- No Doxygen blocks inside `.cpp` bodies

```cpp
// .h — full Doxygen on declaration
/**
 * @brief Returns true if cell (X, Y, Z) is in bounds and currently empty.
 * @param Board  Current board state.
 * @param X      Column index.
 * @param Y      Row index.
 * @param Z      Depth index; always 0 in 2D mode.
 */
bool IsLegalPlace(const FCFRBoardState& Board, int32 X, int32 Y, int32 Z) const;

// .cpp — minimal inline comment only where needed
bool UCFRGameRulesBase::IsLegalPlace(const FCFRBoardState& Board, int32 X, int32 Y, int32 Z) const
{
    return Board.IsInBounds(X, Y, Z) && Board.GetCell(X, Y, Z) == ECFRCell::Empty;
}
```

### No Hardcoded Values

Board dimensions, cell spacing, fall speed, etc. must never be hardcoded.
All tunable values are owned as `UPROPERTY` fields so designers can adjust without recompiling.

---

## Branch & Commit Convention

| Branch | Purpose |
|--------|---------|
| `main` | Stable only — merge via PR, never commit directly |
| `dev` | Active development — all feature branches merge here first |
| `feature/<name>` | Individual features |

Commit message format:
```
<type>: <short description>

type:
  feat     — new feature
  fix      — bug fix
  refactor — restructure without behaviour change
  docs     — documentation only
  chore    — build, config, dependencies
```

Examples:
```
feat: implement incremental win detection with bidirectional scan
fix: DrawDebugString lifetime accumulation across mode switches
docs: update ARCHITECTURE.md coordinate system and class reference
```
