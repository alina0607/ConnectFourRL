# ConnectFourRL

> Online multiplayer Connect Four built in **Unreal Engine 5 C++**, with an **AlphaZero-style Reinforcement Learning AI** that learns from human games and improves through self-play.

Supports **2D (6×7)** and **3D (4×4×4)** game modes with fully switchable rule sets.

---

## Highlights

- **Online multiplayer** via UE5 Dedicated Server with full board state replication
- **2D and 3D modes** sharing a single rule architecture — switching modes requires zero logic changes
- **AlphaZero-style AI** pipeline: supervised pretraining on human data → MCTS self-play → ONNX deployment back into UE5
- **Incremental win detection** at O(D × N) per move — ~42× faster than naïve full-board scan, critical for MCTS throughput at scale
- **Structured JSON recording** of every game for AI training data collection

---

## Tech Stack

| | |
|---|---|
| Game Engine | Unreal Engine 5.7 (C++) |
| Networking | UE5 Dedicated Server · Actor Replication |
| AI Training | PyTorch · Custom MCTS |
| Model Deployment | ONNX → UE5 Runtime Inference |
| Data Pipeline | JSON → NumPy |

---

## Architecture

```
┌──────────────────────────────────────────────────┐
│                 Unreal Engine 5                   │
│                                                   │
│   Client A       │   Server    │   Client B       │
│   ─────────      │   ──────    │   ─────────      │
│   Input (click)  │  GameMode   │  Input (click)   │
│        │         │  (rules)    │       │           │
│        └─── RPC ─▶     │      ◀─ RPC ─┘           │
│                   │  GameState │                   │
│                   │  (board)   │                   │
│                   └─────┬──────┘                   │
│              replicate  │  replicate               │
│           ┌─────────────┴──────────────┐           │
│         Client A                   Client B        │
│        (spawn piece)              (spawn piece)    │
│                         │                          │
│                   GameRecorder                     │
└─────────────────────────┼────────────────────────┘
                          │ JSON
                          ▼
┌──────────────────────────────────────────────────┐
│              Python AI Pipeline                   │
│                                                   │
│  Human games → Supervised pretrain                │
│       ↓                                           │
│  Neural Net (Policy head + Value head)            │
│       ↓                                           │
│  MCTS self-play → unlimited training data         │
│       ↓                                           │
│  Export ONNX → UE5 in-game inference              │
└──────────────────────────────────────────────────┘
```

---

## Win Detection — Algorithm Design

Naïve implementations scan the entire board after every move:

```
O(W × H × D × Directions × N)
  2D: 7 × 6 × 1 × 4 × 4 = 672 ops/call
  3D: 4 × 4 × 4 × 13 × 4 = 3,328 ops/call
```

ConnectFourRL uses an **incremental bidirectional scan** anchored on the last placed piece:

```
O(Directions × N)
  2D: 4 × 4 = 16 ops/call    (~42× faster)
  3D: 13 × 4 = 52 ops/call   (~64× faster)
```

**Key insight:** a winning run can only pass through the piece just placed.
The bidirectional scan walks both `+Dir` and `−Dir` from that cell, correctly handling
wins completed from the middle of a sequence.

At MCTS scale (800 simulations × ~21 moves per game):

| | Naïve | Incremental |
|---|---|---|
| Total ops per move (2D) | ~11.3 M | ~268 K |
| Speedup | 1× | **~42×** |

---

## Game Modes

| Mode | Board | Win Condition | Directions |
|------|-------|---------------|------------|
| 2D | 6 × 7 | 4 in a row | 4 (H / V / 2 diagonals) |
| 3D | 4 × 4 × 4 | 4 in a row | 13 (all 3D axes + diagonals) |

---

## AI Pipeline

```
Stage 1 — Supervised Learning
  Collect human vs human games (JSON)
  Train network to predict moves and outcomes

Stage 2 — Reinforcement Learning
  MCTS-guided self-play generates unlimited data
  Network iteratively improves beyond human level

Deployment
  Export model to ONNX
  Load directly into UE5 for real-time inference
```

**Network I/O:**

```
Input  (2D): tensor (3, 6, 7)
  Ch 0 — current player's pieces
  Ch 1 — opponent's pieces
  Ch 2 — turn indicator

Output:
  Policy head — move probability distribution
  Value head  — win probability ∈ [−1, +1]
```

---

## Roadmap

**Engine**
- [x] Core board state & gravity mechanic
- [x] Extensible rule interface (2D / 3D)
- [x] Optimized win detection
- [x] Local game session (GameMode + column input)
- [ ] Networked multiplayer (GameState replication + Server RPC)
- [ ] Game recorder (JSON export)
- [ ] 3D mode

**AI**
- [ ] Python board environment
- [ ] MCTS implementation
- [ ] Supervised training
- [ ] Self-play pipeline
- [ ] ONNX deployment

---

## Quick Start

```bash
git clone https://github.com/alina0607/ConnectFourRL.git
cd ConnectFourRL
git checkout dev
# Open Game/ConnectFourRL/ConnectFourRL.uproject in UE5
# Rebuild when prompted, then open in Rider
```

> For internal development details, class references, and contribution guidelines — see [ARCHITECTURE.md](ARCHITECTURE.md).
