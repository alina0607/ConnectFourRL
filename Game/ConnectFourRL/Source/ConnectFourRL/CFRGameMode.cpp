// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameMode.h"
#include "CFRGameRules2D.h"
#include "CFRGameRules2DClassic.h"
#include "CFRGameRules3D.h"
#include "CFRGameRulesBase.h"
#include "CFRPlayerController.h"
#include "CFRPiece.h"
#include "CFRGameRecorder.h"
#include "CFRGameState.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

ACFRGameMode::ACFRGameMode()
{
	PlayerControllerClass = ACFRPlayerController::StaticClass();
	GameStateClass        = ACFRGameState::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ACFRGameMode::BeginPlay()
{
	Super::BeginPlay();

	Rules        = NewObject<UCFRGameRules2D>(this);
	CurrentBoard = Rules->CreateInitialBoard();
	bGameOver    = false;

	// Mirror static visual parameters into the replicated GameState once, then
	// keep the board in sync after every move.
	CFRGameState = GetWorld()->GetGameState<ACFRGameState>();
	if (CFRGameState)
	{
		CFRGameState->CellSize    = CellSize;
		CFRGameState->CellHeight  = CellHeight;
		CFRGameState->BoardOrigin = BoardOrigin;
	}
	SyncStateToGameState();

	if (bRecordGames)
	{
		Recorder = NewObject<UCFRGameRecorder>(this);
		Recorder->StartGame(CurrentBoard);
	}

	OnGameStarted(ECFRGameMode::Mode2D);
}

void ACFRGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Hand out player numbers in join order: 1, 2, then 3+ (extra = spectators
	// since the turn check only accepts the current player's number).
	if (ACFRPlayerController* PC = Cast<ACFRPlayerController>(NewPlayer))
	{
		PC->PlayerNumber = ++AssignedPlayers;
	}
}

void ACFRGameMode::SyncStateToGameState()
{
	if (!CFRGameState) { return; }

	CFRGameState->Board     = CurrentBoard;
	CFRGameState->bGameOver = bGameOver;
}

void ACFRGameMode::SwitchGameMode()
{
	// Destroy all pieces currently on the board.
	TArray<AActor*> Pieces;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACFRPiece::StaticClass(), Pieces);
	for (AActor* Piece : Pieces) { Piece->Destroy(); }

	// Cycle rule set: Mode2D -> Mode2DClassic -> Mode3D -> Mode2D.
	switch (Rules->GetGameMode())
	{
	case ECFRGameMode::Mode2D:
		Rules = NewObject<UCFRGameRules2DClassic>(this);
		break;
	case ECFRGameMode::Mode2DClassic:
		Rules = NewObject<UCFRGameRules3D>(this);
		break;
	default: // Mode3D
		Rules = NewObject<UCFRGameRules2D>(this);
		break;
	}

	CurrentBoard   = Rules->CreateInitialBoard();
	HoveredColumn  = -1;
	HoveredRow     = 0;
	HoveredDepth   = 0;
	bGameOver      = false;

	// Switching mode discards the unfinished game and starts a fresh record.
	if (Recorder) { Recorder->StartGame(CurrentBoard); }

	SyncStateToGameState();
	OnGameStarted(Rules->GetGameMode());
}

bool ACFRGameMode::TryDrop(int32 X, int32 Z)
{
	if (!Rules || bGameOver || !HasAuthority()) { return false; }

	// Capture the active player and pre-move board before ApplyDrop advances the turn.
	const int32 ActivePlayer = CurrentBoard.CurrentPlayer;
	const FCFRBoardState BoardBefore = CurrentBoard;

	FCFRBoardState NextBoard;
	if (!Rules->ApplyDrop(CurrentBoard, X, Z, NextBoard)) { return false; }

	CurrentBoard = NextBoard;
	SyncStateToGameState();

	// Record the move (human play -> one-hot policy). LastMoveY is the landing row.
	if (Recorder) { Recorder->RecordMove(BoardBefore, X, CurrentBoard.LastMoveY, Z); }

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
		bGameOver = true;
		SyncStateToGameState();
		if (Recorder) { Recorder->EndGame(Result); }
		ShowDebugResult(Result);
		OnGameEnded(Result);
	}

	return true;
}

bool ACFRGameMode::TryPlace(int32 X, int32 Y, int32 Z)
{
	if (!Rules || bGameOver || !HasAuthority()) { return false; }

	const int32 ActivePlayer = CurrentBoard.CurrentPlayer;
	const FCFRBoardState BoardBefore = CurrentBoard;

	FCFRBoardState NextBoard;
	if (!Rules->ApplyPlace(CurrentBoard, X, Y, Z, NextBoard)) { return false; }

	CurrentBoard = NextBoard;
	SyncStateToGameState();

	// Record the move (human play -> one-hot policy).
	if (Recorder) { Recorder->RecordMove(BoardBefore, X, Y, Z); }

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
		bGameOver = true;
		SyncStateToGameState();
		if (Recorder) { Recorder->EndGame(Result); }
		ShowDebugResult(Result);
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

		const bool bFree       = CurrentBoard.UsesFreePlacement();
		const bool bColHovered = (HoveredColumn >= 0 && HoveredColumn < CurrentBoard.SizeX);
		const bool bCellValid  = (bColHovered
			                   && HoveredRow >= 0 && HoveredRow < CurrentBoard.SizeY);

		for (int32 Y = 0; Y < CurrentBoard.SizeY; ++Y)
		{
			for (int32 X = 0; X < CurrentBoard.SizeX; ++X)
			{
				const FVector Centre = GetCellWorldPosition(X, Y, 0);

				FColor Color = FColor::Cyan;
				if (bFree)
				{
					// Free placement: highlight only the single hovered cell (X, Y).
					if (X == HoveredColumn && Y == HoveredRow && bCellValid)
					{
						const bool bLegal = Rules->IsLegalPlace(CurrentBoard, X, Y, 0);
						Color = bLegal ? FColor::Green : FColor::Red;
					}
				}
				else if (X == HoveredColumn && bColHovered)
				{
					// Classic column-drop: highlight the whole hovered column —
					// green if the column still has room, red if it is full.
					const bool bLegal = Rules->IsLegalDrop(CurrentBoard, X, 0);
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
		// 3D mode — flat 4 × 4 base grid on the ground; the hovered column is
		// highlighted as a full vertical stack so the player can see exactly
		// which (X, Z) column the piece will drop into.
		// Z half-extent = CellHeight * 0.5f so the box visually matches the
		// flat donut mesh proportions.  Adjust CellHeight in the Details
		// panel to match your actual mesh height.
		// ----------------------------------------------------------------
		const float PieceHalfH  = FMath::Max(1.f, CellHeight * 0.5f);
		const FVector BoxExtent3D(VisualHalf, VisualHalf, PieceHalfH);

		// Base grid: one cyan box per (X, Z) column with its coordinate label.
		for (int32 Z = 0; Z < CurrentBoard.SizeZ; ++Z)
		{
			for (int32 X = 0; X < CurrentBoard.SizeX; ++X)
			{
				const FVector Centre = GetCellWorldPosition(X, 0, Z);
				DrawDebugBox(GetWorld(), Centre, BoxExtent3D, FColor::Cyan, false, -1.f, 0, 1.5f);

				const FString Label = FString::Printf(TEXT("%d,%d"), X, Z);
				DrawDebugString(GetWorld(), Centre + FVector(0.f, 0.f, PieceHalfH + 5.f),
					Label, nullptr, FColor::Yellow, 0.f, false, 0.8f);
			}
		}

		// Hovered column: highlight the entire vertical stack (all rows) — green
		// if a drop is still legal, red if the column is full.
		const bool bColHovered = (HoveredColumn >= 0 && HoveredColumn < CurrentBoard.SizeX
			                   && HoveredDepth  >= 0 && HoveredDepth  < CurrentBoard.SizeZ);
		if (bColHovered)
		{
			const bool bLegal = Rules->IsLegalDrop(CurrentBoard, HoveredColumn, HoveredDepth);
			const FColor Color = bLegal ? FColor::Green : FColor::Red;
			for (int32 Y = 0; Y < CurrentBoard.SizeY; ++Y)
			{
				const FVector Centre = GetCellWorldPosition(HoveredColumn, Y, HoveredDepth);
				DrawDebugBox(GetWorld(), Centre, BoxExtent3D, Color, false, -1.f, 0, 1.5f);
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

void ACFRGameMode::ShowDebugResult(ECFRGameResult Result) const
{
	if (!GEngine) { return; }

	FString  Message;
	FColor   Color = FColor::White;

	switch (Result)
	{
	case ECFRGameResult::Player1Wins: Message = TEXT("Player 1 Wins!"); Color = FColor::Yellow; break;
	case ECFRGameResult::Player2Wins: Message = TEXT("Player 2 Wins!"); Color = FColor::Cyan;   break;
	case ECFRGameResult::Draw:        Message = TEXT("Draw!");          Color = FColor::White;  break;
	default:                          return; // Ongoing — nothing to show.
	}

	// Key -1 appends a fresh line; shown for 5 seconds. Remove once BP_CFRGameMode has result UI.
	GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Message);
}
