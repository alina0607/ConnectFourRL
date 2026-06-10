/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 */

#pragma once

#include "CoreMinimal.h"
#include "AlphaBetaTypes.h"

/**
 * Reusable minimax search with alpha-beta pruning.
 *
 * GameType supplies the game-specific operations:
 * - GetCurrentPlayer(State)
 * - GenerateMoves(State, OutMoves)
 * - ApplyMove(State, Move, OutChildState)
 * - IsTerminal(State)
 * - Evaluate(State, PerspectivePlayer, RemainingDepth)
 * - OrderMoves(State, Moves)
 */
template<typename StateType, typename MoveType, typename GameType>
class TAlphaBetaSearch
{
public:
	TAlphaBetaResult<MoveType> FindBestMove(
		const StateType& RootState,
		const GameType& Game,
		const FAlphaBetaSettings& Settings) const
	{
		TAlphaBetaResult<MoveType> Result;

		if (Settings.MaxDepth <= 0 || Game.IsTerminal(RootState))
		{
			return Result;
		}

		TArray<MoveType> Moves;
		Game.GenerateMoves(RootState, Moves);
		Game.OrderMoves(RootState, Moves);

		const int32 RootPlayer = Game.GetCurrentPlayer(RootState);
		double Alpha = -Infinity;
		const double Beta = Infinity;

		for (const MoveType& Move : Moves)
		{
			StateType ChildState;
			if (!Game.ApplyMove(RootState, Move, ChildState))
			{
				continue;
			}

			const double Score = Search(
				ChildState,
				Settings.MaxDepth - 1,
				Alpha,
				Beta,
				RootPlayer,
				Game,
				Result.Statistics);

			if (!Result.bHasMove || Score > Result.Score)
			{
				Result.BestMove = Move;
				Result.Score = Score;
				Result.bHasMove = true;
			}

			Alpha = FMath::Max(Alpha, Score);
		}

		return Result;
	}

private:
	static constexpr double Infinity = 1.0e15;

	double Search(
		const StateType& State,
		int32 Depth,
		double Alpha,
		double Beta,
		int32 RootPlayer,
		const GameType& Game,
		FAlphaBetaStatistics& Statistics) const
	{
		++Statistics.NodesVisited;

		if (Depth <= 0 || Game.IsTerminal(State))
		{
			++Statistics.LeafNodes;
			return Game.Evaluate(State, RootPlayer, Depth);
		}

		TArray<MoveType> Moves;
		Game.GenerateMoves(State, Moves);
		Game.OrderMoves(State, Moves);

		if (Moves.IsEmpty())
		{
			++Statistics.LeafNodes;
			return Game.Evaluate(State, RootPlayer, Depth);
		}

		const bool bMaximizing = Game.GetCurrentPlayer(State) == RootPlayer;
		double BestScore = bMaximizing ? -Infinity : Infinity;
		bool bAppliedMove = false;

		for (const MoveType& Move : Moves)
		{
			StateType ChildState;
			if (!Game.ApplyMove(State, Move, ChildState))
			{
				continue;
			}

			bAppliedMove = true;

			const double Score = Search(
				ChildState,
				Depth - 1,
				Alpha,
				Beta,
				RootPlayer,
				Game,
				Statistics);

			if (bMaximizing)
			{
				BestScore = FMath::Max(BestScore, Score);
				Alpha = FMath::Max(Alpha, BestScore);
			}
			else
			{
				BestScore = FMath::Min(BestScore, Score);
				Beta = FMath::Min(Beta, BestScore);
			}

			if (Alpha >= Beta)
			{
				++Statistics.Cutoffs;
				break;
			}
		}

		if (!bAppliedMove)
		{
			++Statistics.LeafNodes;
			return Game.Evaluate(State, RootPlayer, Depth);
		}

		return BestScore;
	}
};
