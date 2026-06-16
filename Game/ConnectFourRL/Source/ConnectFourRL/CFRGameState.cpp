// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameState.h"
#include "Net/UnrealNetwork.h"

void ACFRGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACFRGameState, Board);
	DOREPLIFETIME(ACFRGameState, bGameOver);
	DOREPLIFETIME(ACFRGameState, CellSize);
	DOREPLIFETIME(ACFRGameState, CellHeight);
	DOREPLIFETIME(ACFRGameState, BoardOrigin);
}

FVector ACFRGameState::GetCellWorldPosition(int32 X, int32 Y, int32 Z) const
{
	// Keep this in sync with ACFRGameMode::GetCellWorldPosition.
	if (Board.SizeZ == 1)
	{
		// 2D modes: flat board on the ground (World Z = 0).
		return BoardOrigin + FVector(X * CellSize, Y * CellSize, 0.f);
	}

	// 3D mode: flat base grid; pieces stack upward along World Z.
	return BoardOrigin + FVector(X * CellSize, Z * CellSize, Y * CellHeight);
}

void ACFRGameState::OnRep_Board()
{
	// Phase 2: clients react here (hover highlight, debug grid, result UI).
	// Phase 1 renders pieces via replicated ACFRPiece actors, so this is a stub.
}
