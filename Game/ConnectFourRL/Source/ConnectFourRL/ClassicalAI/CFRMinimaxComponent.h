/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../CFRBoardState.h"
#include "CFRMove.h"
#include "CFRMinimaxComponent.generated.h"

class UCFRGameRulesBase;

/**
 * UCFRMinimaxComponent: Unreal-facing entry point for the reusable minimax
 * alpha-beta search.
 */
UCLASS(ClassGroup = "Connect Four AI", meta = (BlueprintSpawnableComponent))
class CONNECTFOURRL_API UCFRMinimaxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCFRMinimaxComponent();

	/** Maximum number of plies searched. Start low in 2D free-placement mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connect Four|AI",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 SearchDepth = 4;

	/**
	 * Searches a board without modifying it.
	 *
	 * Rules must match Board.GameMode. Returns false if the board is terminal,
	 * no legal move exists, or Rules is null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Connect Four|AI")
	bool FindBestMove(
		const FCFRBoardState& Board,
		UCFRGameRulesBase* Rules,
		FCFRMove& OutMove,
		float& OutScore,
		int64& OutNodesVisited,
		int64& OutCutoffs) const;
};
