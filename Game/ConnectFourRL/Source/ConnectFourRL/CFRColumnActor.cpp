// Fill out your copyright notice in the Description page of Project Settings.

#include "CFRColumnActor.h"
#include "CFRGameMode.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ACFRColumnActor::ACFRColumnActor()
{
	PrimaryActorTick.bCanEverTick = false;

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	SetRootComponent(HitBox);

	// Tall box covering the full column height so the player can click anywhere above it.
	HitBox->SetBoxExtent(FVector(40.f, 40.f, 300.f));
	HitBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ACFRColumnActor::BeginPlay()
{
	Super::BeginPlay();
	HitBox->OnClicked.AddDynamic(this, &ACFRColumnActor::OnHitBoxClicked);
}

void ACFRColumnActor::OnHitBoxClicked(UPrimitiveComponent* ClickedComp, FKey ButtonPressed)
{
	ACFRGameMode* GameMode = Cast<ACFRGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		GameMode->TryDrop(ColumnIndex, DepthIndex);
	}
}
