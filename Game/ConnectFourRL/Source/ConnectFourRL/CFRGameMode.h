// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CFRBoardState.h"
#include "CFRGameRules.h"
#include "CFRGameMode.generated.h"

class UCFRGameRules2D;

/**
 * @brief Game mode that owns the rule set and live board state for a Connect Four session.
 *
 * Responsibilities:
 *   - Creates and holds the UCFRGameRules2D instance at BeginPlay.
 *   - Processes player move requests via TryDrop().
 *   - Converts board coordinates to Unreal world positions for visual piece placement.
 *   - Fires Blueprint events when a piece is placed or the game ends.
 */
UCLASS()
class CONNECTFOURRL_API ACFRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACFRGameMode();

	// ------------------------------------------------------------------
	// Game state
	// ------------------------------------------------------------------

	/** @brief The active rule set. Created at BeginPlay via NewObject. */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	TObjectPtr<UCFRGameRules2D> Rules;

	/** @brief The live board state updated after every accepted move. */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	FCFRBoardState CurrentBoard;

	// ------------------------------------------------------------------
	// Visual configuration
	// ------------------------------------------------------------------

	/** @brief Distance between cell centres in Unreal world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Visual")
	float CellSize = 100.f;

	/** @brief World-space position of board cell (0, 0, 0). Set this in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Visual")
	FVector BoardOrigin = FVector::ZeroVector;

	// ------------------------------------------------------------------
	// Game logic — called by ACFRColumnActor on player click
	// ------------------------------------------------------------------

	/**
	 * @brief Attempts to drop the current player's piece in column (X, Z).
	 *
	 * Validates the move, updates CurrentBoard, fires OnPiecePlaced,
	 * and fires OnGameEnded if the game has reached a terminal state.
	 *
	 * @param X  Column index (0 to SizeX-1).
	 * @param Z  Depth index; always 0 in 2D mode.
	 * @return   True if the move was accepted and applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool TryDrop(int32 X, int32 Z = 0);

	/**
	 * @brief Returns the Unreal world position of board cell (X, Y, Z).
	 *
	 * Coordinate mapping:
	 *   Board X (column) → World Y  (left / right)
	 *   Board Y (row)    → World Z  (up / down)
	 *   Board Z (depth)  → World X  (always 0 in 2D mode)
	 *
	 * @param X  Board column index.
	 * @param Y  Board row index (gravity direction).
	 * @param Z  Board depth index.
	 */
	UFUNCTION(BlueprintCallable, Category = "Board|Visual")
	FVector GetCellWorldPosition(int32 X, int32 Y, int32 Z) const;

	// ------------------------------------------------------------------
	// Blueprint events — implement visuals in BP_CFRGameMode
	// ------------------------------------------------------------------

	/**
	 * @brief Fired after a piece is successfully placed.
	 * Blueprint should spawn the donut mesh at GetCellWorldPosition(X, Y, Z).
	 *
	 * @param X       Column index of the placed piece.
	 * @param Y       Row index (the gravity-resolved drop row).
	 * @param Z       Depth index (always 0 in 2D).
	 * @param Player  The player who placed the piece (1 or 2).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void OnPiecePlaced(int32 X, int32 Y, int32 Z, int32 Player);

	/**
	 * @brief Fired when the game reaches a terminal state.
	 * Blueprint should display the result (win / draw) UI.
	 *
	 * @param Result  The final game result.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void OnGameEnded(ECFRGameResult Result);

protected:

	virtual void BeginPlay() override;
};
