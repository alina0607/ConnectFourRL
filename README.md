# ConnectFourRL

A multiplayer Connect Four game built with **Unreal Engine 5 (C++)**, combined with an **AlphaZero-style Reinforcement Learning AI** that learns by playing against itself.

Supports both **2D (6×7)** and **3D (4×4×4)** game modes with switchable rules.

---

## Project Goals

- Build a fully playable **online multiplayer** Connect Four in UE5 (C++)
- Record all human games as training data
- Train a **Neural Network + MCTS** AI using the AlphaZero approach
- AI improves continuously through **self-play**, beyond human level

---

## Architecture Overview

```
┌─────────────────────────────────────┐
│          Unreal Engine 5            │
│                                     │
│  ┌─────────┐      ┌──────────────┐  │
│  │  Game   │──────│  Dedicated   │  │
│  │ Client  │      │    Server    │  │
│  └─────────┘      └──────┬───────┘  │
│                          │          │
│                   ┌──────▼───────┐  │
│                   │ Game Recorder│  │
│                   └──────┬───────┘  │
└──────────────────────────┼──────────┘
                           │ JSON
                           ▼
┌─────────────────────────────────────┐
│             Python AI               │
│                                     │
│  Human Data → Supervised Pretrain   │
│       ↓                             │
│  Neural Network (Policy + Value)    │
│       ↓                             │
│  MCTS + Self-Play → More Data       │
│       ↓                             │
│  Stronger Model → Export to UE5     │
└─────────────────────────────────────┘
```

---

## Game Modes

| Mode | Board Size | Win Condition |
|------|-----------|---------------|
| 2D   | 6 × 7     | 4 in a row (horizontal / vertical / diagonal) |
| 3D   | 4 × 4 × 4 | 4 in a row (49 possible directions) |

---

## Project Structure

```
ConnectFourRL/
├── Game/                        # Unreal Engine 5 C++ Project
│   └── Source/
│       └── ConnectFourRL/
│           ├── Core/            # Board state, rules interface (2D & 3D)
│           │   ├── CFRBoardState.h
│           │   ├── ICFRGameRules.h
│           │   ├── CFRGameRules2D.h/.cpp
│           │   └── CFRGameMode.h/.cpp
│           ├── Network/         # Multiplayer replication
│           │   └── CFRGameState.h/.cpp
│           ├── UI/              # HUD, widgets
│           └── Data/            # Game recording & JSON export
│               └── CFRGameRecorder.h/.cpp
│
└── AI/                          # Python Deep Learning
    ├── data/
    │   ├── games/               # Raw game records (.json)
    │   └── processed/           # Converted tensors (.npy)
    ├── models/
    │   ├── checkpoints/         # Training checkpoints (.pt)
    │   └── exported/            # Final models for UE5 (.onnx)
    ├── env/                     # Game environment (for MCTS)
    ├── mcts/                    # Monte Carlo Tree Search
    ├── training/                # Training scripts
    └── selfplay/                # Self-play data generation
```

---

## C++ Class Responsibilities

| Class | Type | Responsibility |
|-------|------|----------------|
| `FCFRBoardState` | Struct | Holds the raw board data — who placed a piece where |
| `ICFRGameRules` | Interface | Shared rule contract for both 2D and 3D modes |
| `UCFRGameRules2D` | UObject | Implements 2D win detection, legal moves, apply move |
| `ACFRGameMode` | AGameMode | Controls turn flow, calls rules, detects game over |
| `ACFRGameState` | AGameState | Replicates board state to all clients over network |
| `UCFRGameRecorder` | UObject | Records every move, exports finished game as JSON |

> Adding 3D support later only requires a new `UCFRGameRules3D` class — nothing else changes.

---

## Core Data & Rules Design

### FCFRBoardState

The board is stored as a **flat 1D array** regardless of mode, using an index formula to map 3D coordinates:

```
index = X + (Y * SizeX) + (Z * SizeX * SizeY)

Coordinate system:
  X = column  (left → right)
  Y = row     (bottom → top, gravity direction)
  Z = depth   (front → back, always 0 in 2D mode)

2D board: SizeX=7, SizeY=6, SizeZ=1
3D board: SizeX=4, SizeY=4, SizeZ=4
```

Why a flat array?
- Native UE5 `TArray` replication support (required for networking)
- Contiguous memory = better cache performance
- Simple serialization to JSON for AI training data

Key methods:

| Method | Description |
|--------|-------------|
| `Init(Mode)` | Sets board dimensions, clears all cells, resets turn to Player 1 |
| `GetCell(X, Y, Z)` | Reads a cell state; returns Empty if out of bounds |
| `SetCell(X, Y, Z, Value)` | Writes a cell state; no-op if out of bounds |
| `IsInBounds(X, Y, Z)` | Validates that all three indices are within board limits |
| `GetDropRow(X, Z)` | Gravity mechanic — returns the lowest empty Y in column (X, Z), or -1 if full |

### ICFRGameRules

A pure C++ interface that defines the complete rule contract for Connect Four.
Both 2D and 3D implementations must satisfy every method.

**Why an interface?**
Swapping between 2D and 3D only requires pointing to a different implementation — no other system changes.

**Move execution flow:**

```
Player selects (X, Z)
        ↓
IsLegalDrop(Board, X, Z)      — is this move allowed?
        ↓ legal
ApplyDrop(Board, X, Z, Out)   — execute move, return NEW board (original untouched)
        ↓
CheckResult(OutBoard)         — did the game end?
        ↓
HasPlayerWon()                — called internally by CheckResult
```

**Why ApplyDrop returns a new board instead of modifying in place?**
MCTS simulates thousands of branches from the same position simultaneously.
Each branch needs its own independent copy — mutating the original would corrupt all other branches.

**Why GetLegalMoves matters for MCTS:**

```
2D — scans X=0..6, Z=0:    up to 7 legal positions   e.g. [(0,0),(1,0),(3,0)...]
3D — scans all (X,Z) pairs: up to 16 legal positions  e.g. [(0,0),(0,1),(1,0)...]
```

MCTS calls `GetLegalMoves` at every node to know which branches to expand next.

Interface methods:

| Method | Description |
|--------|-------------|
| `GetGameMode()` | Returns Mode2D or Mode3D — identifies which rule set is active |
| `CreateInitialBoard()` | Builds and returns a fresh empty board for this mode |
| `IsLegalDrop(Board, X, Z)` | True if (X,Z) is in bounds and the column is not full |
| `GetLegalMoves(Board)` | Returns all legal (X,Z) positions as `TArray<FIntPoint>` |
| `ApplyDrop(Board, X, Z, Out)` | Copies board, places piece, advances turn — original board unchanged |
| `CheckResult(Board)` | Returns Ongoing / Player1Wins / Player2Wins / Draw |
| `HasPlayerWon(Board, Player)` | Checks all directions for four consecutive pieces |

---

## Performance & Algorithm Design

### Incremental Win Detection

Naïve win-checking scans every cell on the board after each move:

```
Complexity: O(W × H × D × Directions × N)
  2D example: 7 × 6 × 1 × 4 × 4  =   672 operations / call
  3D example: 4 × 4 × 4 × 13 × 4 = 3,328 operations / call
```

This project uses an **incremental, bidirectional scan** anchored on the last placed piece:

```
Complexity: O(Directions × N)
  2D: 4 × 4  =  16 operations / call   (~42× faster)
  3D: 13 × 4 =  52 operations / call   (~64× faster)
```

**Why this is correct:**
A new winning run can only pass through the piece that was just placed. Any run that existed before the move was already checked in a previous turn. Therefore it is sufficient — and complete — to restrict the search to that single cell.

**Bidirectional scan:**
For each direction axis, the algorithm walks both `+Dir` and `−Dir` from the placed piece and sums the counts. This correctly handles cases where the winning piece completes a run from the middle:

```
Before:  ■ ■ _ ■    (3 pieces, gap in position 2)
Move:    ■ ■ X ■    (X placed at position 2)

+Dir count = 1  (one piece to the right)
−Dir count = 2  (two pieces to the left)
Total      = 1 + 2 + 1 (center) = 4  →  WIN
```

**Additional optimization — single-player result check:**
`CheckResult` only evaluates the player who just moved. Because a win requires placing a piece, the non-active player cannot have created a new winning condition this turn, halving the number of `HasPlayerWon` calls per move.

### MCTS Performance Impact

During self-play, MCTS evaluates thousands of simulated game trees per move. A typical 2D game lasts ~21 moves; with 800 simulations per move:

| Metric | Naïve scan | Incremental scan |
|--------|-----------|-----------------|
| `CheckResult` calls per move | 800 × 21 = 16,800 | 16,800 |
| Operations per call (2D) | 672 | 16 |
| **Total operations per move** | **11.3 M** | **268 K** |
| Relative speedup | 1× | **~42×** |

At scale (millions of self-play games), this difference is the boundary between a training pipeline that completes overnight and one that takes weeks.

---

## AI Pipeline

### Stage 1 — Supervised Learning (Human Data)
- Collect human vs human games via UE5 multiplayer
- Each game saved as JSON: `{ board_state, move, player, winner }`
- Train neural network to imitate human decisions

### Stage 2 — Self-Play (Reinforcement Learning)
- AI plays against itself using MCTS guidance
- Generates unlimited training data automatically
- Network improves iteratively — no human data needed

### Neural Network Input
```
2D mode: Tensor shape (3, 6, 7)
  Channel 0 — current player's pieces
  Channel 1 — opponent's pieces
  Channel 2 — whose turn (all 1s or all 0s)

3D mode: Tensor shape (3, 4, 4, 4)
  Same channel structure, extra spatial dimension
```

### Neural Network Output
```
Policy head — probability distribution over legal moves
Value head  — estimated win probability [-1, +1]
```

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Game Engine | Unreal Engine 5 (C++) |
| Networking | UE5 Dedicated Server + Replication |
| AI Framework | PyTorch |
| MCTS | Custom Python implementation |
| Model Export | ONNX (PyTorch → UE5) |
| Data Format | JSON → NumPy tensors |

---

## Development Roadmap

- [ ] 2D game logic (board, win detection, legal moves)
- [ ] Local multiplayer (same machine)
- [ ] UE5 Dedicated Server setup
- [ ] Online multiplayer
- [ ] Game recording & JSON export
- [ ] Python environment & MCTS implementation
- [ ] Supervised learning from human games
- [ ] Self-play pipeline
- [ ] ONNX model export & UE5 inference
- [ ] 3D mode extension
