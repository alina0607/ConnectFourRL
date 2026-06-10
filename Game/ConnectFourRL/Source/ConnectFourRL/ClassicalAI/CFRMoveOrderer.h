/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#pragma once

#include "CoreMinimal.h"
#include "../CFRBoardState.h"
#include "CFRMove.h"

class UCFRGameRulesBase;
enum class ECFRGameResult : uint8;

/** Orders promising Connect Four moves first to improve alpha-beta pruning. */
class CONNECTFOURRL_API FCFRMoveOrderer
{
public:
	void OrderMoves(
		const FCFRBoardState& Board,
		const UCFRGameRulesBase& Rules,
		TArray<FCFRMove>& Moves) const;

private:
	static bool DidPlayerWin(ECFRGameResult Result, int32 Player);

	int32 ScoreMove(
		const FCFRBoardState& Board,
		const UCFRGameRulesBase& Rules,
		const FCFRMove& Move) const;

	bool ApplyMove(
		const FCFRBoardState& Board,
		const UCFRGameRulesBase& Rules,
		const FCFRMove& Move,
		FCFRBoardState& OutBoard) const;
};
