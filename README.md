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
