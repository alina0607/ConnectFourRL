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
 * @note Mode2D uses a 6x7x1 board (classic Connect Four).
 *       Mode3D uses a 4x4x4 board.
 */
UENUM(BlueprintType)
enum class ECFRGameMode : uint8
{
	Mode2D  UMETA(DisplayName = "2D"),
	Mode3D  UMETA(DisplayName = "3D")
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
	 * @brief Initializes the board for the given game mode.
	 *
	 * Sets board dimensions, clears all cells to Empty,
	 * and resets CurrentPlayer to 1 and MoveCount to 0.
	 *
	 * @param Mode  The game mode to initialize (Mode2D or Mode3D).
	 */
	void Init(ECFRGameMode Mode)
	{
		GameMode = Mode;

		if (Mode == ECFRGameMode::Mode2D)
		{
			SizeX = 7;
			SizeY = 6;
			SizeZ = 1;
		}
		else
		{
			SizeX = 4;
			SizeY = 4;
			SizeZ = 4;
		}

		Cells.Init(ECFRCell::Empty, SizeX * SizeY * SizeZ);
		CurrentPlayer = 1;
		MoveCount     = 0;
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
};
