// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CFRGameRules.h"
#include "CFRGameRulesBase.generated.h"

/**
 * @brief Abstract base class that implements all shared Connect Four rule logic.
 *
 * Both UCFRGameRules2D and UCFRGameRules3D inherit from this class.
 * Subclasses only need to implement GetGameMode() and HasPlayerWon().
 * All other rule logic is shared and implemented here.
 */
UCLASS(Abstract)
class CONNECTFOURRL_API UCFRGameRulesBase : public UObject, public ICFRGameRules
{
	GENERATED_BODY()

public:

	/** @brief Board width (number of columns, X axis). Default 7 for 2D, 4 for 3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board Settings")
	int32 BoardSizeX = 7;

	/** @brief Board height (number of rows, Y axis, gravity direction). Default 6 for 2D, 4 for 3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board Settings")
	int32 BoardSizeY = 6;

	/** @brief Board depth (Z axis). Always 1 for 2D, default 4 for 3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board Settings")
	int32 BoardSizeZ = 1;
	
	// ------------------------------------------------------------------
	// Pure virtual — implemented by UCFRGameRules2D / UCFRGameRules3D
	// ------------------------------------------------------------------

	/** @brief Returns the game mode this subclass handles (Mode2D or Mode3D). */
	virtual ECFRGameMode GetGameMode() const
		PURE_VIRTUAL(UCFRGameRulesBase::GetGameMode, return ECFRGameMode::Mode2D;);

	/**
	 * @brief Returns true if the given player has four consecutive pieces in any valid direction.
	 * @param Board   The board state to inspect.
	 * @param Player  Player index (1 or 2).
	 */
	virtual bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const
		PURE_VIRTUAL(UCFRGameRulesBase::HasPlayerWon, return false;);

	// ------------------------------------------------------------------
	// Shared implementations — defined in CFRGameRulesBase.cpp
	// ------------------------------------------------------------------

	/** @brief Creates and returns a blank board sized to BoardSizeX/Y/Z. */
	virtual FCFRBoardState CreateInitialBoard() const override;

	/**
	 * @brief Returns true if dropping at column (X, Z) is legal.
	 * @param Board  Current board state.
	 * @param X      Column index (0 to SizeX-1).
	 * @param Z      Depth index  (0 to SizeZ-1); always 0 in 2D mode.
	 */
	virtual bool IsLegalDrop(const FCFRBoardState& Board, int32 X, int32 Z) const override;

	/**
	 * @brief Returns every legal drop position as (X, Z) pairs.
	 * @param Board  Current board state.
	 * @return       Array of FIntPoint(X, Z) for each non-full column.
	 */
	virtual TArray<FIntPoint> GetLegalMoves(const FCFRBoardState& Board) const override;

	/**
	 * @brief Copies the board, places the current player's piece, advances turn.
	 * @param Board     Input board (not modified).
	 * @param X         Column index.
	 * @param Z         Depth index; always 0 in 2D mode.
	 * @param OutBoard  Output board after the move.
	 * @return          True if the move was applied; false if illegal.
	 */
	virtual bool ApplyDrop(const FCFRBoardState& Board, int32 X, int32 Z, FCFRBoardState& OutBoard) const override;

	/**
	 * @brief Evaluates the board and returns the current game result.
	 *
	 * Checks Player1Wins → Player2Wins → Draw (no legal moves) → Ongoing.
	 *
	 * @param Board  The board state to evaluate.
	 * @return       ECFRGameResult value.
	 */
	virtual ECFRGameResult CheckResult(const FCFRBoardState& Board) const override;

protected:

	/**
	 * @brief Generic consecutive-piece checker shared by all rule subclasses.
	 *
	 * Iterates every cell; for each cell owned by Player, walks ConnectN-1 steps
	 * in every supplied direction. Returns true on the first complete run found.
	 *
	 * Subclasses pass their own direction list — 2D needs 4 directions,
	 * 3D needs 13 — the loop body is written only once here.
	 *
	 * @param Board       The board state to inspect.
	 * @param Player      Player index (1 or 2).
	 * @param Directions  Array of unit step vectors to check.
	 * @param ConnectN    Number of consecutive pieces required to win (default 4).
	 * @return            True if a winning run exists.
	 */
	static bool CheckConsecutive(
		const FCFRBoardState&        Board,
		int32                        Player,
		TArrayView<const FIntVector> Directions,
		int32                        ConnectN = 4
	);
};