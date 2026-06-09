// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CFRBoardState.h"
#include "CFRGameRules.generated.h"

/**
 * @brief Represents the outcome of a game after each move is applied.
 */
UENUM(BlueprintType)
enum class ECFRGameResult : uint8
{
	Ongoing     UMETA(DisplayName = "Ongoing"),
	Player1Wins UMETA(DisplayName = "Player 1 Wins"),
	Player2Wins UMETA(DisplayName = "Player 2 Wins"),
	Draw        UMETA(DisplayName = "Draw")
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCFRGameRules : public UInterface
{
	GENERATED_BODY()
};

class CONNECTFOURRL_API ICFRGameRules
{
    GENERATED_BODY()

public:

    // ------------------------------------------------------------------
    // Board identity
    // ------------------------------------------------------------------

    /**
     * @brief Returns the game mode this rule set handles (2D or 3D).
     * @return ECFRGameMode::Mode2D or ECFRGameMode::Mode3D.
     */
    virtual ECFRGameMode GetGameMode() const = 0;

    /**
     * @brief Creates and returns a fresh board configured for this rule set.
     * @return A fully initialized FCFRBoardState ready to play.
     */
    virtual FCFRBoardState CreateInitialBoard() const = 0;

    // ------------------------------------------------------------------
    // Move validation
    // ------------------------------------------------------------------

    /**
     * @brief Returns true if dropping a piece at column (X, Z) is legal.
     *
     * A move is legal when (X, Z) is in bounds and the column is not full.
     *
     * @param Board  The current board state.
     * @param X      Column index.
     * @param Z      Depth index (always 0 in 2D mode).
     * @return       True if the move is legal.
     */
    virtual bool IsLegalDrop(const FCFRBoardState& Board, int32 X, int32 Z) const = 0;

    /**
     * @brief Returns all legal drop positions as (X, Z) pairs.
     *
     * Used by MCTS to enumerate possible moves from the current state.
     *
     * @param Board  The current board state.
     * @return       Array of FIntPoint(X, Z) for every legal column.
     */
    virtual TArray<FIntPoint> GetLegalMoves(const FCFRBoardState& Board) const = 0;

    // ------------------------------------------------------------------
    // Move execution
    // ------------------------------------------------------------------

    /**
     * @brief Applies a drop at column (X, Z) and returns the resulting board.
     *
     * Does NOT modify the input board — returns a new copy.
     * The piece falls to the lowest empty row under gravity.
     * Advances CurrentPlayer and increments MoveCount on the output board.
     *
     * @param Board     The current board state (unchanged).
     * @param X         Column index.
     * @param Z         Depth index (always 0 in 2D mode).
     * @param OutBoard  The resulting board state after the move.
     * @return          True if the move was applied, false if illegal.
     */
    virtual bool ApplyDrop(
        const FCFRBoardState& Board,
        int32                 X,
        int32                 Z,
        FCFRBoardState&       OutBoard
    ) const = 0;

    // ------------------------------------------------------------------
    // Game result
    // ------------------------------------------------------------------

    /**
     * @brief Evaluates the board and returns the current game result.
     *
     * Evaluation order:
     *   1. HasPlayerWon(Board, 1) -> Player1Wins
     *   2. HasPlayerWon(Board, 2) -> Player2Wins
     *   3. GetLegalMoves empty    -> Draw
     *   4. Otherwise              -> Ongoing
     *
     * @param Board  The board state to evaluate.
     * @return       The current ECFRGameResult.
     */
    virtual ECFRGameResult CheckResult(const FCFRBoardState& Board) const = 0;

    /**
     * @brief Returns true if the given player has four consecutive pieces
     *        in any valid direction on the board.
     *
     * @param Board   The board state to check.
     * @param Player  The player index to check (1 or 2).
     * @return        True if that player has won.
     */
    virtual bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const = 0;
};