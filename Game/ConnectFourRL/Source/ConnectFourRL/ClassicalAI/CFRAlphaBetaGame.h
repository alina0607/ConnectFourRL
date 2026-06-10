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
#include "CFRBoardEvaluator.h"
#include "CFRMoveOrderer.h"

class UCFRGameRulesBase;

/**
 * Connect Four adapter that supplies the operations required by
 * TAlphaBetaSearch.
 */
class CONNECTFOURRL_API FCFRAlphaBetaGame
{
public:
	explicit FCFRAlphaBetaGame(const UCFRGameRulesBase& InRules);

	int32 GetCurrentPlayer(const FCFRBoardState& Board) const;
	void GenerateMoves(const FCFRBoardState& Board, TArray<FCFRMove>& OutMoves) const;
	bool ApplyMove(const FCFRBoardState& Board, const FCFRMove& Move, FCFRBoardState& OutBoard) const;
	bool IsTerminal(const FCFRBoardState& Board) const;
	double Evaluate(const FCFRBoardState& Board, int32 PerspectivePlayer, int32 RemainingDepth) const;
	void OrderMoves(const FCFRBoardState& Board, TArray<FCFRMove>& Moves) const;

private:
	const UCFRGameRulesBase& Rules;
	FCFRBoardEvaluator Evaluator;
	FCFRMoveOrderer MoveOrderer;
};
