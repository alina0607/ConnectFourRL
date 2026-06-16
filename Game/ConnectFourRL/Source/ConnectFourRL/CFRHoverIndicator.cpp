#include "CFRHoverIndicator.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

ACFRHoverIndicator::ACFRHoverIndicator()
{
	// Driven externally by the player controller — no self-tick.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);

	// Hidden until the first Update() call.
	SetActorHiddenInGame(true);
}

void ACFRHoverIndicator::Update(bool bShow, bool bLegal, const FVector& Center, const FVector& WorldSize)
{
	if (!bShow)
	{
		SetActorHiddenInGame(true);
		return;
	}

	SetActorHiddenInGame(false);
	SetActorLocation(Center);

	// Scale the mesh so its full extents match WorldSize, derived from the asset's
	// own unscaled bounds so any centred mesh the designer assigns works correctly.
	if (const UStaticMesh* Mesh = MeshComponent->GetStaticMesh())
	{
		const FVector HalfExtent = Mesh->GetBounds().BoxExtent;
		const FVector Scale(
			HalfExtent.X > KINDA_SMALL_NUMBER ? (WorldSize.X * 0.5f) / HalfExtent.X : 1.f,
			HalfExtent.Y > KINDA_SMALL_NUMBER ? (WorldSize.Y * 0.5f) / HalfExtent.Y : 1.f,
			HalfExtent.Z > KINDA_SMALL_NUMBER ? (WorldSize.Z * 0.5f) / HalfExtent.Z : 1.f);
		MeshComponent->SetWorldScale3D(Scale);
	}

	// Swap material only when legality changes (avoids per-frame churn).
	if (!bMaterialInitialised || bLegal != bLastLegal)
	{
		if (UMaterialInterface* Mat = bLegal ? MaterialLegal : MaterialIllegal)
		{
			MeshComponent->SetMaterial(0, Mat);
		}
		bLastLegal           = bLegal;
		bMaterialInitialised = true;
	}
}
