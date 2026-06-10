/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 *
 * CFR: Connect Four Reinforcement Learning
 */

#include "CFRBoardEvaluator.h"
#include "../CFRGameRulesBase.h"

ECFRCell FCFRBoardEvaluator::CellForPlayer(int32 Player)
{
	return Player == 1 ? ECFRCell::Player1 : ECFRCell::Player2;
}

int32 FCFRBoardEvaluator::OpponentOf(int32 Player)
{
	return Player == 1 ? 2 : 1;
}

double FCFRBoardEvaluator::Evaluate(
	const FCFRBoardState& Board,
	const UCFRGameRulesBase& Rules,
	int32 PerspectivePlayer,
	int32 RemainingDepth) const
{
	const ECFRGameResult Result = Rules.CheckResult(Board);
	if (Result == ECFRGameResult::Draw)
	{
		return 0.0;
	}

	const bool bPerspectiveWon =
		(PerspectivePlayer == 1 && Result == ECFRGameResult::Player1Wins) ||
		(PerspectivePlayer == 2 && Result == ECFRGameResult::Player2Wins);

	if (Result != ECFRGameResult::Ongoing)
	{
		const double DepthBonus = FMath::Max(0, RemainingDepth);
		return bPerspectiveWon
			? WinScore + DepthBonus
			: -WinScore - DepthBonus;
	}

	return ScorePlayer(Board, PerspectivePlayer)
		- ScorePlayer(Board, OpponentOf(PerspectivePlayer));
}

double FCFRBoardEvaluator::ScorePlayer(
	const FCFRBoardState& Board,
	int32 Player) const
{
	static const FIntVector Directions2D[] =
	{
		{ 1, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, -1, 0 }
	};

	static const FIntVector Directions3D[] =
	{
		{ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
		{ 1, 1, 0 }, { 1, -1, 0 },
		{ 1, 0, 1 }, { 1, 0, -1 },
		{ 0, 1, 1 }, { 0, 1, -1 },
		{ 1, 1, 1 }, { 1, 1, -1 },
		{ 1, -1, 1 }, { 1, -1, -1 }
	};

	const TArrayView<const FIntVector> Directions =
		Board.GameMode == ECFRGameMode::Mode2D
			? MakeArrayView(Directions2D)
			: MakeArrayView(Directions3D);

	double Score = 0.0;

	for (int32 Z = 0; Z < Board.SizeZ; ++Z)
	{
		for (int32 Y = 0; Y < Board.SizeY; ++Y)
		{
			for (int32 X = 0; X < Board.SizeX; ++X)
			{
				for (const FIntVector& Direction : Directions)
				{
					Score += ScoreLine(Board, X, Y, Z, Direction, Player);
				}
			}
		}
	}

	return Score;
}

double FCFRBoardEvaluator::ScoreLine(
	const FCFRBoardState& Board,
	int32 StartX,
	int32 StartY,
	int32 StartZ,
	const FIntVector& Direction,
	int32 Player) const
{
	int32 PlayerCells = 0;
	int32 EmptyCells = 0;
	const ECFRCell Target = CellForPlayer(Player);

	for (int32 Step = 0; Step < 4; ++Step)
	{
		const int32 X = StartX + Direction.X * Step;
		const int32 Y = StartY + Direction.Y * Step;
		const int32 Z = StartZ + Direction.Z * Step;

		if (!Board.IsInBounds(X, Y, Z))
		{
			return 0.0;
		}

		const ECFRCell Cell = Board.GetCell(X, Y, Z);
		if (Cell == Target)
		{
			++PlayerCells;
		}
		else if (Cell == ECFRCell::Empty)
		{
			++EmptyCells;
		}
		else
		{
			return 0.0;
		}
	}

	if (PlayerCells == 3 && EmptyCells == 1) { return 1'000.0; }
	if (PlayerCells == 2 && EmptyCells == 2) { return 50.0; }
	if (PlayerCells == 1 && EmptyCells == 3) { return 2.0; }
	return 0.0;
}
