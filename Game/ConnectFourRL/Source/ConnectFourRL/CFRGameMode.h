// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CFRBoardState.h"
#include "CFRGameRules.h"
#include "CFRGameMode.generated.h"

class UCFRGameRulesBase;
class ACFRPiece;
class UCFRGameRecorder;
class ACFRGameState;

/**
 * @brief Game mode that owns the rule set and live board state for a Connect Four session.
 *
 * Responsibilities:
 *   - Creates and holds the UCFRGameRules2D instance at BeginPlay.
 *   - Processes player move requests via TryDrop().
 *   - Spawns game piece actors at the correct world position after each move.
 *   - Converts board coordinates to Unreal world positions.
 *   - Fires a Blueprint event when the game ends.
 */
UCLASS()
class CONNECTFOURRL_API ACFRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACFRGameMode();

	// ------------------------------------------------------------------
	// Game state
	// ------------------------------------------------------------------

	/** @brief The active rule set. Swapped between UCFRGameRules2D and UCFRGameRules3D on mode toggle. */
	UPROPERTY(BlueprintReadOnly, Category = "Rules")
	TObjectPtr<UCFRGameRulesBase> Rules;

	/** @brief The live board state updated after every accepted move. */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	FCFRBoardState CurrentBoard;

	/** @brief When true, records each completed game as JSON training data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording")
	bool bRecordGames = true;

	/** @brief Records moves and exports finished games. Created at BeginPlay when bRecordGames is set. */
	UPROPERTY()
	TObjectPtr<UCFRGameRecorder> Recorder;

	/** @brief True once the game reaches a terminal state; blocks further input until the board is reset. */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bGameOver = false;

	// ------------------------------------------------------------------
	// Visual configuration
	// ------------------------------------------------------------------

	/** @brief Distance between cell centres in the horizontal plane (World X / Y), in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Visual")
	float CellSize = 150.f;

	/**
	 * @brief Vertical distance between stacked piece centres in 3D mode, in world units.
	 *
	 * Decoupled from CellSize so the stacking step can be tuned to match the
	 * actual donut mesh height without affecting the horizontal grid spacing.
	 * Also controls the Z half-extent of the 3D debug-grid box so the overlay
	 * visually matches the piece proportions.
	 *
	 * Has no effect in 2D mode (all pieces land flat at World Z = 0).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Visual", meta = (ClampMin = "1.0"))
	float CellHeight = 50.f;

	/** @brief When true, draws the board grid with debug lines every frame during PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Debug")
	bool bDrawDebugGrid = false;

	/**
	 * @brief Visual gap between adjacent debug-grid cell boxes, in world units.
	 * Increase to create visible spacing; 0 = cells are edge-to-edge.
	 * Exposed so designers can tune appearance without recompiling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Debug", meta = (ClampMin = "0.0"))
	float CellGap = 10.f;

	// ------------------------------------------------------------------
	// Hover state — updated every frame by ACFRPlayerController
	// ------------------------------------------------------------------

	/** @brief Column index (X) currently under the mouse cursor (-1 = none). */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 HoveredColumn = -1;

	/**
	 * @brief Row index (Y) currently under the mouse cursor.
	 * Meaningful only in 2D free-placement mode where the player picks individual cells.
	 * Always 0 in 3D mode (gravity determines the row after drop).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 HoveredRow = 0;

	/** @brief Depth index (Z) currently under the mouse cursor (always 0 in 2D). */
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 HoveredDepth = 0;

	/**
	 * @brief Called every frame by ACFRPlayerController with the board cell under the cursor.
	 * Updates the hover highlight in the debug grid.
	 *
	 * @param Col    Board column index (X), or -1 if outside the board.
	 * @param Row    Board row index (Y); used for individual cell highlight in 2D mode.
	 * @param Depth  Board depth index (Z); always 0 in 2D mode.
	 */
	void SetHoveredCell(int32 Col, int32 Row, int32 Depth);

	/** @brief World-space position of board cell (0, 0, 0). Set this in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board|Visual")
	FVector BoardOrigin = FVector::ZeroVector;

	/** @brief Piece class to spawn for Player 1. Set to BP_CFRPiece_P1 in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piece")
	TSubclassOf<ACFRPiece> PieceActorClassP1;

	/** @brief Piece class to spawn for Player 2. Set to BP_CFRPiece_P2 in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piece")
	TSubclassOf<ACFRPiece> PieceActorClassP2;

	/** @brief Height above the target cell where the piece spawns before falling (World Z offset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piece")
	float DropStartHeight = 800.f;

	/** @brief Fall speed in Unreal units per second. Higher = faster drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piece")
	float PieceFallSpeed = 600.f;

	// ------------------------------------------------------------------
	// Game logic — called by ACFRColumnActor on player click
	// ------------------------------------------------------------------

	/**
	 * @brief Attempts to drop the current player's piece in column (X, Z).
	 *
	 * Uses the gravity mechanic: the piece falls to the lowest empty row in the
	 * chosen column.  Intended for 3D mode (and any gravity-based variant).
	 *
	 * @param X  Column index (0 to SizeX-1).
	 * @param Z  Depth index; always 0 in 2D mode.
	 * @return   True if the move was accepted and applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool TryDrop(int32 X, int32 Z = 0);

	/**
	 * @brief Places the current player's piece directly at cell (X, Y, Z).
	 *
	 * No gravity — the piece lands at the exact row specified.
	 * Used in 2D free-placement mode where the player picks any vacant cell.
	 *
	 * @param X  Column index (0 to SizeX-1).
	 * @param Y  Row index    (0 to SizeY-1).
	 * @param Z  Depth index; always 0 in 2D mode.
	 * @return   True if the move was accepted and applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool TryPlace(int32 X, int32 Y, int32 Z = 0);

	/**
	 * @brief Toggles between 2D and 3D mode.
	 * Destroys all spawned pieces, resets the board, and reinitialises the rule set.
	 * Intended for debug / testing only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game|Debug")
	void SwitchGameMode();

	/**
	 * @brief Returns the Unreal world position of board cell (X, Y, Z).
	 *
	 * The mapping is mode-dependent:
	 *
	 *   2D (SizeZ == 1):
	 *     Board X (col, 0-6) → World X  (left / right)
	 *     Board Y (row, 0-5) → World Y  (front / back; gravity fills Y=0 first)
	 *     Board Z = 0        → World Z  = 0  (all cells lie flat on the ground)
	 *
	 *   3D (SizeZ == 4):
	 *     Board X (col,   0-3) → World X  (left / right)
	 *     Board Z (depth, 0-3) → World Y  (front / back)
	 *     Board Y (row,   0-3) → World Z  (up; pieces stack upward)
	 *
	 * @param X  Board column index.
	 * @param Y  Board row index (gravity direction).
	 * @param Z  Board depth index (always 0 in 2D mode).
	 */
	UFUNCTION(BlueprintCallable, Category = "Board|Visual")
	FVector GetCellWorldPosition(int32 X, int32 Y, int32 Z) const;

	// ------------------------------------------------------------------
	// Blueprint event — implement result UI in BP_CFRGameMode
	// ------------------------------------------------------------------

	/**
	 * @brief Fired when a new game session begins or the mode is switched.
	 * Blueprint should reset the UI and prepare for the new session.
	 *
	 * @param Mode  The active game mode (Mode2D or Mode3D).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void OnGameStarted(ECFRGameMode Mode);

	/**
	 * @brief Fired when the game reaches a terminal state.
	 * Blueprint should display the result (win / draw) UI.
	 *
	 * @param Result  The final game result.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void OnGameEnded(ECFRGameResult Result);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** @brief Assigns a player number (1, 2, ...) to each joining controller. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:

	/** @brief Cached replicated game state. Clients read it; the server mirrors into it. */
	UPROPERTY()
	TObjectPtr<ACFRGameState> CFRGameState;

	/** @brief Number of players assigned so far (used to hand out player numbers). */
	int32 AssignedPlayers = 0;

	/** @brief Pushes the authoritative board and game-over flag into the replicated GameState. */
	void SyncStateToGameState();

	/** @brief Draws a box at every cell centre to visualise grid layout during PIE. */
	void DrawDebugGrid() const;

	/** @brief Shows the game result as an on-screen debug message. Temporary aid until BP_CFRGameMode implements result UI. */
	void ShowDebugResult(ECFRGameResult Result) const;
};
