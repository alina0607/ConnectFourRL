// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CFRGameRules2D.h"
#include "CFRGameRules2DClassic.generated.h"

/**
 * @brief Concrete rule set for classic 2D Connect Four (7 columns × 6 rows).
 *
 * Identical board geometry and win detection to UCFRGameRules2D (flat 7×6×1,
 * 4-direction win), but identified as Mode2DClassic so the input layer uses the
 * gravity column-drop mechanic (TryDrop) instead of free cell placement.
 *
 * Only overrides GetGameMode(); board sizing and HasPlayerWon() are inherited.
 */
UCLASS()
class CONNECTFOURRL_API UCFRGameRules2DClassic : public UCFRGameRules2D
{
	GENERATED_BODY()

public:

	/** @brief Returns ECFRGameMode::Mode2DClassic to identify this rule set. */
	virtual ECFRGameMode GetGameMode() const override;
};
