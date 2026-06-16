// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CFRPlayerController.generated.h"

class ACFRHoverIndicator;

/**
 * @brief Player controller for Connect Four.
 *
 * Every frame, projects the mouse position onto the board's horizontal plane
 * via a ray-plane intersection and reports the hovered column to ACFRGameMode.
 *
 * On left-click, calls ACFRGameMode::TryDrop() with the hovered column.
 * This approach works correctly at any camera angle — no hit-box actors required.
 *
 * In the multiplayer phase, TryDrop() will be replaced with a Server RPC.
 */
UCLASS()
class CONNECTFOURRL_API ACFRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ACFRPlayerController();

	virtual void Tick(float DeltaTime) override;

	/** @brief Player number assigned by the server on login (1 or 2; 0 = unassigned). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 PlayerNumber = 0;

	/** @brief Hover indicator class to spawn for the local player. Set to BP_CFRHoverIndicator. */
	UPROPERTY(EditDefaultsOnly, Category = "Game")
	TSubclassOf<ACFRHoverIndicator> HoverIndicatorClass;

protected:

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	/** @brief Server RPC: requests a gravity column-drop at (X, Z). */
	UFUNCTION(Server, Reliable)
	void Server_TryDrop(int32 X, int32 Z);

	/** @brief Server RPC: requests a free placement at cell (X, Y, Z). */
	UFUNCTION(Server, Reliable)
	void Server_TryPlace(int32 X, int32 Y, int32 Z);

	/** @brief Server RPC: requests a 2D/3D mode switch (resets the game). */
	UFUNCTION(Server, Reliable)
	void Server_SwitchGameMode();

	/** @brief Updates the local hover indicator from the current hovered cell/column. */
	void UpdateHoverIndicator();

	/** @brief Local hover highlight (spawned for the local player only). */
	UPROPERTY()
	TObjectPtr<ACFRHoverIndicator> HoverIndicator;

	/**
	 * @brief Projects the mouse cursor onto the board plane and updates
	 *        ACFRGameMode with the hovered column index.
	 *
	 * Computes the ray-plane intersection:
	 *   T = (BoardZ - RayOrigin.Z) / RayDirection.Z
	 *   HitPoint = RayOrigin + RayDirection * T
	 * Then converts HitPoint to board column / depth indices.
	 */
	void TraceToBoard();

	/** @brief Submits a drop at the currently hovered column on left-click. */
	void OnClickBoard();

	/** @brief Column index (X) resolved on the last frame (-1 = cursor outside board). */
	int32 HoveredColumn = -1;

	/** @brief Row index (Y) resolved on the last frame; meaningful only in 2D mode. */
	int32 HoveredRow = 0;

	/** @brief Depth index (Z) resolved on the last frame (always 0 in 2D mode). */
	int32 HoveredDepth = 0;
};
