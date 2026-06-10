// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CFRGameRulesBase.h"
#include "CFRGameRules3D.generated.h"

/**
 * @brief Concrete rule set for 3D Connect Four (4 × 4 × 4).
 *
 * Inherits all shared logic from UCFRGameRulesBase.
 * Only overrides GetGameMode() and HasPlayerWon() with 13 direction vectors
 * that cover every possible winning axis in a 3D grid.
 */
UCLASS()
class CONNECTFOURRL_API UCFRGameRules3D : public UCFRGameRulesBase
{
	GENERATED_BODY()

public:

	UCFRGameRules3D();

	/** @brief Returns ECFRGameMode::Mode3D to identify this rule set. */
	virtual ECFRGameMode GetGameMode() const override;

	/**
	 * @brief Returns true if the given player has four consecutive pieces in any 3D direction.
	 *
	 * Directions checked (13 unique axes in a 3D grid):
	 *   3 face-to-face axes  (+X, +Y, +Z)
	 *   6 face diagonals     (XY, XZ, YZ planes, both orientations)
	 *   4 space diagonals    (corner-to-corner through the cube)
	 *
	 * @param Board   The board state to inspect.
	 * @param Player  Player index (1 or 2).
	 * @return        True if that player has won.
	 */
	virtual bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const override;
};
