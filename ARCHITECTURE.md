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
9. [Networking Plan](#networking-plan)
10. [Code Conventions](#code-conventions)
11. [How to Add 3D Mode](#how-to-add-3d-mode)
12. [Branch & Commit Convention](#branch--commit-convention)

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
│       ├── CFRBoardState.h            # USTRUCT — board data
│       ├── CFRGameRules.h/.cpp        # UINTERFACE — rule contract
│       ├── CFRGameRulesBase.h/.cpp    # Abstract UObject — shared logic
│       ├── CFRGameRules2D.h/.cpp      # Concrete 2D rule set
│       ├── CFRGameMode.h/.cpp         # AGameModeBase — session controller
│       └── CFRColumnActor.h/.cpp      # AActor — column click detection
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
  int32 SizeX, SizeY, SizeZ          Board dimensions (set by Rules, not hardcoded)
  ECFRGameMode GameMode               Mode2D or Mode3D
  TArray<ECFRCell> Cells              Flat 1D cell array
  int32 CurrentPlayer                 1 or 2
  int32 MoveCount                     Total moves this game
  int32 LastMoveX, LastMoveY, LastMoveZ   Position of last placed piece (-1 if none)
```

```
Key methods:
  Init(Mode, SizeX, SizeY, SizeZ)    Reset board — called by CreateInitialBoard() only
  GetCell(X, Y, Z)                   Safe read — returns Empty if out of bounds
  SetCell(X, Y, Z, Value)            Safe write — no-op if out of bounds
  IsInBounds(X, Y, Z)                Bounds check for all three axes
  GetDropRow(X, Z)                   Gravity — lowest empty Y in column, or -1 if full
```

The out-of-bounds safety in `GetCell` / `SetCell` is intentional — it allows the win-detection
scanner to walk past board edges without explicit boundary checks in the loop.

---

### ICFRGameRules
**File:** `CFRGameRules.h`  
**Type:** `UINTERFACE`

Pure interface. No logic lives here — only method signatures.
Any system that needs to call game rules holds an `ICFRGameRules*` pointer,
never a concrete type. This makes rule-set swapping transparent.

```
Pure virtual methods:
  GetGameMode()                      Returns Mode2D or Mode3D
  CreateInitialBoard()               Fresh board, dimensions from the rule set
  IsLegalDrop(Board, X, Z)           In-bounds + column not full
  GetLegalMoves(Board)               All legal (X,Z) pairs as TArray<FIntPoint>
  ApplyDrop(Board, X, Z, OutBoard)   Copy board, drop piece, advance turn — original unchanged
  CheckResult(Board)                 Ongoing / Player1Wins / Player2Wins / Draw
  HasPlayerWon(Board, Player)        Checks all directions for a run of 4
```

---

### UCFRGameRulesBase
**File:** `CFRGameRulesBase.h / .cpp`  
**Type:** `UCLASS(Abstract)` — inherits `UObject`, implements `ICFRGameRules`

Implements every method that is **identical between 2D and 3D**.
Subclasses only override `GetGameMode()` and `HasPlayerWon()`.

```
Configurable UPROPERTY fields (set in Editor or Blueprint):
  int32 BoardSizeX = 7
  int32 BoardSizeY = 6
  int32 BoardSizeZ = 1
```

```
Shared implementations:
  CreateInitialBoard()    Calls Board.Init() with the configured dimensions
  IsLegalDrop()          IsInBounds check + GetDropRow != -1
  GetLegalMoves()        Scans all (X,Z), collects legal positions
  ApplyDrop()            Deep copy, gravity, record LastMove, advance turn
  CheckResult()          Only checks the last mover for win; then checks draw
```

```
Protected static helper:
  CheckConsecutive(Board, Player, Directions, ConnectN)
    — Bidirectional scan from Board.LastMove in each direction
    — Subclasses pass their direction array; loop body written once here
    — See Win Detection Algorithm section for details
```

---

### UCFRGameRules2D
**File:** `CFRGameRules2D.h / .cpp`  
**Type:** `UCLASS` — inherits `UCFRGameRulesBase`

Only two overrides needed:

```cpp
ECFRGameMode GetGameMode() const override { return ECFRGameMode::Mode2D; }

bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const override
{
    static const FIntVector Directions[] = {
        { 1,  0, 0 },  // horizontal
        { 0,  1, 0 },  // vertical
        { 1,  1, 0 },  // diagonal up-right
        { 1, -1, 0 },  // diagonal down-right
    };
    return CheckConsecutive(Board, Player, Directions);
}
```

Everything else is inherited from `UCFRGameRulesBase` unchanged.

---

### ACFRGameMode
**File:** `CFRGameMode.h / .cpp`  
**Type:** `UCLASS` — inherits `AGameModeBase`

Server-authoritative session controller. In a networked game, this class only exists on the server.

```
Fields:
  UCFRGameRules2D* Rules         Created via NewObject at BeginPlay
  FCFRBoardState CurrentBoard    Updated after every accepted move
  float CellSize = 100.f         World-unit size of one cell (configurable)
  FVector BoardOrigin            World position of cell (0,0,0) (configurable)
```

```
Methods:
  BeginPlay()                    Creates Rules + initial board
  TryDrop(X, Z)                  Validates, applies move, fires events
  GetCellWorldPosition(X, Y, Z)  Board coords → Unreal world space

Blueprint events (implement in BP_CFRGameMode):
  OnPiecePlaced(X, Y, Z, Player) Spawn mesh at GetCellWorldPosition(X, Y, Z)
  OnGameEnded(Result)            Show win/draw UI
```

**World coordinate mapping:**

```
Board X (column) → World Y  (left / right)
Board Y (row)    → World Z  (up / down, gravity)
Board Z (depth)  → World X  (always 0 in 2D)
```

**Important:** Capture `CurrentPlayer` before calling `ApplyDrop` — the turn advances inside that call.

---

### ACFRColumnActor
**File:** `CFRColumnActor.h / .cpp`  
**Type:** `UCLASS` — inherits `AActor`

Invisible tall box placed above each column in the level.
On click → casts to `ACFRGameMode` → calls `TryDrop(ColumnIndex, DepthIndex)`.

```
Fields:
  int32 ColumnIndex = 0    Board X index — set per instance in Editor
  int32 DepthIndex  = 0    Board Z index — always 0 in 2D
  UBoxComponent* HitBox    Root component, captures mouse clicks
```

In 2D mode: place 7 instances with `ColumnIndex = 0..6`.
Box extent defaults to `(40, 40, 300)` — covers full column height.

Click detection requires the Player Controller to have `bEnableClickEvents = true`.
Set this in the Game Mode Blueprint or in a custom PlayerController class.

---

## Board Coordinate System

```
index = X + (Y * SizeX) + (Z * SizeX * SizeY)

  X — column  (0 = left,   SizeX-1 = right)
  Y — row     (0 = bottom, SizeY-1 = top)     ← gravity fills from 0 upward
  Z — depth   (0 = front,  SizeZ-1 = back)    ← always 0 in 2D mode

2D default:  SizeX=7  SizeY=6  SizeZ=1   →  42 cells
3D default:  SizeX=4  SizeY=4  SizeZ=4   →  64 cells
```

GetCell returns `ECFRCell::Empty` for any out-of-bounds coordinate.
This is load-bearing — the win scanner relies on it to terminate naturally at board edges.

---

## Rules System Design

```
ICFRGameRules              (interface — pure contract)
        │
UCFRGameRulesBase          (abstract — shared logic)
        ├── UCFRGameRules2D    (4 directions)
        └── UCFRGameRules3D    (13 directions — future)
```

**Why interface + abstract base, not just a base class?**

- Any system holds `ICFRGameRules*` — never a concrete type
- Runtime mode switching = swap the pointer, nothing else changes
- `UCFRGameRulesBase` removes all code duplication between modes
- Adding a new mode requires only one new class

**Why ApplyDrop returns a new board instead of mutating?**

MCTS explores thousands of branches simultaneously from the same root state:

```
Root ─┬─ column 0 ─ column 0 ─ ...
      ├─ column 1 ─ column 3 ─ ...
      └─ column 4 ─ column 1 ─ ...
```

If ApplyDrop mutated the board in place, branches would corrupt each other.
Returning `OutBoard` keeps every branch fully independent.

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

### Why Bidirectional

Walking only `+Dir` misses wins completed from the middle:

```
Board:  ■ ■ _ ■    Player drops at position 2:    ■ ■ X ■
+Dir only: sees 1 piece to the right → Count = 2 → miss
Bidirectional: 2 left + 1 right + 1 center = 4 → WIN ✓
```

### Why Only Check Last Mover in CheckResult

A win requires placing a piece. The player who did not just move placed nothing new this turn,
so they cannot have formed a new winning run. `CheckResult` identifies the last mover as
`3 - CurrentPlayer` (since `ApplyDrop` already advanced the turn) and checks only them.

---

## Move Execution Flow

### Local (current)

```
ACFRColumnActor::OnHitBoxClicked
    └─ ACFRGameMode::TryDrop(ColumnIndex, 0)
           ├─ Rules->IsLegalDrop()           reject if illegal
           ├─ Rules->ApplyDrop()             deep copy, gravity, LastMove, advance turn
           ├─ CurrentBoard = NextBoard
           ├─ OnPiecePlaced(X, Y, Z, Player) BP event → spawn mesh
           ├─ Rules->CheckResult()
           └─ OnGameEnded(Result)            BP event (only if terminal)
```

### Networked (planned)

```
Client: ACFRColumnActor click
    └─ Server RPC: TryDrop_Server(X, Z)
           └─ ACFRGameMode::TryDrop()        server-side only
                  └─ ACFRGameState::Board updated + marked dirty
                         └─ Replication → all clients
                                └─ OnRep_Board() → each client spawns mesh
```

---

## Networking Plan

Classes to add:

| Class | Type | Role |
|-------|------|------|
| `ACFRGameState` | `AGameState` | Holds `FCFRBoardState` with `Replicated` tag; replicates board to all clients |
| `ACFRPlayerController` | `APlayerController` | Issues Server RPCs on player click; handles local input only |

Changes to existing classes:

| Class | Change |
|-------|--------|
| `ACFRColumnActor` | Replace direct `TryDrop` call with `Server RPC` via PlayerController |
| `ACFRGameMode` | Add authority guard (`HasAuthority()`) to `TryDrop` |
| Blueprint visual layer | Move mesh spawn from `OnPiecePlaced` event to `OnRep_Board` callback |

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
 * @brief Returns true if dropping at column (X, Z) is legal.
 * @param Board  Current board state.
 * @param X      Column index.
 * @param Z      Depth index; always 0 in 2D mode.
 */
virtual bool IsLegalDrop(const FCFRBoardState& Board, int32 X, int32 Z) const override;

// .cpp — minimal inline comment only where needed
bool UCFRGameRulesBase::IsLegalDrop(const FCFRBoardState& Board, int32 X, int32 Z) const
{
    if (!Board.IsInBounds(X, 0, Z)) { return false; }
    return Board.GetDropRow(X, Z) != -1;  // -1 means column is full
}
```

### No Hardcoded Values

Board dimensions must never be hardcoded. They are owned by `UCFRGameRulesBase` as
`UPROPERTY` fields (`BoardSizeX`, `BoardSizeY`, `BoardSizeZ`) and passed to `FCFRBoardState::Init()`.

---

## How to Add 3D Mode

1. `Tools → New C++ Class`, parent = `None`, name = `CFRGameRules3D`
2. Change parent in `.h` to `UCFRGameRulesBase`, add correct include
3. Implement two methods only:

```cpp
ECFRGameMode GetGameMode() const override { return ECFRGameMode::Mode3D; }

bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const override
{
    static const FIntVector Directions[] =
    {
        // Face-to-face axes
        { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
        // Face diagonals
        { 1, 1, 0 }, { 1,-1, 0 },
        { 1, 0, 1 }, { 1, 0,-1 },
        { 0, 1, 1 }, { 0, 1,-1 },
        // Space diagonals
        { 1, 1, 1 }, { 1, 1,-1 },
        { 1,-1, 1 }, { 1,-1,-1 },
    };
    return CheckConsecutive(Board, Player, Directions);
}
```

4. In `ACFRGameMode`, swap `UCFRGameRules2D` for `UCFRGameRules3D` (or expose as a selectable UPROPERTY)
5. In the level, place 16 `ACFRColumnActor` instances (4×4 grid, ColumnIndex 0–3, DepthIndex 0–3)

No other class changes required.

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
fix: LastMoveY not recorded when column is full
docs: add networking plan to ARCHITECTURE.md
```
