// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameMode.h"
#include "CFRGameRules2D.h"

ACFRGameMode::ACFRGameMode()
{
}

void ACFRGameMode::BeginPlay()
{
	Super::BeginPlay();

	Rules = NewObject<UCFRGameRules2D>(this);
	CurrentBoard = Rules->CreateInitialBoard();
}

bool ACFRGameMode::TryDrop(int32 X, int32 Z)
{
	if (!Rules) { return false; }

	// Capture the active player before ApplyDrop advances the turn.
	const int32 ActivePlayer = CurrentBoard.CurrentPlayer;

	FCFRBoardState NextBoard;
	if (!Rules->ApplyDrop(CurrentBoard, X, Z, NextBoard)) { return false; }

	CurrentBoard = NextBoard;

	// LastMoveY holds the gravity-resolved row set by ApplyDrop.
	OnPiecePlaced(X, CurrentBoard.LastMoveY, Z, ActivePlayer);

	const ECFRGameResult Result = Rules->CheckResult(CurrentBoard);
	if (Result != ECFRGameResult::Ongoing)
	{
		OnGameEnded(Result);
	}

	return true;
}

FVector ACFRGameMode::GetCellWorldPosition(int32 X, int32 Y, int32 Z) const
{
	return BoardOrigin + FVector(
		Z * CellSize,   // board depth  → World X (always 0 in 2D)
		X * CellSize,   // board column → World Y (left / right)
		Y * CellSize    // board row    → World Z (up / down)
	);
}
