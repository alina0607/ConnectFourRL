/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#pragma once

#include "CoreMinimal.h"
#include "CFRMove.generated.h"

/**
 * FCFRMove: Connect Four Reinforcement Learning Move.
 *
 * Describes one move for all supported board modes.
 * The 2D free-placement mode uses X and Y.
 * The 3D gravity mode uses X and Z; gravity determines Y.
 */
USTRUCT(BlueprintType)
struct CONNECTFOURRL_API FCFRMove
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connect Four|AI")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connect Four|AI")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connect Four|AI")
	int32 Z = 0;

	FCFRMove() = default;

	FCFRMove(int32 InX, int32 InY, int32 InZ)
		: X(InX)
		, Y(InY)
		, Z(InZ)
	{
	}
};
