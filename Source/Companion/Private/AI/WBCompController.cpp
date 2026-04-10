// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/WBCompController.h"
#include "Components/StateTreeAIComponent.h"

AWBCompController::AWBCompController()
{
	// No tick needed — StateTreeAIComponent has its own tick.
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(
		TEXT("StateTreeAIComponent"));
}

void AWBCompController::BeginPlay()
{
	Super::BeginPlay();
}
