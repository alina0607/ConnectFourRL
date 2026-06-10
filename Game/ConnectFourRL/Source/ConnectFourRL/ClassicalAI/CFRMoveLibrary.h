/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CFRMove.h"
#include "CFRMoveLibrary.generated.h"

/**
 * UCFRMoveLibrary: Connect Four Reinforcement Learning Move Library.
 *
 * Exposes FCFRMove construction to Blueprints and gives Unreal's C++ Classes
 * browser a reflected class to display inside the ClassicalAI source folder.
 */
UCLASS()
class CONNECTFOURRL_API UCFRMoveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Creates a Connect Four move from board coordinates. */
	UFUNCTION(BlueprintPure, Category = "Connect Four|AI")
	static FCFRMove MakeCFRMove(int32 X, int32 Y, int32 Z);
};
