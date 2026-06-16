// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CFRBoardState.h"
#include "CFRGameState.generated.h"

/**
 * @brief Replicates the shared game state to all connected clients.
 *
 * ACFRGameMode (server only) owns the authoritative logic and pushes the board,
 * game-over flag, and visual parameters here after every change. Clients read
 * this class for rendering, input projection, and UI — they never touch the
 * GameMode (which does not exist on clients).
 */
UCLASS()
class CONNECTFOURRL_API ACFRGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	/** @brief Authoritative board, mirrored from the server's GameMode after each move. */
	UPROPERTY(ReplicatedUsing = OnRep_Board, BlueprintReadOnly, Category = "Game")
	FCFRBoardState Board;

	/** @brief True once the game has reached a terminal state. Blocks client input. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameOver = false;

	/** @brief Horizontal spacing between cell centres (World X / Y). Mirrored from GameMode. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Board|Visual")
	float CellSize = 150.f;

	/** @brief Vertical stacking step in 3D (World Z). Mirrored from GameMode. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Board|Visual")
	float CellHeight = 50.f;

	/** @brief World position of board cell (0,0,0). Mirrored from GameMode. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Board|Visual")
	FVector BoardOrigin = FVector::ZeroVector;

	/**
	 * @brief Returns the Unreal world position of board cell (X, Y, Z).
	 *
	 * Client-side mirror of ACFRGameMode::GetCellWorldPosition — the two MUST
	 * stay identical. The server uses the GameMode version for piece spawning;
	 * clients use this one for input projection and any local visuals.
	 *
	 * @param X  Board column index.
	 * @param Y  Board row index (gravity direction).
	 * @param Z  Board depth index (always 0 in 2D modes).
	 */
	UFUNCTION(BlueprintCallable, Category = "Board|Visual")
	FVector GetCellWorldPosition(int32 X, int32 Y, int32 Z) const;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** @brief Fires on clients when the board changes. Reserved for Phase 2 hover/UI updates. */
	UFUNCTION()
	void OnRep_Board();
};
