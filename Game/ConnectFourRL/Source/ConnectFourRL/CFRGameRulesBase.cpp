// Fill out your copyright notice in the Description page of Project Settings.


#include "CFRGameRulesBase.h"

FCFRBoardState UCFRGameRulesBase::CreateInitialBoard() const
{
	FCFRBoardState Board;
	Board.Init(GetGameMode(), BoardSizeX, BoardSizeY, BoardSizeZ);
	return Board;
}

bool UCFRGameRulesBase::CheckConsecutive(
	const FCFRBoardState&        Board,
	int32                        Player,
	TArrayView<const FIntVector> Directions,
	int32                        ConnectN)
{
	// A win can only be created by the piece that was just placed.
	// Restrict the search to that single cell instead of scanning the whole board.
	// Complexity: O(D * N)  vs  naive O(W * H * D * N).
	if (Board.LastMoveX == -1) { return false; }

	const ECFRCell Target = (Player == 1) ? ECFRCell::Player1 : ECFRCell::Player2;
	const int32 X = Board.LastMoveX;
	const int32 Y = Board.LastMoveY;
	const int32 Z = Board.LastMoveZ;

	for (const FIntVector& Dir : Directions)
	{
		// Bidirectional scan: walk +Dir and -Dir from the placed piece.
		// Summing both sides handles wins where the piece completes a run
		// from the middle (e.g. ■■ X ■ → Count = 4).
		int32 Count = 1;

		for (int32 Step = 1; Step < ConnectN; ++Step)
		{
			if (Board.GetCell(X + Dir.X * Step, Y + Dir.Y * Step, Z + Dir.Z * Step) != Target) { break; }
			++Count;
		}
		for (int32 Step = 1; Step < ConnectN; ++Step)
		{
			if (Board.GetCell(X - Dir.X * Step, Y - Dir.Y * Step, Z - Dir.Z * Step) != Target) { break; }
			++Count;
		}

		if (Count >= ConnectN) { return true; }
	}

	return false;
}

ECFRGameResult UCFRGameRulesBase::CheckResult(const FCFRBoardState& Board) const
{
	// A win can only be created by the piece most recently placed.
	// ApplyDrop advances CurrentPlayer before returning, so the last mover
	// is now the opponent of CurrentPlayer.
	const int32 LastPlayer = (Board.CurrentPlayer == 1) ? 2 : 1;

	if (HasPlayerWon(Board, LastPlayer))
	{
		return (LastPlayer == 1) ? ECFRGameResult::Player1Wins : ECFRGameResult::Player2Wins;
	}

	if (GetLegalMoves(Board).IsEmpty()) { return ECFRGameResult::Draw; }
	return ECFRGameResult::Ongoing;
}

TArray<FIntPoint> UCFRGameRulesBase::GetLegalMoves(const FCFRBoardState& Board) const
{
	TArray<FIntPoint> Moves;

	for (int32 Z = 0; Z < Board.SizeZ; ++Z)
	{
		for (int32 X = 0; X < Board.SizeX; ++X)
		{
			if (IsLegalDrop(Board, X, Z))
			{
				Moves.Add(FIntPoint(X, Z));
			}
		}
	}

	return Moves;
}

bool UCFRGameRulesBase::ApplyDrop(const FCFRBoardState& Board, int32 X, int32 Z, FCFRBoardState& OutBoard) const
{
	if (!IsLegalDrop(Board, X, Z))
	{
		return false;
	}

	// Deep-copy the input board so the original stays untouched (required for MCTS branching).
	OutBoard = Board;

	const int32 Row = OutBoard.GetDropRow(X, Z);
	const ECFRCell Piece = (OutBoard.CurrentPlayer == 1) ? ECFRCell::Player1 : ECFRCell::Player2;
	OutBoard.SetCell(X, Row, Z, Piece);

	// Record position for O(D*N) win detection in CheckConsecutive.
	OutBoard.LastMoveX = X;
	OutBoard.LastMoveY = Row;
	OutBoard.LastMoveZ = Z;

	// Advance turn: 1 → 2 → 1 → ...
	OutBoard.CurrentPlayer = (OutBoard.CurrentPlayer == 1) ? 2 : 1;
	OutBoard.MoveCount++;

	return true;
}

bool UCFRGameRulesBase::IsLegalDrop(const FCFRBoardState& Board, int32 X, int32 Z) const
{
	return Board.IsLegalDrop(X, Z);
}

bool UCFRGameRulesBase::IsLegalPlace(const FCFRBoardState& Board, int32 X, int32 Y, int32 Z) const
{
	return Board.IsLegalPlace(X, Y, Z);
}

bool UCFRGameRulesBase::ApplyPlace(
	const FCFRBoardState& Board,
	int32 X, int32 Y, int32 Z,
	FCFRBoardState& OutBoard) const
{
	if (!IsLegalPlace(Board, X, Y, Z)) { return false; }

	// Deep-copy so the caller's board is not mutated (required for MCTS branching).
	OutBoard = Board;

	const ECFRCell Piece = (OutBoard.CurrentPlayer == 1) ? ECFRCell::Player1 : ECFRCell::Player2;
	OutBoard.SetCell(X, Y, Z, Piece);

	// Record position for O(D*N) win detection in CheckConsecutive.
	OutBoard.LastMoveX = X;
	OutBoard.LastMoveY = Y;
	OutBoard.LastMoveZ = Z;

	// Advance turn: 1 → 2 → 1 → ...
	OutBoard.CurrentPlayer = (OutBoard.CurrentPlayer == 1) ? 2 : 1;
	OutBoard.MoveCount++;

	return true;
}