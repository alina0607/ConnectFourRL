// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CFRPiece.generated.h"

/**
 * @brief A single game piece that falls from above and lands on its target cell.
 *
 * After spawning, call StartFall() to begin the drop animation.
 * The piece moves toward TargetLocation at FallSpeed units per second.
 * Tick is enabled only while falling and disabled automatically on arrival.
 */
UCLASS()
class CONNECTFOURRL_API ACFRPiece : public AActor
{
	GENERATED_BODY()

public:

	ACFRPiece();

	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief Begins the fall animation toward the given world position.
	 *
	 * @param InTargetLocation  The board cell centre the piece should land on.
	 * @param InFallSpeed       Movement speed in Unreal units per second.
	 */
	void StartFall(const FVector& InTargetLocation, float InFallSpeed);

private:

	/** @brief Invisible root — anchors the Actor at the exact spawn position. */
	UPROPERTY(VisibleAnywhere, Category = "Piece")
	TObjectPtr<USceneComponent> SceneRoot;

	/** @brief Visual mesh. Assign the donut asset and fix the relative offset in BP subclass. */
	UPROPERTY(VisibleAnywhere, Category = "Piece")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** @brief World position the piece is falling toward. */
	FVector TargetLocation = FVector::ZeroVector;

	/** @brief Movement speed in units per second while falling. */
	float FallSpeed = 500.f;

	/** @brief True while the piece is still in motion. */
	bool bFalling = false;
};
