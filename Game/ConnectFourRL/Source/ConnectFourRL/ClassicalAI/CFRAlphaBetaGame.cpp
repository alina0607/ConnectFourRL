/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#include "CFRAlphaBetaGame.h"
#include "../CFRGameRulesBase.h"

FCFRAlphaBetaGame::FCFRAlphaBetaGame(const UCFRGameRulesBase& InRules)
	: Rules(InRules)
{
}

int32 FCFRAlphaBetaGame::GetCurrentPlayer(const FCFRBoardState& Board) const
{
	return Board.CurrentPlayer;
}

void FCFRAlphaBetaGame::GenerateMoves(
	const FCFRBoardState& Board,
	TArray<FCFRMove>& OutMoves) const
{
	OutMoves.Reset();

	if (Board.GameMode == ECFRGameMode::Mode2D)
	{
		for (int32 Y = 0; Y < Board.SizeY; ++Y)
		{
			for (int32 X = 0; X < Board.SizeX; ++X)
			{
				if (Rules.IsLegalPlace(Board, X, Y, 0))
				{
					OutMoves.Emplace(X, Y, 0);
				}
			}
		}
		return;
	}

	for (const FIntPoint& Drop : Rules.GetLegalMoves(Board))
	{
		OutMoves.Emplace(Drop.X, 0, Drop.Y);
	}
}

bool FCFRAlphaBetaGame::ApplyMove(
	const FCFRBoardState& Board,
	const FCFRMove& Move,
	FCFRBoardState& OutBoard) const
{
	if (Board.GameMode == ECFRGameMode::Mode2D)
	{
		return Rules.ApplyPlace(Board, Move.X, Move.Y, Move.Z, OutBoard);
	}

	return Rules.ApplyDrop(Board, Move.X, Move.Z, OutBoard);
}

bool FCFRAlphaBetaGame::IsTerminal(const FCFRBoardState& Board) const
{
	return Rules.CheckResult(Board) != ECFRGameResult::Ongoing;
}

double FCFRAlphaBetaGame::Evaluate(
	const FCFRBoardState& Board,
	int32 PerspectivePlayer,
	int32 RemainingDepth) const
{
	return Evaluator.Evaluate(Board, Rules, PerspectivePlayer, RemainingDepth);
}

void FCFRAlphaBetaGame::OrderMoves(
	const FCFRBoardState& Board,
	TArray<FCFRMove>& Moves) const
{
	MoveOrderer.OrderMoves(Board, Rules, Moves);
}
