// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRPiece.h"
#include "Components/StaticMeshComponent.h"

ACFRPiece::ACFRPiece()
{
	// Tick is disabled at start — enabled only when StartFall() is called.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
}

void ACFRPiece::StartFall(const FVector& InTargetLocation, float InFallSpeed)
{
	TargetLocation = InTargetLocation;
	FallSpeed      = InFallSpeed;
	bFalling       = true;

	SetActorTickEnabled(true);
}

void ACFRPiece::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFalling) { return; }

	// Move toward the target at a constant speed.
	const FVector Current = GetActorLocation();
	const FVector NewLocation = FMath::VInterpConstantTo(Current, TargetLocation, DeltaTime, FallSpeed);
	SetActorLocation(NewLocation);

	// Stop when close enough to the target.
	if (NewLocation.Equals(TargetLocation, 0.5f))
	{
		SetActorLocation(TargetLocation);
		bFalling = false;
		SetActorTickEnabled(false);
	}
}
