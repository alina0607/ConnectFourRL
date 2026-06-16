// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRPlayerController.h"
#include "CFRGameMode.h"
#include "CFRGameState.h"
#include "CFRHoverIndicator.h"
#include "Net/UnrealNetwork.h"

ACFRPlayerController::ACFRPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Show the OS mouse cursor so the player can see which column they are hovering.
	bShowMouseCursor = true;

	// Allow UPrimitiveComponent::OnClicked delegates to fire (kept for compatibility).
	bEnableClickEvents = true;
}

void ACFRPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACFRPlayerController, PlayerNumber);
}

void ACFRPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Each client spawns its own local hover indicator (not replicated).
	if (IsLocalController() && HoverIndicatorClass)
	{
		HoverIndicator = GetWorld()->SpawnActor<ACFRHoverIndicator>(HoverIndicatorClass);
	}
}

void ACFRPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Input is meaningful only for the locally controlled player. On the listen
	// server the host also holds the remote client's controller — skip it here.
	if (!IsLocalController()) { return; }

	TraceToBoard();
	UpdateHoverIndicator();

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		OnClickBoard();
	}

	if (WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		Server_SwitchGameMode();
	}
}

void ACFRPlayerController::TraceToBoard()
{
	// Clear hover first so any early-out (no hit / bad ray) leaves no stale column.
	HoveredColumn = -1;

	// Clients read the replicated GameState — the GameMode does not exist here.
	ACFRGameState* GS = GetWorld()->GetGameState<ACFRGameState>();
	if (!GS) { return; }

	// Deproject the 2D mouse position into a 3D world-space ray.
	FVector RayOrigin, RayDirection;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection)) { return; }

	// Avoid division by zero when the ray is nearly parallel to the ground.
	if (FMath::IsNearlyZero(RayDirection.Z)) { return; }

	const float T = (GS->BoardOrigin.Z - RayOrigin.Z) / RayDirection.Z;
	if (T <= 0.f) { return; }

	const FVector HitPoint = RayOrigin + RayDirection * T;

	const FCFRBoardState& Board = GS->Board;

	const int32 Col = FMath::FloorToInt((HitPoint.X - GS->BoardOrigin.X) / GS->CellSize);
	const int32 WorldYIndex = FMath::FloorToInt((HitPoint.Y - GS->BoardOrigin.Y) / GS->CellSize);

	int32 Row   = 0;
	int32 Depth = 0;

	if (Board.UsesFreePlacement())
	{
		// 2D free-placement: hover an individual cell (X, Y).
		Row = WorldYIndex;
	}
	else if (Board.SizeZ == 1)
	{
		// 2D classic: choose a column; preview the gravity landing row.
		Row = Board.IsInBounds(Col, 0, 0) ? Board.GetDropRow(Col, 0) : -1;
	}
	else
	{
		// 3D gravity-drop: choose an (X, Z) column on the base grid.
		Depth = WorldYIndex;
	}

	HoveredColumn = Col;
	HoveredRow    = Row;
	HoveredDepth  = Depth;

	// Host-only debug hover: the GameMode exists solely on the authority.
	// (Phase 2 moves the hover highlight client-side via the GameState.)
	if (ACFRGameMode* GM = GetWorld()->GetAuthGameMode<ACFRGameMode>())
	{
		GM->SetHoveredCell(Col, Row, Depth);
	}
}

void ACFRPlayerController::OnClickBoard()
{
	ACFRGameState* GS = GetWorld()->GetGameState<ACFRGameState>();
	if (!GS || GS->bGameOver || HoveredColumn < 0) { return; }

	const FCFRBoardState& Board = GS->Board;

	if (Board.UsesFreePlacement())
	{
		Server_TryPlace(HoveredColumn, HoveredRow, 0);
	}
	else if (Board.SizeZ == 1)
	{
		Server_TryDrop(HoveredColumn, 0);
	}
	else
	{
		Server_TryDrop(HoveredColumn, HoveredDepth);
	}
}

void ACFRPlayerController::Server_TryDrop_Implementation(int32 X, int32 Z)
{
	ACFRGameMode*  GM = GetWorld()->GetAuthGameMode<ACFRGameMode>();
	ACFRGameState* GS = GetWorld()->GetGameState<ACFRGameState>();
	if (!GM || !GS) { return; }

	// Reject moves that are not this player's turn.
	if (PlayerNumber != GS->Board.CurrentPlayer) { return; }

	GM->TryDrop(X, Z);
}

void ACFRPlayerController::Server_TryPlace_Implementation(int32 X, int32 Y, int32 Z)
{
	ACFRGameMode*  GM = GetWorld()->GetAuthGameMode<ACFRGameMode>();
	ACFRGameState* GS = GetWorld()->GetGameState<ACFRGameState>();
	if (!GM || !GS) { return; }

	if (PlayerNumber != GS->Board.CurrentPlayer) { return; }

	GM->TryPlace(X, Y, Z);
}

void ACFRPlayerController::Server_SwitchGameMode_Implementation()
{
	if (ACFRGameMode* GM = GetWorld()->GetAuthGameMode<ACFRGameMode>())
	{
		GM->SwitchGameMode();
	}
}

void ACFRPlayerController::UpdateHoverIndicator()
{
	if (!HoverIndicator) { return; }

	ACFRGameState* GS = GetWorld()->GetGameState<ACFRGameState>();

	// Hide while there is no valid hover, the game is over, or the column is out of range.
	if (!GS || GS->bGameOver || HoveredColumn < 0 || HoveredColumn >= GS->Board.SizeX)
	{
		HoverIndicator->Update(false, false, FVector::ZeroVector, FVector::ZeroVector);
		return;
	}

	const FCFRBoardState& Board = GS->Board;
	const float Thin = 10.f; // flat highlight thickness for 2D modes

	FVector Center;
	FVector Size;
	bool    bLegal;

	if (Board.UsesFreePlacement())
	{
		// 2D free: single cell at (Col, Row).
		if (HoveredRow < 0 || HoveredRow >= Board.SizeY)
		{
			HoverIndicator->Update(false, false, FVector::ZeroVector, FVector::ZeroVector);
			return;
		}
		Center = GS->GetCellWorldPosition(HoveredColumn, HoveredRow, 0);
		Size   = FVector(GS->CellSize, GS->CellSize, Thin);
		bLegal = Board.IsLegalPlace(HoveredColumn, HoveredRow, 0);
	}
	else if (Board.SizeZ == 1)
	{
		// 2D classic: whole column spanning the rows along World Y.
		const FVector A = GS->GetCellWorldPosition(HoveredColumn, 0, 0);
		const FVector B = GS->GetCellWorldPosition(HoveredColumn, Board.SizeY - 1, 0);
		Center = (A + B) * 0.5f;
		Size   = FVector(GS->CellSize, GS->CellSize * Board.SizeY, Thin);
		bLegal = Board.IsLegalDrop(HoveredColumn, 0);
	}
	else
	{
		// 3D: whole vertical column spanning the rows along World Z.
		if (HoveredDepth < 0 || HoveredDepth >= Board.SizeZ)
		{
			HoverIndicator->Update(false, false, FVector::ZeroVector, FVector::ZeroVector);
			return;
		}
		const FVector A = GS->GetCellWorldPosition(HoveredColumn, 0, HoveredDepth);
		const FVector B = GS->GetCellWorldPosition(HoveredColumn, Board.SizeY - 1, HoveredDepth);
		Center = (A + B) * 0.5f;
		Size   = FVector(GS->CellSize, GS->CellSize, GS->CellHeight * Board.SizeY);
		bLegal = Board.IsLegalDrop(HoveredColumn, HoveredDepth);
	}

	HoverIndicator->Update(true, bLegal, Center, Size);
}
