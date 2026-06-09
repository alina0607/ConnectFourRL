// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CFRColumnActor.generated.h"

class UBoxComponent;

/**
 * @brief Invisible hit-box actor placed above each board column.
 *
 * When the player clicks this actor, it retrieves ACFRGameMode from the world
 * and calls TryDrop(ColumnIndex, DepthIndex) to submit the move.
 *
 * Place one instance per column in the level and set ColumnIndex accordingly.
 * In 2D mode, leave DepthIndex at 0.
 */
UCLASS()
class CONNECTFOURRL_API ACFRColumnActor : public AActor
{
	GENERATED_BODY()

public:

	ACFRColumnActor();

	/** @brief Board column (X axis) this actor represents. Set per instance in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board")
	int32 ColumnIndex = 0;

	/** @brief Board depth (Z axis). Always 0 in 2D mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board")
	int32 DepthIndex = 0;

protected:

	virtual void BeginPlay() override;

private:

	/** @brief Invisible collision volume that captures player mouse clicks. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> HitBox;

	/** @brief Bound to HitBox.OnClicked — forwards the click to ACFRGameMode. */
	UFUNCTION()
	void OnHitBoxClicked(UPrimitiveComponent* ClickedComp, FKey ButtonPressed);
};
