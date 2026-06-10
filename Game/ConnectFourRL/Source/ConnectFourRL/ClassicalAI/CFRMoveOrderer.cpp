/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#include "CFRMoveOrderer.h"
#include "../CFRGameRulesBase.h"

void FCFRMoveOrderer::OrderMoves(
	const FCFRBoardState& Board,
	const UCFRGameRulesBase& Rules,
	TArray<FCFRMove>& Moves) const
{
	Moves.Sort(
		[this, &Board, &Rules](const FCFRMove& A, const FCFRMove& B)
		{
			return ScoreMove(Board, Rules, A) > ScoreMove(Board, Rules, B);
		});
}

bool FCFRMoveOrderer::DidPlayerWin(ECFRGameResult Result, int32 Player)
{
	return
		(Player == 1 && Result == ECFRGameResult::Player1Wins) ||
		(Player == 2 && Result == ECFRGameResult::Player2Wins);
}

int32 FCFRMoveOrderer::ScoreMove(
	const FCFRBoardState& Board,
	const UCFRGameRulesBase& Rules,
	const FCFRMove& Move) const
{
	FCFRBoardState Child;
	if (!ApplyMove(Board, Rules, Move, Child))
	{
		return MIN_int32;
	}

	int32 Score = 0;
	if (DidPlayerWin(Rules.CheckResult(Child), Board.CurrentPlayer))
	{
		Score += 100'000;
	}

	const double CenterX = (Board.SizeX - 1) * 0.5;
	const double CenterZ = (Board.SizeZ - 1) * 0.5;
	Score -= FMath::RoundToInt(FMath::Abs(Move.X - CenterX) * 10.0);
	Score -= FMath::RoundToInt(FMath::Abs(Move.Z - CenterZ) * 10.0);

	return Score;
}

bool FCFRMoveOrderer::ApplyMove(
	const FCFRBoardState& Board,
	const UCFRGameRulesBase& Rules,
	const FCFRMove& Move,
	FCFRBoardState& OutBoard) const
{
	if (Board.GameMode == ECFRGameMode::Mode2D)
	{
		return Rules.ApplyPlace(Board, Move.X, Move.Y, Move.Z, OutBoard);
	}

	return Rules.ApplyDrop(Board, Move.X, Move.Z, OutBoard);
}
