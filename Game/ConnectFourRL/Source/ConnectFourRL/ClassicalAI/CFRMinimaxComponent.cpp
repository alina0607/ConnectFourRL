/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#include "CFRMinimaxComponent.h"
#include "AlphaBetaSearch.h"
#include "CFRAlphaBetaGame.h"
#include "../CFRGameRulesBase.h"

UCFRMinimaxComponent::UCFRMinimaxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCFRMinimaxComponent::FindBestMove(
	const FCFRBoardState& Board,
	UCFRGameRulesBase* Rules,
	FCFRMove& OutMove,
	float& OutScore,
	int64& OutNodesVisited,
	int64& OutCutoffs) const
{
	OutMove = FCFRMove();
	OutScore = 0.0f;
	OutNodesVisited = 0;
	OutCutoffs = 0;

	if (!IsValid(Rules))
	{
		return false;
	}

	FCFRAlphaBetaGame Game(*Rules);

	FAlphaBetaSettings Settings;
	Settings.MaxDepth = FMath::Max(1, SearchDepth);

	const TAlphaBetaSearch<
		FCFRBoardState,
		FCFRMove,
		FCFRAlphaBetaGame> Search;

	const TAlphaBetaResult<FCFRMove> Result =
		Search.FindBestMove(Board, Game, Settings);

	OutScore = static_cast<float>(Result.Score);
	OutNodesVisited = Result.Statistics.NodesVisited;
	OutCutoffs = Result.Statistics.Cutoffs;

	if (!Result.bHasMove)
	{
		return false;
	}

	OutMove = Result.BestMove;
	return true;
}
