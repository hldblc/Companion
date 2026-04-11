// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/Tasks/WBSTGlobalTask_SpeedManager.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// ============================================================
// Constructor
// ============================================================
FWBSTGlobalTask_SpeedManager::FWBSTGlobalTask_SpeedManager()
{
	// Global task ticks to monitor distance and adjust speed.
	bShouldCallTick = true;

	// If tree re-selects root, don't restart — keep tracking.
	bShouldStateChangeOnReselect = false;
}

// ============================================================
// Link
// ============================================================
bool FWBSTGlobalTask_SpeedManager::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(PawnHandle);
	return true;
}

// ============================================================
// EnterState — capture original speed
// ============================================================
EStateTreeRunStatus FWBSTGlobalTask_SpeedManager::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Capture the original MaxWalkSpeed on first entry.
	// This is the "default" speed set on the character Blueprint.
	if (!Data.bHasCapturedOriginalSpeed)
	{
		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		if (const ACharacter* Character = Cast<ACharacter>(&Pawn))
		{
			if (const UCharacterMovementComponent* MoveComp =
					Character->GetCharacterMovement())
			{
				Data.OriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
				Data.bHasCapturedOriginalSpeed = true;

				if (EnableDebugLog)
				{
					UE_LOG(LogTemp, Log,
						TEXT("SpeedManager::EnterState — %s original "
						     "speed captured: %.0f cm/s"),
						*Pawn.GetName(),
						Data.OriginalMaxWalkSpeed);
				}
			}
		}
	}

	Data.CurrentSpeedTier = EWBFollowSpeedTier::None;

	// CRITICAL: Global Tasks MUST return Running.
	// Returning Succeeded/Failed stops the entire State Tree.
	return EStateTreeRunStatus::Running;
}

// ============================================================
// Tick — adjust speed based on distance
// ============================================================
EStateTreeRunStatus FWBSTGlobalTask_SpeedManager::Tick(
	FStateTreeExecutionContext& Context,
	float DeltaTime) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// If companion is very close to player (below MinActiveDistance),
	// force Walk tier. This prevents sprint speed during Idle.
	EWBFollowSpeedTier NewTier;
	if (Data.DistanceToTarget < MinActiveDistance)
	{
		NewTier = EWBFollowSpeedTier::Walk;
	}
	else
	{
		NewTier = CalculateSpeedTier(
			Data.DistanceToTarget, Data.CurrentSpeedTier);
	}

	// Only update the movement component when the tier changes.
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
						TEXT("SpeedManager::Tick — %s tier: %s "
						     "(%.0f cm/s) at %.0f cm"),
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
// ExitState — restore original speed
// ============================================================
void FWBSTGlobalTask_SpeedManager::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	// Restore original speed when the tree stops.
	if (Data.bHasCapturedOriginalSpeed)
	{
		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		if (ACharacter* Character = const_cast<ACharacter*>(
				Cast<ACharacter>(&Pawn)))
		{
			if (UCharacterMovementComponent* MoveComp =
					Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = Data.OriginalMaxWalkSpeed;

				if (EnableDebugLog)
				{
					UE_LOG(LogTemp, Log,
						TEXT("SpeedManager::ExitState — %s speed "
						     "restored to %.0f cm/s"),
						*Pawn.GetName(),
						Data.OriginalMaxWalkSpeed);
				}
			}
		}
	}
}

// ============================================================
// CalculateSpeedTier — with hysteresis
// ============================================================
EWBFollowSpeedTier FWBSTGlobalTask_SpeedManager::CalculateSpeedTier(
	float Distance, EWBFollowSpeedTier CurrentTier) const
{
	const float Buffer = SpeedTierHysteresis;

	switch (CurrentTier)
	{
	case EWBFollowSpeedTier::Sprint:
		if (Distance < SprintDistanceThreshold - Buffer)
		{
			if (Distance < RunDistanceThreshold - Buffer)
				return EWBFollowSpeedTier::Walk;
			return EWBFollowSpeedTier::Run;
		}
		return EWBFollowSpeedTier::Sprint;

	case EWBFollowSpeedTier::Run:
		if (Distance >= SprintDistanceThreshold)
			return EWBFollowSpeedTier::Sprint;
		if (Distance < RunDistanceThreshold - Buffer)
			return EWBFollowSpeedTier::Walk;
		return EWBFollowSpeedTier::Run;

	case EWBFollowSpeedTier::Walk:
	case EWBFollowSpeedTier::None:
	default:
		if (Distance >= SprintDistanceThreshold)
			return EWBFollowSpeedTier::Sprint;
		if (Distance >= RunDistanceThreshold)
			return EWBFollowSpeedTier::Run;
		return EWBFollowSpeedTier::Walk;
	}
}

// ============================================================
// GetSpeedForTier
// ============================================================
float FWBSTGlobalTask_SpeedManager::GetSpeedForTier(
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
