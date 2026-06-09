// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CFRGameRulesBase.h"
#include "CFRGameRules2D.generated.h"

/**
 * @brief Concrete rule set for 2D Connect Four (6 rows × 7 columns).
 *
 * Inherits all shared logic (CreateInitialBoard, IsLegalDrop, GetLegalMoves,
 * ApplyDrop, CheckResult) from UCFRGameRulesBase.
 *
 * Only implements the two pieces that differ per mode:
 *   - GetGameMode()  — identifies this rule set as Mode2D.
 *   - HasPlayerWon() — scans 4 directions for a run of 4 consecutive pieces.
 */
UCLASS()
class CONNECTFOURRL_API UCFRGameRules2D : public UCFRGameRulesBase
{
	GENERATED_BODY()

public:

	/** @brief Returns ECFRGameMode::Mode2D to identify this rule set. */
	virtual ECFRGameMode GetGameMode() const override;

	/**
	 * @brief Returns true if the given player has four consecutive pieces in any 2D direction.
	 *
	 * Directions checked (Z is always 0 in 2D mode):
	 *   - Horizontal      (+X)
	 *   - Vertical        (+Y)
	 *   - Diagonal up     (+X, +Y)
	 *   - Diagonal down   (+X, -Y)
	 *
	 * @param Board   The board state to inspect.
	 * @param Player  Player index (1 or 2).
	 * @return        True if that player has won.
	 */
	virtual bool HasPlayerWon(const FCFRBoardState& Board, int32 Player) const override;
};
