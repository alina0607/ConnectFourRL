// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameRules2D.h"

ECFRGameMode UCFRGameRules2D::GetGameMode() const
{
	return ECFRGameMode::Mode2D;
}

bool UCFRGameRules2D::HasPlayerWon(const FCFRBoardState& Board, int32 Player) const
{
	static const FIntVector Directions[] =
	{
		{ 1,  0, 0 },   // horizontal
		{ 0,  1, 0 },   // vertical
		{ 1,  1, 0 },   // diagonal up-right
		{ 1, -1, 0 },   // diagonal down-right
	};

	return CheckConsecutive(Board, Player, Directions);
}
