// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/WBCompController.h"
#include "Components/StateTreeAIComponent.h"

AWBCompController::AWBCompController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(
		TEXT("StateTreeAIComponent"));

	// Do NOT start the State Tree automatically.
	// We start it manually after the companion's OwnerPlayerPawn
	// is assigned, so the evaluator sees the player in TreeStart.
	// Without this, TreeStart fires before BeginPlay sets the owner,
	// causing a one-frame null-player race condition.
	StateTreeAIComponent->SetStartLogicAutomatically(false);
}

void AWBCompController::BeginPlay()
{
	Super::BeginPlay();
}
