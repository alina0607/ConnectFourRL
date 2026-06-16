# ConnectFourRL — Claude Handoff Document

> **This file is for the next Claude session.**
> Read this before touching any code. Every section exists because a bug already happened.

---

## 1. Project Goal

Build a Connect Four game in **Unreal Engine 5 C++** that:
- Supports both **2D** (7×6 flat board, free placement) and **3D** (4×4×4, gravity drop) modes, switchable at runtime (Space Bar)
- Has online multiplayer via **UE5 Dedicated Server** (not yet implemented)
- Records game data as **JSON** for a **Deep Learning / AlphaZero-style RL pipeline** (not yet implemented)
- All code comments in **professional Doxygen English**
- Communication with user in **Chinese**

---

## 2. Current Implementation State

### Done ✓

| Feature | Status |
|---------|--------|
| FCFRBoardState — flat 1D board, shared by all modes | ✓ |
| FCFRBoardState — action encoding (GetActionSpaceSize, EncodeAction) + board-level IsLegalDrop/IsLegalPlace | ✓ |
| UCFRGameRulesBase — shared logic (ApplyDrop, ApplyPlace, CheckResult) | ✓ |
| UCFRGameRules2D — 4-direction win, **free placement** (Gomoku-style) | ✓ |
| **UCFRGameRules2DClassic — 4-direction win, gravity column-drop (standard Connect Four)** | ✓ |
| UCFRGameRules3D — 13-direction win, gravity column-drop | ✓ |
| ACFRGameMode — board owner, TryDrop, TryPlace, 3-way SwitchGameMode, game-over guard | ✓ |
| ACFRPlayerController — ray-plane input, hover, Server RPC input, turn check | ✓ |
| ACFRPiece — falling animation, **replicated** (target/speed replicate; clients animate locally) | ✓ |
| **UCFRGameRecorder — JSON export of every game (MCTS-ready schema)** | ✓ |
| **ACFRGameState — replicated board + visual params + game-over (networking Phase 1)** | ✓ |
| **ACFRHoverIndicator — local per-client hover highlight (mesh + green/red material)** | ✓ |
| Networking Phase 1 — Listen-Server multiplayer (replicated board/pieces, RPC input, player numbers) | ✓ |
| Debug grid overlay (bDrawDebugGrid, host-only dev aid) | ✓ |
| OnGameStarted / OnGameEnded BP events (interface for UI engineer) + ShowDebugResult temp message | ✓ |
| Blueprint layer: BP_CFRGameMode, BP_CFRPiece_P1, BP_CFRPiece_P2 | ✓ |
| ARCHITECTURE.md and README.md | ✓ |

### Three Game Modes (cycle with Space Bar)

`Mode2D` (free placement / Gomoku) → `Mode2DClassic` (1 column at a time) → `Mode3D` → back to `Mode2D`.

### Not Yet Started ✗

| Feature | Notes |
|---------|-------|
| Networking Phase 2 — client-side hover/grid + replicated win/draw UI | Debug grid & ShowDebugResult are host-only today |
| Networking Phase 3 — Dedicated Server packaging | Phase 1 runs on Listen Server (PIE 2 players) |
| Board frame mesh | Currently only the host-only debug grid; needs a real placed mesh (art) |
| Win/draw results panel + whose-turn UI | OnGameEnded / OnGameStarted BP events exist, not implemented in BP |
| Python AlphaZero pipeline | See AI/ folder; reads the recorder JSON |
| Python rules + parity test | Reimplement rules in Python, replay recorder JSON to verify identical results |
| Camera polish / proper game view | Camera currently set manually |

---

## 3. Critical UE5-Specific Rules

**Violating any of these will cost hours of debugging. They have already caused bugs.**

---

### 3.1 Always Use the New C++ Class Wizard

**Never create `.h` / `.cpp` files manually.**

UE5's Unreal Header Tool (UHT) must generate `.generated.h` for every UCLASS/USTRUCT/UINTERFACE.
Manual files will compile locally but fail at link time with cryptic errors about missing reflection data.

```
Tools → New C++ Class → select parent → name (prefix CFR) → Create Class
```

After the wizard: replace file contents as needed. Never skip the wizard step.

---

### 3.2 Flat File Structure — No Subfolders

All C++ files live in one flat directory:
```
Source/ConnectFourRL/          ← everything here, no subdirectories
```

UHT and `#include` paths both depend on this. If you create a subfolder, includes will break.
The include convention is always `#include "CFRMyClass.h"` with no path prefix.

---

### 3.3 PURE_VIRTUAL Macro — Not = 0

Pure virtual methods in UCLASS must use:
```cpp
virtual ReturnType Method() const
    PURE_VIRTUAL(ClassName::Method, return DefaultValue;);
```

Using `= 0` directly causes UHT parse errors. This applies to every abstract base class.

---

### 3.4 DrawDebugString Lifetime — Use 0.f, Not -1.f

This is a UE5 trap where two debug functions treat `-1.f` differently:

| Function | `-1.f` meaning |
|----------|---------------|
| `DrawDebugBox` | Draw for **one frame**, cleared next frame ✓ |
| `DrawDebugString` | **Permanent**, never cleared ✗ |

If you use `-1.f` with `DrawDebugString` and call it every Tick, strings accumulate infinitely.
After switching 2D→3D, the old 2D strings remain in world space forever.

**Always use `0.f`** for strings drawn in Tick:
```cpp
DrawDebugString(GetWorld(), Location, Label, nullptr, FColor::Yellow, 0.f, false, 0.8f);
```

---

### 3.5 UE5 World Axes — Z is UP

In Unreal Engine 5:
- **X = Forward** (into the screen when camera looks forward)
- **Y = Right**
- **Z = Up (height)**

This is **not** the same as many other 3D tools. Do not confuse World Y with height.

The board coordinate system maps to these UE5 axes — see Section 4.

---

### 3.6 Blueprint Property Defaults vs Runtime Values

When you add a new `UPROPERTY` to a C++ parent class:
- The Blueprint child class inherits the C++ default value
- If you change the default in C++, already-created Blueprints may keep their old cached value
- Always verify the value in the **Details panel of the Blueprint** after adding new properties

Changes made in the Details panel during PIE (Play In Editor) affect the **live instance**.
Changes made in the Blueprint editor affect the **default for future PIE sessions**.
Stop PIE → change value → restart PIE for the change to be guaranteed.

---

### 3.7 NewObject vs SpawnActor

- `NewObject<UMyClass>(this)` — for non-Actor UObjects (rule sets, data objects, components not yet attached)
- `GetWorld()->SpawnActor<AMyActor>(Class, Location, Rotation)` — for Actors placed in the world

Using the wrong one for the wrong class type causes silent nullptrs or ensure failures.

UCFRGameRulesBase and its subclasses are UObjects → use `NewObject`.
ACFRPiece is an Actor → use `SpawnActor`.

---

### 3.8 Capture CurrentPlayer Before ApplyDrop / ApplyPlace

```cpp
// CORRECT
const int32 ActivePlayer = CurrentBoard.CurrentPlayer;   // capture BEFORE
Rules->ApplyDrop(CurrentBoard, X, Z, NextBoard);         // this advances CurrentPlayer
CurrentBoard = NextBoard;

// WRONG — ActivePlayer will be the NEXT player's index
Rules->ApplyDrop(CurrentBoard, X, Z, NextBoard);
const int32 ActivePlayer = CurrentBoard.CurrentPlayer;   // already advanced
```

Both `ApplyDrop` and `ApplyPlace` advance `CurrentPlayer` inside.
The piece spawned must belong to the player who made the move, not the next one.

---

### 3.9 Hot Reload Limitations

When you modify a `UPROPERTY` or `UFUNCTION` signature (not just the body), hot reload
is unreliable. Always do a **full rebuild**:

```
Tools → Compile  (Ctrl+Shift+B in Rider)
```

If you get bizarre "property not found" or reflection errors in PIE, close the editor,
rebuild from Rider/VS, then reopen.

---

### 3.10 TArray Replication

`TArray<ECFRCell>` inside `FCFRBoardState` will replicate natively when the struct is
marked with `Replicated` on its containing property. No custom serialization needed.
This is the plan for the networking phase.

---

## 4. Board Coordinate System (Read This First)

**This was the source of the most severe bugs.** The 2D and 3D modes use DIFFERENT world mappings.

### Board Axes (shared)

```
X — column  (0 = left,   SizeX-1 = right)
Y — row     (0 = bottom, SizeY-1 = top)   ← gravity fills from Y=0 upward
Z — depth   (0 = front,  SizeZ-1 = back)  ← always 0 in 2D mode
```

### `GetCellWorldPosition(X, Y, Z)` — Mode-Aware

```cpp
if (SizeZ == 1) // 2D
{
    return BoardOrigin + FVector(
        X * CellSize,   // Board X → World X  (left/right)
        Y * CellSize,   // Board Y → World Y  (front/back, gravity fills Y=0 first)
        0.f             // Board Z=0 → World Z=0 (flat on ground)
    );
}
else // 3D
{
    return BoardOrigin + FVector(
        X * CellSize,    // Board X → World X  (left/right)
        Z * CellSize,    // Board Z → World Y  (front/back depth)
        Y * CellHeight   // Board Y → World Z  (UP, stacking; uses CellHeight not CellSize)
    );
}
```

### Why CellHeight ≠ CellSize in 3D

The horizontal grid spacing (`CellSize`) and the vertical stacking step (`CellHeight`)
are decoupled so the stack height can match the actual donut mesh thickness
without affecting the base grid layout.

### Ray-Plane Intersection for Input

**Both 2D and 3D** use the horizontal ground plane (`Z = BoardOrigin.Z`):

```cpp
// Works for both modes — board bottom face is always at World Z = BoardOrigin.Z
const float T = (BoardOrigin.Z - RayOrigin.Z) / RayDirection.Z;
const FVector HitPoint = RayOrigin + RayDirection * T;

const int32 Col   = FloorToInt((HitPoint.X - BoardOrigin.X) / CellSize);
const int32 Row   = FloorToInt((HitPoint.Y - BoardOrigin.Y) / CellSize);  // 2D: board row
const int32 Depth = FloorToInt((HitPoint.Y - BoardOrigin.Y) / CellSize);  // 3D: board depth Z
```

For 2D, the player hovers over the flat board from above.
For 3D, the player hovers over the flat base grid from above.

**Previous wrong approach:** using Y-plane intersection (`Y = BoardOrigin.Y`) for 2D because
the board "looks like a wall." This fails when the camera is at a shallow angle to the ground.
The ground-plane approach works for any camera angle that isn't perfectly horizontal.

---

## 5. Gameplay Mechanics — 2D vs 3D

| | 2D — Mode2D (7×6×1) | 2D Classic — Mode2DClassic (7×6×1) | 3D — Mode3D (4×4×4) |
|---|---|---|---|
| Board layout | Flat horizontal board | Flat horizontal board | Flat 4×4 base, pieces stack up |
| Player action | Click any empty cell (X, Y) | Click a column (X); gravity picks row | Click a column (X, Z) on base grid |
| Gravity | None — free placement | Yes — lowest empty Y in column X | Yes — lowest empty Y in (X, Z) |
| Legal check | `IsLegalPlace(X, Y, 0)` | `IsLegalDrop(X, 0)` | `IsLegalDrop(X, Z)` |
| Apply method | `ApplyPlace(X, Y, Z)` | `ApplyDrop(X, 0)` | `ApplyDrop(X, Z)` |
| Win directions | 4 (XY plane) | 4 (XY plane) | 13 (axes + face + space diagonals) |
| Action space | 42 cells, encode `X + Y*7` | 7 columns, encode `X` | 16 columns, encode `X + Z*4` |
| Hover highlight | Single cell | Whole column (along World Y) | Whole vertical column (along World Z) |
| Discriminator | `Board.UsesFreePlacement()` (== Mode2D) | `SizeZ==1 && !UsesFreePlacement()` | `SizeZ > 1` |

> **Key refactor:** geometry uses `SizeZ` (both 2D modes are flat); the place-vs-drop mechanic uses
> `Board.UsesFreePlacement()` (only `Mode2D`). `Mode2DClassic` reuses `UCFRGameRules2D`'s board + win
> detection and only overrides `GetGameMode()`; the drop mechanic comes from the input layer calling
> `TryDrop` instead of `TryPlace`.

---

## 6. Win Detection — Key Facts

- `CheckConsecutive` is a **bidirectional scan from LastMove**, not a full-board scan
- Complexity: **O(D × N)** where D = directions, N = ConnectN (4)
- **Anchors on `LastMoveX/Y/Z`** — if those are -1 (no move yet), returns false immediately
- **Only checks the last mover** — `CheckResult` computes `LastPlayer = 3 - CurrentPlayer`
- `GetCell` returns `ECFRCell::Empty` for out-of-bounds — win scanner walks off edges safely

**Bidirectional is required** — forward-only misses wins completed from the middle:
```
■ ■ _ ■    drop at index 2:    ■ ■ X ■
Forward only: 1 to the right → count = 2 → MISS
Bidirectional: 2 left + 1 right + center = 4 → WIN ✓
```

---

## 7. Piece Spawn & Fall Pattern

```cpp
// In TryDrop and TryPlace — same pattern both:
const int32 ActivePlayer = CurrentBoard.CurrentPlayer;  // BEFORE ApplyDrop/ApplyPlace
Rules->Apply...(CurrentBoard, ..., NextBoard);
CurrentBoard = NextBoard;

const FVector Target = GetCellWorldPosition(X, LastMoveY, Z);
const FVector Spawn  = FVector(Target.X, Target.Y, BoardOrigin.Z + DropStartHeight);

ACFRPiece* Piece = GetWorld()->SpawnActor<ACFRPiece>(PieceClass, Spawn, FRotator::ZeroRotator);
Piece->StartFall(Target, PieceFallSpeed);
```

Spawn always at fixed `BoardOrigin.Z + DropStartHeight` regardless of stack height —
this ensures every piece falls from the same screen height visually.

ACFRPiece uses `USceneComponent` as root (not `UStaticMeshComponent`) to isolate
donut mesh pivot offsets from the spawn position. Pivot correction is done inside
the Blueprint child's mesh relative transform.

---

## 8. Deep Learning Architecture Plan

### Data Collection (UCFRGameRecorder — IMPLEMENTED)

Every completed game (human play today; Python self-play later) is written as one JSON file to
`Game/ConnectFourRL/Saved/TrainingData/<game_id>.json`. The schema is **MCTS-ready** — the same
format serves both UE5 human games (one-hot policy) and Python self-play (real visit-count policy).

```json
{
  "game_id": "hexstring",
  "mode": "2D" | "2D-Classic" | "3D",
  "board_size": {"x": 7, "y": 6, "z": 1},
  "connect_n": 4,
  "action_space": 42,
  "final_result": "Player1Wins" | "Player2Wins" | "Draw",
  "moves": [
    {
      "move_number": 1,
      "player": 1,
      "board": [0,0,0,...],              // state BEFORE the move (flat ECFRCell)
      "action": 8,                        // flat index (see EncodeAction)
      "action_xyz": {"x": 1, "y": 1, "z": 0},
      "policy": [0,...,1,...,0],          // visit counts over action_space; one-hot for human play
      "value": 1.0                        // +1/-1/0 from this move's player perspective (filled at game end)
    }
  ]
}
```

Notes:
- `board_after` was dropped (redundant — it's the next move's `board`, halves file size).
- `value` is the AlphaZero z-target; `policy` is the π-target (raw counts, trainer normalises).
- `action`/`action_space` come from `FCFRBoardState::EncodeAction` / `GetActionSpaceSize` — **the Python
  pipeline must use the identical encoding** (this is what the parity test guards).
- Only completed games are written; switching mode mid-game discards the unfinished record.
- Toggle with `bRecordGames` on `BP_CFRGameMode` (default true). Recording runs server-side only.

### AlphaZero Pipeline (Python — AI/ folder)

```
1. Self-play → JSON records (UCFRGameRecorder)
2. JSON → NumPy tensors (data/processed/)
3. Train neural net:
   - Input:  board state as 3D tensor [SizeX × SizeY × SizeZ × channels]
   - Policy head: probability over all legal moves
   - Value head: scalar [-1, 1] win estimate
4. MCTS guided by network policy + value
5. Export trained model to ONNX
6. Import ONNX into UE5 via NNE plugin
```

Board representation for NN:
```
2D: [7 × 6 × 1 × 3] — 3 channels: empty / player1 / player2
3D: [4 × 4 × 4 × 3] — same channel scheme
```

### UE5 ↔ Python Interface

UCFRGameRecorder writes JSON to disk (or pipes via socket) after each game.
Python reads these files for training. UE5 loads the ONNX via the `NNE` plugin for inference.
No runtime Python in UE5 — all inference is via exported ONNX.

---

## 9. Networking

### Phase 1 — DONE (Listen Server)

The critical UE5 fact: **`AGameModeBase` exists only on the server.** Clients have no GameMode, so
everything client-readable was moved to the replicated `ACFRGameState`.

| Step | Status | How |
|------|--------|-----|
| `ACFRGameState` holds replicated board + visual params | ✓ | `UPROPERTY(ReplicatedUsing=OnRep_Board) FCFRBoardState Board`; `CellSize/CellHeight/BoardOrigin/bGameOver` also replicated |
| Server keeps board in sync | ✓ | `ACFRGameMode` owns authoritative `CurrentBoard`; `SyncStateToGameState()` mirrors it into the GameState after every change |
| Pieces visible on all clients | ✓ | `ACFRPiece` is replicated; server spawns, UE replicates the actor. Fall animation: replicate start+target+speed, each client interpolates locally (`SetReplicateMovement(false)`) |
| Input via Server RPC | ✓ | `ACFRPlayerController::Server_TryDrop / Server_TryPlace / Server_SwitchGameMode` |
| Authority guard | ✓ | `ACFRGameMode::TryDrop/TryPlace` start with `if (!Rules \|\| bGameOver \|\| !HasAuthority()) return false` |
| Player numbers + turn check | ✓ | `PostLogin` assigns `PlayerNumber` (1,2,...); RPCs reject moves when `PlayerNumber != Board.CurrentPlayer` |
| Client hover/legality | ✓ | client reads `GameState->Board`; `FCFRBoardState::IsLegalDrop/IsLegalPlace` are board-level (no Rules needed) |

**Test:** PIE → Number of Players = 2, Net Mode = **Play As Listen Server**.

### Phase 2 — remaining (client-visible UI)

- Debug grid (`DrawDebugGrid` in `GameMode::Tick`) and `ShowDebugResult` run server-side only → **host sees them, clients don't.** Move hover/grid client-side and drive win/draw UI from replicated `bGameOver` / a replicated result. `OnRep_Board` is a ready stub for this.
- `ACFRHoverIndicator` already works client-side (each client drives its own) — assign `BP_CFRHoverIndicator` to `BP_CFRPlayerController.HoverIndicatorClass` to enable it.

### Phase 3 — remaining (Dedicated Server)

- Add a server build target, package, launch with `-server -log`, connect two clients.

`FCFRBoardState.Cells` is `TArray<ECFRCell>` — replicates natively, no custom serialization.

---

## 10. Parameters Reference

All tunable values are `UPROPERTY` on `BP_CFRGameMode` (Details panel → Board | Visual / Board | Debug / Piece).

| Parameter | Class | Default | Purpose |
|-----------|-------|---------|---------|
| `CellSize` | ACFRGameMode | 150 | Horizontal spacing between cell centres (World X/Y) |
| `CellHeight` | ACFRGameMode | 50 | Vertical stacking step in 3D (World Z); also debug box Z height |
| `CellGap` | ACFRGameMode | 10 | Shrinks debug box relative to CellSize; set to 20–30 for visible gap |
| `BoardOrigin` | ACFRGameMode | (0,0,0) | World position of board cell (0,0,0) |
| `DropStartHeight` | ACFRGameMode | 800 | Piece spawn height above BoardOrigin.Z |
| `PieceFallSpeed` | ACFRGameMode | 600 | Units/sec fall speed |
| `bDrawDebugGrid` | ACFRGameMode | false | Enable cyan cell overlay in PIE |
| `PieceActorClassP1` | ACFRGameMode | — | Set to BP_CFRPiece_P1 |
| `PieceActorClassP2` | ACFRGameMode | — | Set to BP_CFRPiece_P2 |
| `BoardSizeX/Y/Z` | UCFRGameRulesBase | 7/6/1 (2D), 4/4/4 (3D) | Override in editor if needed |

---

## 11. Bug History — What Went Wrong and Why

These bugs already happened. Do not repeat them.

| Bug | Root cause | Fix |
|-----|-----------|-----|
| `.generated.h not found` | Files created manually without UHT wizard | Always use Tools → New C++ Class |
| Wrong include path | Tried to use subfolder structure | Flat structure only |
| UCLASS pure virtual error | Used `= 0` instead of `PURE_VIRTUAL` macro | `PURE_VIRTUAL(Cls::Method, return val;)` |
| Hardcoded board sizes | Magic numbers in constructors | All sizes as `UPROPERTY` fields |
| Naive O(W×H×D×N) win scan | Full board walk every turn | Bidirectional scan from `LastMoveX/Y/Z` |
| Debug grid showing as staircase | Board Y → World Z made all rows offset | Fixed to draw all rows at Z=0 (was wrong coordinate mapping) |
| 3D grid showing as flat wall | Board Y → World Y (depth) instead of World Z (up) | See Section 4 |
| 2D board showing as vertical wall | Board Y → World Z made board stand up | 2D: Board Y → World Y, World Z = 0 |
| Pieces spawning outside grid | Ray hit wrong plane (Y-plane used for 2D) | Both modes: use Z-plane (ground) |
| Whole column highlighted in 2D | `X == HoveredColumn` checked without Y | Added `HoveredRow`, check `X==Col && Y==Row` |
| DrawDebugString accumulating | `-1.f` lifetime = permanent for strings | Changed to `0.f` for per-frame draw |
| Donut pivot offset | Mesh pivot not at centre | USceneComponent as root; fix in Blueprint child's relative transform |
| Wrong ActivePlayer for piece colour | Captured after ApplyDrop | Capture BEFORE Apply call |
| `SetPlayerMaterial` approach | User's donuts have their own materials | Two separate TSubclassOf classes (P1/P2) |

---

## 12. Style Rules (Non-Negotiable)

The user has corrected these multiple times. Do not violate.

1. **No hardcoded values.** Every number that could reasonably vary is a `UPROPERTY`.
2. **C++ first.** Never suggest Blueprint for logic that can be done in C++.
3. **Professional architecture from the start.** Do not propose a naive approach and iterate.
   The user will reject it and it wastes time. Design correctly the first time.
4. **Short, targeted comments only.** Doxygen on declarations in `.h`. One-line explanations inside `.cpp` only when the WHY is non-obvious. No narrative comments.
5. **No OnPiecePlaced Blueprint event.** Piece spawning is done directly in C++ via `SpawnActor`.
6. **Communication in Chinese.** All explanations to the user are in Chinese. Code and comments are in English.

---

## 13. Next Priorities (suggested order)

**Done since last handoff:** 2D Classic mode · UCFRGameRecorder (JSON) · Networking Phase 1 (Listen Server) · ACFRHoverIndicator.

Active focus is the **Python pipeline** (the owner is working on JSON → Python next; UE5 is paused).

1. **Python rules + parity test** — reimplement the 3 modes' rules in Python; replay recorder JSON and assert identical board/result (guards the shared `EncodeAction`/win-detection encoding)
2. **Python AlphaZero pipeline** — board env, JSON→tensor loader, policy/value net, MCTS self-play (writes the same JSON schema with real visit counts)
3. **(Optional, "做法 2")** — extract a pure-C++ rules core and bind it to Python via pybind11 so both UE5 and Python share one rules implementation (eliminates divergence; replaces the parity test)
4. **Networking Phase 2** — client-side hover/grid + replicated win/draw UI (see Section 9)
5. **Win/Draw + whose-turn UI** — implement `OnGameEnded` / `OnGameStarted` in `BP_CFRGameMode`
6. **Networking Phase 3** — Dedicated Server packaging
7. **ONNX inference in UE5** — NNE plugin integration for the AI opponent
8. **Camera / board frame mesh** — proper game view + a real board mesh (replaces host-only debug grid)

---

## 14. Repository

- **Repo:** https://github.com/alina0607/ConnectFourRL.git
- **Active branch:** `dev`
- **Commit convention:** `feat/fix/refactor/docs/chore: short description`
- **Git GUI:** GitHub Desktop (user does not use CLI for git)
