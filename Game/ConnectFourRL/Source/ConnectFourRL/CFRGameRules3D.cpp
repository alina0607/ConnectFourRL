// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameRules3D.h"

UCFRGameRules3D::UCFRGameRules3D()
{
	// Override base class defaults to match the 3D board dimensions.
	BoardSizeX = 4;
	BoardSizeY = 4;
	BoardSizeZ = 4;
}

ECFRGameMode UCFRGameRules3D::GetGameMode() const
{
	return ECFRGameMode::Mode3D;
}

bool UCFRGameRules3D::HasPlayerWon(const FCFRBoardState& Board, int32 Player) const
{
	static const FIntVector Directions[] =
	{
		// Face-to-face axes (3)
		{ 1,  0,  0 },
		{ 0,  1,  0 },
		{ 0,  0,  1 },

		// Face diagonals — XY plane (2)
		{ 1,  1,  0 },
		{ 1, -1,  0 },

		// Face diagonals — XZ plane (2)
		{ 1,  0,  1 },
		{ 1,  0, -1 },

		// Face diagonals — YZ plane (2)
		{ 0,  1,  1 },
		{ 0,  1, -1 },

		// Space diagonals — corner to corner through the cube (4)
		{ 1,  1,  1 },
		{ 1,  1, -1 },
		{ 1, -1,  1 },
		{ 1, -1, -1 },
	};

	return CheckConsecutive(Board, Player, Directions);
}
