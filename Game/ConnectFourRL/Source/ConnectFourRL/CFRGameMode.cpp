// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameMode.h"
#include "CFRGameRules2D.h"
#include "CFRGameRules3D.h"
#include "CFRGameRulesBase.h"
#include "CFRPlayerController.h"
#include "CFRPiece.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ACFRGameMode::ACFRGameMode()
{
	PlayerControllerClass = ACFRPlayerController::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ACFRGameMode::BeginPlay()
{
	Super::BeginPlay();

	Rules        = NewObject<UCFRGameRules2D>(this);
	CurrentBoard = Rules->CreateInitialBoard();
}

void ACFRGameMode::SwitchGameMode()
{
	// Destroy all pieces currently on the board.
	TArray<AActor*> Pieces;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACFRPiece::StaticClass(), Pieces);
	for (AActor* Piece : Pieces) { Piece->Destroy(); }

	// Toggle rule set and reset board.
	if (Rules->GetGameMode() == ECFRGameMode::Mode2D)
	{
		Rules = NewObject<UCFRGameRules3D>(this);
	}
	else
	{
		Rules = NewObject<UCFRGameRules2D>(this);
	}

	CurrentBoard   = Rules->CreateInitialBoard();
	HoveredColumn  = -1;
	HoveredDepth   = 0;
}

bool ACFRGameMode::TryDrop(int32 X, int32 Z)
{
	if (!Rules) { return false; }

	// Capture the active player before ApplyDrop advances the turn.
	const int32 ActivePlayer = CurrentBoard.CurrentPlayer;

	FCFRBoardState NextBoard;
	if (!Rules->ApplyDrop(CurrentBoard, X, Z, NextBoard)) { return false; }

	CurrentBoard = NextBoard;

	// Spawn the piece above the column and let it fall to the target cell.
	TSubclassOf<ACFRPiece> PieceClass = (ActivePlayer == 1) ? PieceActorClassP1 : PieceActorClassP2;
	if (PieceClass && GetWorld())
	{
		const FVector TargetLocation = GetCellWorldPosition(X, CurrentBoard.LastMoveY, Z);

		// Always spawn at a fixed height above the board origin so pieces
		// consistently enter the frame from the same altitude regardless
		// of how many pieces are already stacked in this column.
		const FVector SpawnLocation = FVector(
			TargetLocation.X,
			TargetLocation.Y,
			BoardOrigin.Z + DropStartHeight
		);

		ACFRPiece* Piece = GetWorld()->SpawnActor<ACFRPiece>(PieceClass, SpawnLocation, FRotator::ZeroRotator);
		if (Piece)
		{
			Piece->StartFall(TargetLocation, PieceFallSpeed);
		}
	}

	const ECFRGameResult Result = Rules->CheckResult(CurrentBoard);
	if (Result != ECFRGameResult::Ongoing)
	{
		OnGameEnded(Result);
	}

	return true;
}

bool ACFRGameMode::TryPlace(int32 X, int32 Y, int32 Z)
{
	if (!Rules) { return false; }

	const int32 ActivePlayer = CurrentBoard.CurrentPlayer;

	FCFRBoardState NextBoard;
	if (!Rules->ApplyPlace(CurrentBoard, X, Y, Z, NextBoard)) { return false; }

	CurrentBoard = NextBoard;

	// Spawn the piece above the target cell and let it fall straight down.
	TSubclassOf<ACFRPiece> PieceClass = (ActivePlayer == 1) ? PieceActorClassP1 : PieceActorClassP2;
	if (PieceClass && GetWorld())
	{
		const FVector TargetLocation = GetCellWorldPosition(X, Y, Z);
		const FVector SpawnLocation  = FVector(
			TargetLocation.X,
			TargetLocation.Y,
			BoardOrigin.Z + DropStartHeight
		);

		ACFRPiece* Piece = GetWorld()->SpawnActor<ACFRPiece>(PieceClass, SpawnLocation, FRotator::ZeroRotator);
		if (Piece)
		{
			Piece->StartFall(TargetLocation, PieceFallSpeed);
		}
	}

	const ECFRGameResult Result = Rules->CheckResult(CurrentBoard);
	if (Result != ECFRGameResult::Ongoing)
	{
		OnGameEnded(Result);
	}

	return true;
}

void ACFRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDrawDebugGrid) { DrawDebugGrid(); }
}

void ACFRGameMode::SetHoveredCell(int32 Col, int32 Row, int32 Depth)
{
	HoveredColumn = Col;
	HoveredRow    = Row;
	HoveredDepth  = Depth;
}

void ACFRGameMode::DrawDebugGrid() const
{
	if (!GetWorld() || !Rules) { return; }

	// Visual half-extent of each cell box.
	// CellGap shrinks the drawn box relative to the grid spacing, creating visible separation.
	const float VisualHalf = FMath::Max(1.f, (CellSize - CellGap) * 0.5f);
	// The "thin" dimension (2 % of VisualHalf) so the slab doesn't obscure pieces.
	const float SlabHalf   = VisualHalf * 0.02f;

	const bool b2D = (CurrentBoard.SizeZ == 1);

	if (b2D)
	{
		// ----------------------------------------------------------------
		// 2D mode — flat 7 × 6 chess board on the ground.
		// Thin slab (SlabHalf in Z) since all pieces land at Z = 0.
		// Only the single hovered cell (X, Y) is highlighted.
		// ----------------------------------------------------------------
		const FVector BoxExtent2D(VisualHalf, VisualHalf, SlabHalf);

		const bool bCellValid = (HoveredColumn >= 0 && HoveredColumn < CurrentBoard.SizeX
			                  && HoveredRow    >= 0 && HoveredRow    < CurrentBoard.SizeY);

		for (int32 Y = 0; Y < CurrentBoard.SizeY; ++Y)
		{
			for (int32 X = 0; X < CurrentBoard.SizeX; ++X)
			{
				const FVector Centre = GetCellWorldPosition(X, Y, 0);

				FColor Color = FColor::Cyan;
				if (X == HoveredColumn && Y == HoveredRow && bCellValid)
				{
					const bool bLegal = Rules->IsLegalPlace(CurrentBoard, X, Y, 0);
					Color = bLegal ? FColor::Green : FColor::Red;
				}

				DrawDebugBox(GetWorld(), Centre, BoxExtent2D, Color, false, -1.f, 0, 1.5f);

				const FString Label = FString::Printf(TEXT("%d,%d"), X, Y);
				DrawDebugString(GetWorld(), Centre + FVector(0.f, 0.f, 5.f),
					Label, nullptr, FColor::Yellow, 0.f, false, 0.8f);
			}
		}
	}
	else
	{
		// ----------------------------------------------------------------
		// 3D mode — flat 4 × 4 base grid on the ground (Board Y = 0 only).
		// Z half-extent = CellHeight * 0.5f so the box visually matches the
		// flat donut mesh proportions.  Adjust CellHeight in the Details
		// panel to match your actual mesh height.
		// ----------------------------------------------------------------
		const float PieceHalfH  = FMath::Max(1.f, CellHeight * 0.5f);
		const FVector BoxExtent3D(VisualHalf, VisualHalf, PieceHalfH);

		for (int32 Z = 0; Z < CurrentBoard.SizeZ; ++Z)
		{
			for (int32 X = 0; X < CurrentBoard.SizeX; ++X)
			{
				const FVector Centre = GetCellWorldPosition(X, 0, Z);

				const bool bIsHovered = (X == HoveredColumn && Z == HoveredDepth);
				FColor Color = FColor::Cyan;
				if (bIsHovered)
				{
					const bool bLegal = Rules->IsLegalDrop(CurrentBoard, X, Z);
					Color = bLegal ? FColor::Green : FColor::Red;
				}

				DrawDebugBox(GetWorld(), Centre, BoxExtent3D, Color, false, -1.f, 0, 1.5f);

				const FString Label = FString::Printf(TEXT("%d,%d"), X, Z);
				DrawDebugString(GetWorld(), Centre + FVector(0.f, 0.f, PieceHalfH + 5.f),
					Label, nullptr, FColor::Yellow, 0.f, false, 0.8f);
			}
		}
	}
}

FVector ACFRGameMode::GetCellWorldPosition(int32 X, int32 Y, int32 Z) const
{
	if (CurrentBoard.SizeZ == 1)
	{
		// ---------------------------------------------------------------
		// 2D mode: the board is a flat horizontal slab lying on the ground
		// (World XY plane, Z = 0).  No vertical stacking — every piece
		// lands flush with the ground.
		//
		//   Board X (column) → World X  (left / right)
		//   Board Y (row)    → World Y  (front / back, gravity fills Y=0 first)
		//   Board Z = 0      → World Z  = 0  (always on the ground)
		// ---------------------------------------------------------------
		return BoardOrigin + FVector(
			X * CellSize,
			Y * CellSize,
			0.f
		);
	}
	else
	{
		// ---------------------------------------------------------------
		// 3D mode: the 4×4 base grid is flat on the ground; pieces stack
		// upward along World Z as they fill each (X, Z) column.
		//
		//   Board X (column) → World X  (left / right)
		//   Board Z (depth)  → World Y  (front / back)
		//   Board Y (row)    → World Z  (up, stacking direction)
		// ---------------------------------------------------------------
		return BoardOrigin + FVector(
			X * CellSize,    // Board X → World X  (horizontal, uses CellSize)
			Z * CellSize,    // Board Z → World Y  (horizontal, uses CellSize)
			Y * CellHeight   // Board Y → World Z  (vertical stacking, uses CellHeight)
		);
	}
}
