// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CFRBoardState.generated.h"

/**
 * Represents which player occupies a cell, or whether it is empty.
 */
UENUM(BlueprintType)
enum class ECFRCell : uint8
{
	Empty	 UMETA(DisplayName = "Empty"),
	Player1  UMETA(DisplayName = "Player 1"),
	Player2  UMETA(DisplayName = "Player 2")
};

/**
 * Represents the current game mode.
 * 2D: classic 6x7 Connect Four (SizeZ is always 1).
 * 3D: 4x4x4 Connect Four.
 */
UENUM(BlueprintType)
enum class ECFRGameMode : uint8
{
	Mode2D  UMETA(DisplayName = "2D"),
	Mode3D  UMETA(DisplayName = "3D")
};

/**
 * FCFRBoardState
 *
 * Stores the complete state of the game board in a flat array.
 * Supports both 2D and 3D modes using the same data structure.
 *
 * Coordinate system:
 *   X = column  (left  -> right)
 *   Y = row     (bottom -> top, gravity direction)
 *   Z = depth   (front -> back, always 0 in 2D mode)
 *
 * Cell index formula: X + (Y * SizeX) + (Z * SizeX * SizeY)
 *
 * 2D default: SizeX=7, SizeY=6, SizeZ=1
 * 3D default: SizeX=4, SizeY=4, SizeZ=4
 */
USTRUCT(BlueprintType)
struct CONNECTFOURRL_API FCFRBoardState
{
	GENERATED_BODY()

	// Board dimensions
	UPROPERTY(BlueprintReadOnly)
	int32 SizeX = 7;

	UPROPERTY(BlueprintReadOnly)
	int32 SizeY = 6;

	UPROPERTY(BlueprintReadOnly)
	int32 SizeZ = 1;

	// Current game mode (2D or 3D)
	UPROPERTY(BlueprintReadOnly)
	ECFRGameMode GameMode = ECFRGameMode::Mode2D;

	// Flat array storing every cell's state
	// Size = SizeX * SizeY * SizeZ
	UPROPERTY(BlueprintReadOnly)
	TArray<ECFRCell> Cells;

	// Which player's turn it is (1 or 2)
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayer = 1;

	// Total number of moves made so far
	UPROPERTY(BlueprintReadOnly)
	int32 MoveCount = 0;

	/**
	 * Initializes the board for the given game mode.
	 * Clears all cells and resets turn to Player 1.
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
	 * Returns the cell state at position (X, Y, Z).
	 * Returns Empty if coordinates are out of bounds.
	 */
	ECFRCell GetCell(int32 X, int32 Y, int32 Z) const
	{
		if (!IsInBounds(X, Y, Z)) return ECFRCell::Empty;
		return Cells[X + (Y * SizeX) + (Z * SizeX * SizeY)];
	}

	/**
	 * Sets the cell state at position (X, Y, Z).
	 * Does nothing if coordinates are out of bounds.
	 */
	void SetCell(int32 X, int32 Y, int32 Z, ECFRCell Value)
	{
		if (!IsInBounds(X, Y, Z)) return;
		Cells[X + (Y * SizeX) + (Z * SizeX * SizeY)] = Value;
	}

	/**
	 * Returns true if (X, Y, Z) is within the board boundaries.
	 */
	bool IsInBounds(int32 X, int32 Y, int32 Z) const
	{
		return X >= 0 && X < SizeX
			&& Y >= 0 && Y < SizeY
			&& Z >= 0 && Z < SizeZ;
	}

	/**
	 * Returns the lowest empty row (Y) in the column defined by (X, Z).
	 * Returns -1 if the column is full.
	 * This is the gravity / drop mechanic shared by both 2D and 3D modes.
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
