// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/Tasks/WBSTTask_Follow.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

// ============================================================
// Constructor
// ============================================================
FWBSTTask_Follow::FWBSTTask_Follow()
{
	// Follow DOES tick — we need per-frame speed adjustment.
	// The navigation itself auto-tracks via MoveToActor, but
	// speed tier changes require monitoring distance each frame.
	bShouldCallTick = true;

	// True: if the tree re-selects Follow while already following,
	// restart the move request. This handles edge cases where the
	// target changed or the path became invalid.
	bShouldStateChangeOnReselect = true;
}

// ============================================================
// Link
// ============================================================
bool FWBSTTask_Follow::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	Linker.LinkExternalData(PawnHandle);
	return true;
}

// ============================================================
// EnterState — start following the target
// ============================================================
EStateTreeRunStatus FWBSTTask_Follow::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Validate target — if the evaluator hasn't found a player yet,
	// we can't follow. Return Failed to trigger a transition back.
	if (!Data.TargetActor)
	{
		if (EnableDebugLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("WBSTTask_Follow::EnterState — No target actor! "
				     "Is the evaluator's PlayerPawn bound?"));
		}
		return EStateTreeRunStatus::Failed;
	}

	// Get the companion's pawn to save/modify movement speed.
	const APawn& Pawn = Context.GetExternalData(PawnHandle);
	if (ACharacter* Character = const_cast<ACharacter*>(
			Cast<ACharacter>(&Pawn)))
	{
		if (UCharacterMovementComponent* MoveComp =
				Character->GetCharacterMovement())
		{
			// Save the original speed so we can restore it on exit.
			// This is critical — without this, after leaving Follow,
			// the companion would be stuck at sprint speed during Idle.
			Data.OriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;

			// Set initial speed based on current distance.
			Data.CurrentSpeedTier = CalculateSpeedTier(
				Data.DistanceToTarget, Data.CurrentSpeedTier);
			MoveComp->MaxWalkSpeed = GetSpeedForTier(Data.CurrentSpeedTier);
		}
	}

	// Issue the move request through the AIController.
	// MoveToActor with a live actor pointer automatically updates
	// the destination as the target moves — we don't need to
	// re-request navigation every frame.
	AAIController& Controller = const_cast<AAIController&>(
		Context.GetExternalData(AIControllerHandle));

	const EPathFollowingRequestResult::Type Result =
		Controller.MoveToActor(
			Data.TargetActor,    // Goal actor
			AcceptableRadius,    // Stop this close
			true,                // bStopOnOverlap
			true,                // bUsePathfinding
			false,               // bCanStrafe
			nullptr,             // FilterClass
			true                 // bAllowPartialPath
		);

	if (EnableDebugLog)
	{
		const FString TierName =
			Data.CurrentSpeedTier == EWBFollowSpeedTier::Walk ? TEXT("Walk") :
			Data.CurrentSpeedTier == EWBFollowSpeedTier::Run ? TEXT("Run") :
			TEXT("Sprint");

		UE_LOG(LogTemp, Log,
			TEXT("WBSTTask_Follow::EnterState — %s following %s "
			     "(tier: %s, speed: %.0f, distance: %.0f, nav: %s)"),
			*Pawn.GetName(),
			*Data.TargetActor->GetName(),
			*TierName,
			GetSpeedForTier(Data.CurrentSpeedTier),
			Data.DistanceToTarget,
			Result == EPathFollowingRequestResult::RequestSuccessful
				? TEXT("OK")
				: Result == EPathFollowingRequestResult::AlreadyAtGoal
					? TEXT("Already there")
					: TEXT("Failed"));
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================
// Tick — adjust speed based on distance
// ============================================================
EStateTreeRunStatus FWBSTTask_Follow::Tick(
	FStateTreeExecutionContext& Context,
	float DeltaTime) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Target lost — player disconnected or died.
	if (!Data.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Calculate the appropriate speed tier for the current distance.
	const EWBFollowSpeedTier NewTier =
		CalculateSpeedTier(Data.DistanceToTarget, Data.CurrentSpeedTier);

	// Only modify MaxWalkSpeed when the tier actually changes.
	// This is the optimization — we avoid calling SetMaxWalkSpeed
	// 60 times per second when the companion is staying in the
	// same distance zone. With 200 companions, this matters.
	if (NewTier != Data.CurrentSpeedTier)
	{
		Data.CurrentSpeedTier = NewTier;

		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		if (ACharacter* Character = const_cast<ACharacter*>(
				Cast<ACharacter>(&Pawn)))
		{
			if (UCharacterMovementComponent* MoveComp =
					Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = GetSpeedForTier(NewTier);

				if (EnableDebugLog)
				{
					const FString TierName =
						NewTier == EWBFollowSpeedTier::Walk
							? TEXT("Walk") :
						NewTier == EWBFollowSpeedTier::Run
							? TEXT("Run") : TEXT("Sprint");

					UE_LOG(LogTemp, Log,
						TEXT("WBSTTask_Follow::Tick — %s speed tier "
						     "changed to %s (%.0f cm/s) at %.0f cm"),
						*Pawn.GetName(),
						*TierName,
						GetSpeedForTier(NewTier),
						Data.DistanceToTarget);
				}
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================
// ExitState — stop movement, restore original speed
// ============================================================
void FWBSTTask_Follow::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Stop the active pathfinding request.
	AAIController& Controller = const_cast<AAIController&>(
		Context.GetExternalData(AIControllerHandle));
	Controller.StopMovement();

	// Restore the original MaxWalkSpeed.
	// Without this, the companion would keep whatever speed
	// it had when it left Follow — potentially sprint speed
	// during Idle, which looks wrong even if it's not moving.
	const APawn& Pawn = Context.GetExternalData(PawnHandle);
	if (ACharacter* Character = const_cast<ACharacter*>(
			Cast<ACharacter>(&Pawn)))
	{
		if (UCharacterMovementComponent* MoveComp =
				Character->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = Data.OriginalMaxWalkSpeed;
		}
	}

	if (EnableDebugLog)
	{
		UE_LOG(LogTemp, Log,
			TEXT("WBSTTask_Follow::ExitState — %s stopped following. "
			     "Speed restored to %.0f cm/s."),
			*Pawn.GetName(),
			Data.OriginalMaxWalkSpeed);
	}

	// Reset tier for next entry.
	Data.CurrentSpeedTier = EWBFollowSpeedTier::None;
}

// ============================================================
// Helper: determine speed tier from distance
// ============================================================
EWBFollowSpeedTier FWBSTTask_Follow::CalculateSpeedTier(
	float Distance, EWBFollowSpeedTier CurrentTier) const
{
	// HYSTERESIS — use different thresholds for entering vs leaving
	// a speed tier. Without this, hovering at a boundary causes
	// frame-by-frame flickering between tiers.
	//
	// Example: SprintDistanceThreshold = 1200
	//   Enter sprint:  distance >= 1200 (threshold)
	//   Leave sprint:  distance <  1080 (threshold - buffer)
	//   Buffer zone:   1080-1200 = stays in current tier
	//
	// This is the same principle as the Idle/Follow transition
	// hysteresis (500 to enter, 300 to leave).

	const float Buffer = SpeedTierHysteresis;

	switch (CurrentTier)
	{
	case EWBFollowSpeedTier::Sprint:
		// Already sprinting — only drop to Run if well below threshold
		if (Distance < SprintDistanceThreshold - Buffer)
		{
			if (Distance < RunDistanceThreshold - Buffer)
				return EWBFollowSpeedTier::Walk;
			return EWBFollowSpeedTier::Run;
		}
		return EWBFollowSpeedTier::Sprint;

	case EWBFollowSpeedTier::Run:
		// Already running — check both directions
		if (Distance >= SprintDistanceThreshold)
			return EWBFollowSpeedTier::Sprint;
		if (Distance < RunDistanceThreshold - Buffer)
			return EWBFollowSpeedTier::Walk;
		return EWBFollowSpeedTier::Run;

	case EWBFollowSpeedTier::Walk:
	case EWBFollowSpeedTier::None:
	default:
		// Walking or just entered — only upgrade if above threshold
		if (Distance >= SprintDistanceThreshold)
			return EWBFollowSpeedTier::Sprint;
		if (Distance >= RunDistanceThreshold)
			return EWBFollowSpeedTier::Run;
		return EWBFollowSpeedTier::Walk;
	}
}

// ============================================================
// Helper: get speed value for a tier
// ============================================================
float FWBSTTask_Follow::GetSpeedForTier(
	EWBFollowSpeedTier Tier) const
{
	switch (Tier)
	{
	case EWBFollowSpeedTier::Sprint: return SprintSpeed;
	case EWBFollowSpeedTier::Run:    return RunSpeed;
	case EWBFollowSpeedTier::Walk:   return WalkSpeed;
	default:                         return WalkSpeed;
	}
}
