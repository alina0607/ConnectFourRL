// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CFRHoverIndicator.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/**
 * @brief Local, per-client hover highlight. Not replicated — each player drives
 *        their own indicator from their own cursor trace.
 *
 * ACFRPlayerController spawns one of these for the local player and calls
 * Update() every frame with the cell/column to highlight and whether the move
 * would be legal. A single mesh is positioned and scaled to span the target;
 * its material switches between a legal (green) and illegal (red) look.
 */
UCLASS()
class CONNECTFOURRL_API ACFRHoverIndicator : public AActor
{
	GENERATED_BODY()

public:

	ACFRHoverIndicator();

	/**
	 * @brief Positions, sizes, and colours the highlight, or hides it.
	 *
	 * @param bShow      When false the indicator is hidden and other args ignored.
	 * @param bLegal     Selects the legal (green) or illegal (red) material.
	 * @param Center     World-space centre of the highlighted region.
	 * @param WorldSize  Desired full extents (X, Y, Z) of the highlight in world units.
	 */
	void Update(bool bShow, bool bLegal, const FVector& Center, const FVector& WorldSize);

	/** @brief Material shown when the hovered move is legal. Assign in BP (green, translucent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indicator")
	TObjectPtr<UMaterialInterface> MaterialLegal;

	/** @brief Material shown when the hovered move is illegal. Assign in BP (red, translucent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indicator")
	TObjectPtr<UMaterialInterface> MaterialIllegal;

private:

	/** @brief Root anchor for the highlight mesh. */
	UPROPERTY(VisibleAnywhere, Category = "Indicator")
	TObjectPtr<USceneComponent> SceneRoot;

	/** @brief The highlight mesh. Assign a centred unit cube / plane in the BP subclass. */
	UPROPERTY(VisibleAnywhere, Category = "Indicator")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** @brief Cached last legality so the material is only swapped on change. */
	bool bLastLegal = false;

	/** @brief False until the first material has been applied. */
	bool bMaterialInitialised = false;
};
