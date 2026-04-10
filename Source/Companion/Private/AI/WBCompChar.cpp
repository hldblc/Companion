// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/WBCompChar.h"
#include "AI/WBCompController.h"
#include "Net/UnrealNetwork.h"  // Required for DOREPLIFETIME

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
