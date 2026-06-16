// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRGameRecorder.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

void UCFRGameRecorder::StartGame(const FCFRBoardState& InitialBoard)
{
	GameId      = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Mode        = InitialBoard.GameMode;
	SizeX       = InitialBoard.SizeX;
	SizeY       = InitialBoard.SizeY;
	SizeZ       = InitialBoard.SizeZ;
	ActionSpace = InitialBoard.GetActionSpaceSize();

	Moves.Reset();
	bActive = true;
}

void UCFRGameRecorder::RecordMove(
	const FCFRBoardState& BoardBefore,
	int32 X, int32 Y, int32 Z,
	const TArray<int32>& VisitCounts)
{
	if (!bActive) { return; }

	FCFRRecordedMove Move;
	Move.MoveNumber = Moves.Num() + 1;

	// BoardBefore.CurrentPlayer has not advanced yet, so it is the mover.
	Move.Player = BoardBefore.CurrentPlayer;

	// Flatten the pre-move board into bytes.
	Move.Board.Reserve(BoardBefore.Cells.Num());
	for (const ECFRCell Cell : BoardBefore.Cells)
	{
		Move.Board.Add(static_cast<uint8>(Cell));
	}

	Move.Action  = BoardBefore.EncodeAction(X, Y, Z);
	Move.ActionX = X;
	Move.ActionY = Y;
	Move.ActionZ = Z;

	// Use the supplied search policy when it matches the action space;
	// otherwise record a one-hot at the chosen action (human play).
	if (VisitCounts.Num() == ActionSpace)
	{
		Move.Policy = VisitCounts;
	}
	else
	{
		Move.Policy.Init(0, ActionSpace);
		if (Move.Action >= 0 && Move.Action < ActionSpace)
		{
			Move.Policy[Move.Action] = 1;
		}
	}

	Moves.Add(MoveTemp(Move));
}

void UCFRGameRecorder::EndGame(ECFRGameResult FinalResult)
{
	if (!bActive || Moves.Num() == 0)
	{
		bActive = false;
		return;
	}

	// Determine the winner once, then label every move from its own player's view.
	int32 Winner = 0;
	if      (FinalResult == ECFRGameResult::Player1Wins) { Winner = 1; }
	else if (FinalResult == ECFRGameResult::Player2Wins) { Winner = 2; }

	for (FCFRRecordedMove& Move : Moves)
	{
		Move.Value = (Winner == 0) ? 0.f : (Move.Player == Winner ? 1.f : -1.f);
	}

	const FString Json = BuildJson(FinalResult);

	const FString Dir = FPaths::ProjectSavedDir() / TEXT("TrainingData");
	IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);

	const FString FilePath = Dir / (GameId + TEXT(".json"));
	FFileHelper::SaveStringToFile(Json, *FilePath);

	bActive = false;
}

FString UCFRGameRecorder::BuildJson(ECFRGameResult FinalResult) const
{
	FString Out;
	Out += TEXT("{\n");
	Out += FString::Printf(TEXT("  \"game_id\": \"%s\",\n"), *GameId);
	Out += FString::Printf(TEXT("  \"mode\": \"%s\",\n"), *ModeToString(Mode));
	Out += FString::Printf(TEXT("  \"board_size\": {\"x\": %d, \"y\": %d, \"z\": %d},\n"), SizeX, SizeY, SizeZ);
	Out += FString::Printf(TEXT("  \"connect_n\": %d,\n"), 4);
	Out += FString::Printf(TEXT("  \"action_space\": %d,\n"), ActionSpace);
	Out += FString::Printf(TEXT("  \"final_result\": \"%s\",\n"), *ResultToString(FinalResult));
	Out += TEXT("  \"moves\": [\n");

	for (int32 i = 0; i < Moves.Num(); ++i)
	{
		const FCFRRecordedMove& Move = Moves[i];
		Out += TEXT("    {");
		Out += FString::Printf(TEXT("\"move_number\": %d, "), Move.MoveNumber);
		Out += FString::Printf(TEXT("\"player\": %d, "), Move.Player);
		Out += FString::Printf(TEXT("\"board\": [%s], "), *JoinBytes(Move.Board));
		Out += FString::Printf(TEXT("\"action\": %d, "), Move.Action);
		Out += FString::Printf(TEXT("\"action_xyz\": {\"x\": %d, \"y\": %d, \"z\": %d}, "),
			Move.ActionX, Move.ActionY, Move.ActionZ);
		Out += FString::Printf(TEXT("\"policy\": [%s], "), *JoinInts(Move.Policy));
		Out += FString::Printf(TEXT("\"value\": %.1f"), Move.Value);
		Out += TEXT("}");
		if (i < Moves.Num() - 1) { Out += TEXT(","); }
		Out += TEXT("\n");
	}

	Out += TEXT("  ]\n");
	Out += TEXT("}\n");
	return Out;
}

FString UCFRGameRecorder::ModeToString(ECFRGameMode InMode)
{
	switch (InMode)
	{
	case ECFRGameMode::Mode2D:        return TEXT("2D");
	case ECFRGameMode::Mode2DClassic: return TEXT("2D-Classic");
	case ECFRGameMode::Mode3D:        return TEXT("3D");
	default:                          return TEXT("Unknown");
	}
}

FString UCFRGameRecorder::ResultToString(ECFRGameResult Result)
{
	switch (Result)
	{
	case ECFRGameResult::Ongoing:     return TEXT("Ongoing");
	case ECFRGameResult::Player1Wins: return TEXT("Player1Wins");
	case ECFRGameResult::Player2Wins: return TEXT("Player2Wins");
	case ECFRGameResult::Draw:        return TEXT("Draw");
	default:                          return TEXT("Unknown");
	}
}

FString UCFRGameRecorder::JoinBytes(const TArray<uint8>& Values)
{
	FString Out;
	for (int32 i = 0; i < Values.Num(); ++i)
	{
		if (i > 0) { Out += TEXT(","); }
		Out.AppendInt(Values[i]);
	}
	return Out;
}

FString UCFRGameRecorder::JoinInts(const TArray<int32>& Values)
{
	FString Out;
	for (int32 i = 0; i < Values.Num(); ++i)
	{
		if (i > 0) { Out += TEXT(","); }
		Out.AppendInt(Values[i]);
	}
	return Out;
}
