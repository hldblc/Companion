// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/Evaluators/WBSTEval_PlayerContext.h"
#include "AI/WBCompChar.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

// ============================================================
// Link — resolve external data at initialization
// ============================================================
bool FWBSTEval_PlayerContext::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(CompanionPawnHandle);
	return true;
}

// ============================================================
// TreeStart — cache the owner player reference once
// ============================================================
void FWBSTEval_PlayerContext::TreeStart(
	FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Get our companion pawn through the external data handle.
	const APawn& CompanionPawn = Context.GetExternalData(CompanionPawnHandle);

	// Cast to AWBCompChar to access the OwnerPlayerPawn property.
	// This cast happens ONCE at tree start, not every frame.
	if (const AWBCompChar* Companion = Cast<AWBCompChar>(&CompanionPawn))
	{
		Data.PlayerPawn = Companion->GetOwnerPlayerPawn();

		if (Data.PlayerPawn)
		{
			// Compute initial distance
			Data.DistanceToPlayer = FVector::Dist(
				CompanionPawn.GetActorLocation(),
				Data.PlayerPawn->GetActorLocation()
			);

			UE_LOG(LogTemp, Log,
				TEXT("WBSTEval_PlayerContext::TreeStart — %s tracking %s "
				     "(initial distance: %.0f cm)"),
				*CompanionPawn.GetName(),
				*Data.PlayerPawn->GetName(),
				Data.DistanceToPlayer);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("WBSTEval_PlayerContext::TreeStart — %s has no "
				     "OwnerPlayerPawn assigned! Call SetOwnerPlayerPawn() "
				     "before the State Tree starts."),
				*CompanionPawn.GetName());
		}
	}
}

// ============================================================
// Tick — update distance to the owner player
// ============================================================
void FWBSTEval_PlayerContext::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// If the owner player disconnected or died, try to re-acquire.
	// This handles the case where the player pawn is destroyed
	// and a new one is spawned (e.g., respawn after death).
	if (!Data.PlayerPawn)
	{
		const APawn& CompanionPawn =
			Context.GetExternalData(CompanionPawnHandle);

		if (const AWBCompChar* Companion = Cast<AWBCompChar>(&CompanionPawn))
		{
			Data.PlayerPawn = Companion->GetOwnerPlayerPawn();
		}

		// Still null? Nothing to track.
		if (!Data.PlayerPawn)
		{
			Data.DistanceToPlayer = 0.0f;
			return;
		}
	}

	// Compute distance — a single FVector::Dist call.
	// This is the only work done per frame. No actor lookups,
	// no casts, no string comparisons.
	const APawn& CompanionPawn =
		Context.GetExternalData(CompanionPawnHandle);

	Data.DistanceToPlayer = FVector::Dist(
		CompanionPawn.GetActorLocation(),
		Data.PlayerPawn->GetActorLocation()
	);
}
