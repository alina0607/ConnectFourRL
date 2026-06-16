// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CFRBoardState.generated.h"

/**
 * @brief Represents which player occupies a cell, or whether it is empty.
 */
UENUM(BlueprintType)
enum class ECFRCell : uint8
{
	Empty	 UMETA(DisplayName = "Empty"),
	Player1  UMETA(DisplayName = "Player 1"),
	Player2  UMETA(DisplayName = "Player 2")
};

/**
 * @brief Represents the current game mode.
 *
 * @note Mode2D        — 7x6x1 board, free placement (pick any empty cell).
 *       Mode2DClassic — 7x6x1 board, gravity column-drop (classic Connect Four).
 *       Mode3D        — 4x4x4 board, gravity column-drop.
 */
UENUM(BlueprintType)
enum class ECFRGameMode : uint8
{
	Mode2D        UMETA(DisplayName = "2D"),
	Mode2DClassic UMETA(DisplayName = "2D Classic"),
	Mode3D        UMETA(DisplayName = "3D")
};

/**
 * @brief Stores the complete state of the game board in a flat array.
 *
 * Supports both 2D and 3D modes using the same data structure and index formula.
 * Replicated as a whole by ACFRGameState — this struct itself has no network logic.
 *
 * Coordinate system:
 *   X = column  (left  -> right)
 *   Y = row     (bottom -> top, gravity direction)
 *   Z = depth   (front -> back, always 0 in 2D mode)
 *
 * Cell index formula: X + (Y * SizeX) + (Z * SizeX * SizeY)
 *
 * Default dimensions:
 *   2D: SizeX=7, SizeY=6, SizeZ=1
 *   3D: SizeX=4, SizeY=4, SizeZ=4
 */
USTRUCT(BlueprintType)
struct CONNECTFOURRL_API FCFRBoardState
{
	GENERATED_BODY()

	/** @brief Number of columns (X axis). */
	UPROPERTY(BlueprintReadOnly)
	int32 SizeX = 7;

	/** @brief Number of rows (Y axis, gravity direction). */
	UPROPERTY(BlueprintReadOnly)
	int32 SizeY = 6;

	/** @brief Depth of the board (Z axis). Always 1 in 2D mode. */
	UPROPERTY(BlueprintReadOnly)
	int32 SizeZ = 1;

	/** @brief Current game mode (2D or 3D). Set by Init(). */
	UPROPERTY(BlueprintReadOnly)
	ECFRGameMode GameMode = ECFRGameMode::Mode2D;

	/**
	 * @brief Flat array storing every cell's state.
	 * @note  Size = SizeX * SizeY * SizeZ.
	 */
	UPROPERTY(BlueprintReadOnly)
	TArray<ECFRCell> Cells;

	/** @brief Index of the player whose turn it is (1 or 2). */
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayer = 1;

	/** @brief Total number of moves made so far in this game. */
	UPROPERTY(BlueprintReadOnly)
	int32 MoveCount = 0;

	/**
	 * @brief Column (X), row (Y), and depth (Z) of the most recently placed piece.
	 *
	 * Set by ApplyDrop after every move. Used by CheckConsecutive to restrict
	 * win detection to the last played cell, reducing complexity from
	 * O(W * H * D * Directions * N) to O(Directions * N).
	 *
	 * Initialized to -1 to indicate no move has been made yet.
	 */
	UPROPERTY(BlueprintReadOnly)
	int32 LastMoveX = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 LastMoveY = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 LastMoveZ = -1;

	/**
	 * @brief Initializes the board with explicit dimensions.
	 *
	 * Board dimensions are owned by the rule set (UCFRGameRulesBase),
	 * not hardcoded here. Call this from CreateInitialBoard() only.
	 *
	 * @param Mode  The game mode (2D or 3D).
	 * @param InSizeX  Number of columns.
	 * @param InSizeY  Number of rows (gravity direction).
	 * @param InSizeZ  Depth (1 for 2D mode).
	 */
	void Init(ECFRGameMode Mode, int32 InSizeX, int32 InSizeY, int32 InSizeZ)
	{
		GameMode = Mode;
		SizeX    = InSizeX;
		SizeY    = InSizeY;
		SizeZ    = InSizeZ;

		Cells.Init(ECFRCell::Empty, SizeX * SizeY * SizeZ);
		CurrentPlayer = 1;
		MoveCount     = 0;
		LastMoveX     = -1;
		LastMoveY     = -1;
		LastMoveZ     = -1;
	}

	/**
	 * @brief Returns the cell state at position (X, Y, Z).
	 *
	 * @param X  Column index (0 to SizeX-1).
	 * @param Y  Row index    (0 to SizeY-1).
	 * @param Z  Depth index  (0 to SizeZ-1), always 0 in 2D mode.
	 * @return   The cell state, or ECFRCell::Empty if out of bounds.
	 */
	ECFRCell GetCell(int32 X, int32 Y, int32 Z) const
	{
		if (!IsInBounds(X, Y, Z)) return ECFRCell::Empty;
		return Cells[X + (Y * SizeX) + (Z * SizeX * SizeY)];
	}

	/**
	 * @brief Sets the cell state at position (X, Y, Z).
	 *
	 * Does nothing if the coordinates are out of bounds.
	 *
	 * @param X      Column index (0 to SizeX-1).
	 * @param Y      Row index    (0 to SizeY-1).
	 * @param Z      Depth index  (0 to SizeZ-1), always 0 in 2D mode.
	 * @param Value  The cell state to write.
	 */
	void SetCell(int32 X, int32 Y, int32 Z, ECFRCell Value)
	{
		if (!IsInBounds(X, Y, Z)) return;
		Cells[X + (Y * SizeX) + (Z * SizeX * SizeY)] = Value;
	}

	/**
	 * @brief Returns true if (X, Y, Z) is within the board boundaries.
	 *
	 * @param X  Column index to check.
	 * @param Y  Row index to check.
	 * @param Z  Depth index to check.
	 * @return   True if all three indices are within [0, Size-1].
	 */
	bool IsInBounds(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && X < SizeX
			&& Y >= 0 && Y < SizeY
			&& Z >= 0 && Z < SizeZ;
	}

	/**
	 * @brief Returns the lowest empty row (Y) in the column defined by (X, Z).
	 *
	 * Implements the gravity / drop mechanic shared by both 2D and 3D modes.
	 * In 2D mode, always pass Z = 0.
	 *
	 * @param X  Column index.
	 * @param Z  Depth index (0 in 2D mode).
	 * @return   The lowest empty Y index, or -1 if the column is full.
	 */
	int32 GetDropRow(int32 X, int32 Z) const
	{
		for (int32 Y = 0; Y < SizeY; Y++)
		{
			if (GetCell(X, Y, Z) == ECFRCell::Empty)
			{
				return Y;
			}
		}
		return -1; // Column is full
	}

	/**
	 * @brief Returns true if the active mode lets the player place a piece in any
	 *        empty cell (X, Y) directly, bypassing gravity.
	 *
	 * Only Mode2D uses free placement. Mode2DClassic and Mode3D both use the
	 * gravity column-drop mechanic, so the player chooses a column and the row
	 * is determined by the lowest empty cell.
	 *
	 * @return True for Mode2D; false for Mode2DClassic and Mode3D.
	 */
	bool UsesFreePlacement() const
	{
		return GameMode == ECFRGameMode::Mode2D;
	}

	/**
	 * @brief Returns the number of distinct actions for this mode (policy head size).
	 *
	 * Free placement (Mode2D): every cell is an action      -> SizeX * SizeY.
	 * Column drop (Classic / 3D): every (X, Z) column is one -> SizeX * SizeZ
	 *   (Classic has SizeZ == 1, so this collapses to SizeX).
	 *
	 * @return The flat action-space size used to index the policy vector.
	 */
	int32 GetActionSpaceSize() const
	{
		return UsesFreePlacement() ? (SizeX * SizeY) : (SizeX * SizeZ);
	}

	/**
	 * @brief Encodes a move into a flat action index matching GetActionSpaceSize().
	 *
	 * Free placement encodes the chosen cell (X, Y); column drop encodes the
	 * chosen column (X, Z) and ignores Y (gravity determines the row).
	 * The encoding is shared with the Python training pipeline — keep both sides
	 * identical.
	 *
	 * @param X  Column index.
	 * @param Y  Row index (used only in free placement).
	 * @param Z  Depth index (used only in column drop; always 0 for 2D modes).
	 * @return   The flat action index.
	 */
	int32 EncodeAction(int32 X, int32 Y, int32 Z) const
	{
		return UsesFreePlacement() ? (X + Y * SizeX) : (X + Z * SizeX);
	}

	/**
	 * @brief Returns true if a gravity drop at column (X, Z) is legal.
	 *
	 * Board-level query (no rule set required) so clients can evaluate legality
	 * from the replicated board alone — used for hover feedback.
	 *
	 * @param X  Column index.
	 * @param Z  Depth index (0 in 2D modes).
	 */
	bool IsLegalDrop(int32 X, int32 Z) const
	{
		return IsInBounds(X, 0, Z) && GetDropRow(X, Z) != -1;
	}

	/**
	 * @brief Returns true if placing at cell (X, Y, Z) is legal (in bounds & empty).
	 *
	 * Board-level query for 2D free placement; usable on clients for hover feedback.
	 *
	 * @param X  Column index.
	 * @param Y  Row index.
	 * @param Z  Depth index (0 in 2D modes).
	 */
	bool IsLegalPlace(int32 X, int32 Y, int32 Z) const
	{
		return IsInBounds(X, Y, Z) && GetCell(X, Y, Z) == ECFRCell::Empty;
	}
};
