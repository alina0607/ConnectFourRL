// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CFRBoardState.h"
#include "CFRGameRules.h"
#include "CFRGameRecorder.generated.h"

/**
 * @brief A single recorded move: the state before the move, the action taken,
 *        the search policy over the action space, and the value target.
 *
 * The tuple (Board, Policy, Value) is exactly one AlphaZero training sample.
 * Value is left at 0 until EndGame() back-fills it from the final result.
 *
 * @note Plain struct (no reflection) — internal working data of UCFRGameRecorder.
 */
struct FCFRRecordedMove
{
	/** @brief 1-based index of this move within the game. */
	int32 MoveNumber = 0;

	/** @brief Player who made this move (1 or 2). */
	int32 Player = 0;

	/** @brief Board state BEFORE the move — flat ECFRCell values as bytes. */
	TArray<uint8> Board;

	/** @brief Flat action index taken (see FCFRBoardState::EncodeAction). */
	int32 Action = -1;

	/** @brief Human-readable coordinates of the placed piece. */
	int32 ActionX = -1;
	int32 ActionY = -1;
	int32 ActionZ = -1;

	/**
	 * @brief Visit-count distribution over the full action space (policy target).
	 *
	 * For MCTS self-play these are the raw root visit counts; the trainer
	 * normalises them. For human play this is a one-hot at the chosen action.
	 */
	TArray<int32> Policy;

	/** @brief Game outcome from this move's player perspective: +1 win, -1 loss, 0 draw. */
	float Value = 0.f;
};

/**
 * @brief Records every move of a game and exports it as JSON training data.
 *
 * Owned and driven by ACFRGameMode. Lifecycle:
 *   StartGame()  — begin a fresh record (assigns a new game id, captures mode/dims).
 *   RecordMove() — append one move after it has been applied to the board.
 *   EndGame()    — back-fill value targets and write the file to disk.
 *
 * The JSON schema is shared with the Python AlphaZero pipeline; only completed
 * games are written. Switching mode mid-game discards the unfinished record.
 */
UCLASS()
class CONNECTFOURRL_API UCFRGameRecorder : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * @brief Begins recording a new game. Discards any in-progress record.
	 *
	 * Captures the mode, board dimensions, and action-space size from the
	 * initial board, and assigns a fresh game id.
	 *
	 * @param InitialBoard  The empty board the game will start from.
	 */
	void StartGame(const FCFRBoardState& InitialBoard);

	/**
	 * @brief Appends one move to the current record.
	 *
	 * The mover and move number are derived from BoardBefore (its CurrentPlayer
	 * has not yet advanced). The action index is computed via the board's
	 * mode-aware EncodeAction.
	 *
	 * @param BoardBefore  Board state immediately BEFORE the move was applied.
	 * @param X            Column of the placed piece.
	 * @param Y            Row of the placed piece (final landing row for drops).
	 * @param Z            Depth of the placed piece (0 in 2D modes).
	 * @param VisitCounts  Policy over the action space. Pass an empty array for
	 *                     human play to record a one-hot at the chosen action.
	 */
	void RecordMove(
		const FCFRBoardState& BoardBefore,
		int32                 X,
		int32                 Y,
		int32                 Z,
		const TArray<int32>&  VisitCounts = TArray<int32>());

	/**
	 * @brief Finalises the game: back-fills value targets and writes the JSON file.
	 *
	 * Does nothing if no game is active or no moves were recorded.
	 *
	 * @param FinalResult  The terminal game result.
	 */
	void EndGame(ECFRGameResult FinalResult);

private:

	/** @brief Builds the complete JSON document for the recorded game. */
	FString BuildJson(ECFRGameResult FinalResult) const;

	/** @brief Maps a game mode to its schema string ("2D" / "2D-Classic" / "3D"). */
	static FString ModeToString(ECFRGameMode InMode);

	/** @brief Maps a result enum to its schema string. */
	static FString ResultToString(ECFRGameResult Result);

	/** @brief Joins a byte array into a comma-separated list for JSON. */
	static FString JoinBytes(const TArray<uint8>& Values);

	/** @brief Joins an int array into a comma-separated list for JSON. */
	static FString JoinInts(const TArray<int32>& Values);

	/** @brief Unique id for the current game (hex digits). */
	FString GameId;

	/** @brief Mode and dimensions captured at StartGame. */
	ECFRGameMode Mode = ECFRGameMode::Mode2D;
	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 SizeZ = 0;
	int32 ActionSpace = 0;

	/** @brief All moves recorded so far this game. */
	TArray<FCFRRecordedMove> Moves;

	/** @brief True between StartGame and EndGame. */
	bool bActive = false;
};
