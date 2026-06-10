// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CFRGameState.generated.h"

/**
 * @brief Replicates the live board state to all connected clients.
 *
 * In a networked session, ACFRGameMode updates the board on the server.
 * ACFRGameState holds a Replicated copy of FCFRBoardState so every client
 * always sees the current position without separate RPCs per piece.
 *
 * An OnRep callback fires on each client when the board changes,
 * triggering local piece spawn and UI updates.
 *
 * @note Implementation is pending — this is a forward-declared shell.
 */
UCLASS()
class CONNECTFOURRL_API ACFRGameState : public AGameStateBase
{
	GENERATED_BODY()

	// Implementation will be added in the networking phase.
};
