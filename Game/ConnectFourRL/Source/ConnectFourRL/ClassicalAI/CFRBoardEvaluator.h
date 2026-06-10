/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#pragma once

#include "CoreMinimal.h"
#include "../CFRBoardState.h"

class UCFRGameRulesBase;

/**
 * FCFRBoardEvaluator: Connect Four Reinforcement Learning Board Evaluator.
 *
 * Converts terminal and non-terminal board positions into scores from one
 * player's perspective.
 */
class CONNECTFOURRL_API FCFRBoardEvaluator
{
public:
	double Evaluate(
		const FCFRBoardState& Board,
		const UCFRGameRulesBase& Rules,
		int32 PerspectivePlayer,
		int32 RemainingDepth) const;

private:
	static constexpr double WinScore = 1'000'000.0;

	static ECFRCell CellForPlayer(int32 Player);
	static int32 OpponentOf(int32 Player);

	double ScorePlayer(
		const FCFRBoardState& Board,
		int32 Player) const;

	double ScoreLine(
		const FCFRBoardState& Board,
		int32 StartX,
		int32 StartY,
		int32 StartZ,
		const FIntVector& Direction,
		int32 Player) const;
};
