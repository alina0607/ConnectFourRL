/*
 * Author: Ju-ve Chankasemporn
 * E-mail: juvereturn@gmail.com
 */

#pragma once

#include "CoreMinimal.h"

/** Settings for one reusable minimax alpha-beta search. */
struct FAlphaBetaSettings
{
	/** Maximum number of plies (individual player moves) searched from the root. */
	int32 MaxDepth = 4;
};

/** Diagnostic counters collected during one search. */
struct FAlphaBetaStatistics
{
	int64 NodesVisited = 0;
	int64 LeafNodes = 0;
	int64 Cutoffs = 0;
};

/** Result returned by TAlphaBetaSearch for any supplied move type. */
template<typename MoveType>
struct TAlphaBetaResult
{
	MoveType BestMove{};
	double Score = 0.0;
	bool bHasMove = false;
	FAlphaBetaStatistics Statistics;
};
