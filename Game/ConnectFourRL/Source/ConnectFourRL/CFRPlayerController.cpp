// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRPlayerController.h"
#include "CFRGameMode.h"

ACFRPlayerController::ACFRPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Show the OS mouse cursor so the player can see which column they are hovering.
	bShowMouseCursor = true;

	// Allow UPrimitiveComponent::OnClicked delegates to fire (kept for compatibility).
	bEnableClickEvents = true;
}

void ACFRPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TraceToBoard();

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		OnClickBoard();
	}

	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		ACFRGameMode* GameMode = Cast<ACFRGameMode>(GetWorld()->GetAuthGameMode());
		if (GameMode) { GameMode->SwitchGameMode(); }
	}
}

void ACFRPlayerController::TraceToBoard()
{
	ACFRGameMode* GameMode = Cast<ACFRGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode) { return; }

	// Deproject the 2D mouse position into a 3D world-space ray.
	FVector RayOrigin, RayDirection;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection)) { return; }

	// Both 2D and 3D boards have their bottom face in the World XY plane
	// at Z = BoardOrigin.Z.  A single horizontal ray-plane intersection
	// therefore works for both modes:
	//
	//   2D — flat 7 × 6 slab on the ground.  HitPoint.(X,Y) → (col, row).
	//         Only col (X) is passed to TryDrop; gravity determines the row.
	//   3D — flat 4 × 4 base on the ground.  HitPoint.(X,Y) → (col, depth).
	//         Both col (X) and depth (Y→Board Z) are passed to TryDrop.
	//
	// Avoid division by zero when the ray is nearly parallel to the ground.
	if (FMath::IsNearlyZero(RayDirection.Z)) { return; }

	const float T = (GameMode->BoardOrigin.Z - RayOrigin.Z) / RayDirection.Z;

	// Negative T means the board is behind the camera — ignore.
	if (T <= 0.f) { return; }

	const FVector HitPoint = RayOrigin + RayDirection * T;

	// Board X always maps to World X (column index).
	const int32 Col = FMath::FloorToInt(
		(HitPoint.X - GameMode->BoardOrigin.X) / GameMode->CellSize);

	const bool b2D = (GameMode->CurrentBoard.SizeZ == 1);

	// 2D free-placement: Board Y (row) maps to World Y — extract from HitPoint.Y
	//   so the player can hover over individual cells (X, Y).
	// 3D gravity-drop:   Board Z (depth) maps to World Y — extract from HitPoint.Y
	//   so the player can choose which (X, Z) column to drop into.
	//   HoveredRow is not used in 3D (gravity determines the row after drop).
	const int32 Row = b2D
		? FMath::FloorToInt((HitPoint.Y - GameMode->BoardOrigin.Y) / GameMode->CellSize)
		: 0;
	const int32 Depth = b2D
		? 0
		: FMath::FloorToInt((HitPoint.Y - GameMode->BoardOrigin.Y) / GameMode->CellSize);

	HoveredColumn = Col;
	HoveredRow    = Row;
	HoveredDepth  = Depth;

	GameMode->SetHoveredCell(Col, Row, Depth);
}

void ACFRPlayerController::OnClickBoard()
{
	ACFRGameMode* GameMode = Cast<ACFRGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || HoveredColumn < 0) { return; }

	const bool b2D = (GameMode->CurrentBoard.SizeZ == 1);

	if (b2D)
	{
		// 2D free-placement: place at the exact (Col, Row) cell the player clicked.
		GameMode->TryPlace(HoveredColumn, HoveredRow, 0);
	}
	else
	{
		// 3D gravity-drop: drop into the (Col, Depth) column; gravity determines the row.
		GameMode->TryDrop(HoveredColumn, HoveredDepth);
	}
}
