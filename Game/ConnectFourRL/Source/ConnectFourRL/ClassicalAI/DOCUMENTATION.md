# Classical AI Documentation

This folder contains a reusable minimax alpha-beta search and the classes that
adapt the ConnectFourRL board to it.

## Files

| File | Responsibility |
|---|---|
| `AlphaBetaTypes.h` | Generic search settings, statistics, and result |
| `AlphaBetaSearch.h` | Reusable minimax alpha-beta algorithm |
| `CFRMove.h` | Unified move representation for 2D and 3D modes |
| `CFRBoardEvaluator.*` | Scores Connect Four board positions |
| `CFRMoveOrderer.*` | Searches promising moves first for better pruning |
| `CFRAlphaBetaGame.*` | Adapts Connect Four rules to the generic search API |
| `CFRMinimaxComponent.*` | Unreal and Blueprint-facing search component |
| `CFRMoveLibrary.*` | Blueprint helper for constructing `FCFRMove` |

## Architecture

```text
UCFRMinimaxComponent
        |
        v
FCFRAlphaBetaGame
   |            |
   v            v
Evaluator    Move orderer
        |
        v
TAlphaBetaSearch
```

`TAlphaBetaSearch` contains no Connect Four types. To reuse it in another Unreal
project, provide another adapter with the same methods as `FCFRAlphaBetaGame`.

## C++ Usage

```cpp
#include "ClassicalAI/AlphaBetaSearch.h"
#include "ClassicalAI/CFRAlphaBetaGame.h"

FCFRAlphaBetaGame Game(*Rules);

FAlphaBetaSettings Settings;
Settings.MaxDepth = 4;

TAlphaBetaSearch<
    FCFRBoardState,
    FCFRMove,
    FCFRAlphaBetaGame> Search;

const TAlphaBetaResult<FCFRMove> Result =
    Search.FindBestMove(CurrentBoard, Game, Settings);

if (Result.bHasMove)
{
    if (CurrentBoard.GameMode == ECFRGameMode::Mode2D)
    {
        TryPlace(Result.BestMove.X, Result.BestMove.Y, Result.BestMove.Z);
    }
    else
    {
        TryDrop(Result.BestMove.X, Result.BestMove.Z);
    }
}
```

The search never modifies `CurrentBoard`. It creates copied child states through
the existing `ApplyPlace` and `ApplyDrop` rule methods.

## Blueprint Usage

1. Add a **CFR Minimax Component** to an Actor or Blueprint.
2. Set `Search Depth`. Begin with `2` or `3` for 2D free placement.
3. Call `Find Best Move`.
4. Pass the current `FCFRBoardState` and the matching `UCFRGameRulesBase`.
5. When the function returns true, apply the returned move:
   - 2D: call `TryPlace(X, Y, Z)`.
   - 3D: call `TryDrop(X, Z)`.

The function also returns the score, visited-node count, and cutoff count.

## Adapter Contract

Another game can reuse `TAlphaBetaSearch` by implementing:

```cpp
int32 GetCurrentPlayer(const State&) const;
void GenerateMoves(const State&, TArray<Move>&) const;
bool ApplyMove(const State&, const Move&, State& OutChild) const;
bool IsTerminal(const State&) const;
double Evaluate(const State&, int32 PerspectivePlayer, int32 Depth) const;
void OrderMoves(const State&, TArray<Move>&) const;
```

No inheritance or Unreal reflection is required for the adapter.

## Current Evaluation

Terminal positions use large win/loss scores. Non-terminal positions score every
possible four-cell line:

- Three pieces plus one empty cell: `1000`
- Two pieces plus two empty cells: `50`
- One piece plus three empty cells: `2`

The opponent's score is subtracted from the searching player's score.

## Performance Notes

- 2D currently allows placement in any empty cell, so its branching factor is
  much larger than classic gravity Connect Four.
- Start at depth `2` or `3` in 2D and profile before increasing it.
- Move ordering prioritizes immediate wins and central positions.
- Search currently runs synchronously. A deep search can pause the game thread.
- Iterative deepening, time limits, cancellation, and transposition tables are
  suitable later additions after the basic behavior is tested.

## Testing Checklist

1. AI selects an immediate winning move.
2. AI blocks an opponent's immediate win.
3. AI returns false on a terminal board.
4. AI selects the only remaining legal move.
5. 3D vertical and space-diagonal wins are detected.
6. Increasing depth increases `Nodes Visited`.
