// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CFRGameRecorder.generated.h"

/**
 * @brief Records every move in a game session and exports the result as JSON.
 *
 * Intended to be created by ACFRGameMode at the start of each game.
 * Each call to RecordMove() appends a move entry.
 * When the game ends, SaveToFile() writes the full game record to disk
 * for use as AI training data.
 *
 * @note Implementation is pending — this is a forward-declared shell.
 */
UCLASS()
class CONNECTFOURRL_API UCFRGameRecorder : public UObject
{
	GENERATED_BODY()

	// Implementation will be added in the AI data collection phase.
};
