// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ACFRPiece::ACFRPiece()
{
	// Tick is disabled at start — enabled only when StartFall() is called.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Server spawns the piece; it replicates to all clients. Movement is NOT
	// replicated per-frame — each machine runs the same fall interpolation
	// locally from the replicated target, so only the spawn transform and the
	// target/speed need to travel the wire.
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
}

void ACFRPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACFRPiece, RepStartLocation);
	DOREPLIFETIME(ACFRPiece, RepTargetLocation);
	DOREPLIFETIME(ACFRPiece, RepFallSpeed);
}

void ACFRPiece::StartFall(const FVector& InTargetLocation, float InFallSpeed)
{
	TargetLocation = InTargetLocation;
	FallSpeed      = InFallSpeed;
	bFalling       = true;

	SetActorTickEnabled(true);

	// On the authority, publish the start (current spawn position), target, and
	// speed so each client can replay the exact same fall locally.
	if (HasAuthority())
	{
		RepStartLocation  = GetActorLocation();
		RepTargetLocation = InTargetLocation;
		RepFallSpeed      = InFallSpeed;
	}
}

void ACFRPiece::OnRep_FallTarget()
{
	// Client side: snap to the true spawn height first (the replicated transform
	// may have arrived mid-fall), then run the local fall toward the target.
	SetActorLocation(RepStartLocation);
	StartFall(RepTargetLocation, RepFallSpeed);
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
