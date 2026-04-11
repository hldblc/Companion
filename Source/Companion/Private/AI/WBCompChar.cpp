// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/WBCompChar.h"
#include "AI/WBCompController.h"
#include "Net/UnrealNetwork.h"  // Required for DOREPLIFETIME
#include "Components/StateTreeAIComponent.h"
#include "Kismet/GameplayStatics.h"

AWBCompChar::AWBCompChar()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AWBCompController::StaticClass();

	// Enable replication on this actor.
	// Without this, UPROPERTY(Replicated) properties won't sync.
	bReplicates = true;
}

void AWBCompChar::BeginPlay()
{
	Super::BeginPlay();

	// Server-only: assign the owner player and start the State Tree.
	// This runs AFTER possession, so GetController() is valid.
	if (HasAuthority())
	{
		// --- Set owner player ---
		// In PIE / standalone: use player index 0 (local player).
		// In production multiplayer: the GameMode would call
		// SetOwnerPlayerPawn() with the correct player before this.
		if (!OwnerPlayerPawn)
		{
			APawn* LocalPlayer = UGameplayStatics::GetPlayerPawn(this, 0);
			if (LocalPlayer)
			{
				SetOwnerPlayerPawn(LocalPlayer);
			}
		}

		// --- Start the State Tree ---
		// Now that the owner is set, the evaluator's TreeStart()
		// will see a valid OwnerPlayerPawn. No more race condition.
		if (AWBCompController* CompController =
			Cast<AWBCompController>(GetController()))
		{
			if (UStateTreeAIComponent* STComp =
				CompController->GetStateTreeAIComponent())
			{
				STComp->StartLogic();
			}
		}
	}
}

void AWBCompChar::SetOwnerPlayerPawn(APawn* InPlayerPawn)
{
	// Server-authoritative: only the server should set this.
	// HasAuthority() returns true on the server (or in standalone).
	if (HasAuthority())
	{
		OwnerPlayerPawn = InPlayerPawn;

		UE_LOG(LogTemp, Log,
			TEXT("WBCompChar::SetOwnerPlayerPawn — %s now belongs to %s"),
			*GetName(),
			InPlayerPawn ? *InPlayerPawn->GetName() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("WBCompChar::SetOwnerPlayerPawn — Called on client! "
			     "Only the server should assign companion ownership."));
	}
}

void AWBCompChar::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME tells the replication system to sync this
	// property to ALL connected clients whenever it changes.
	// For a more optimized approach at scale, you could use
	// DOREPLIFETIME_CONDITION with COND_OwnerOnly — but for
	// companions that all players can see, we replicate to everyone.
	DOREPLIFETIME(AWBCompChar, OwnerPlayerPawn);
}
