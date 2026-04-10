// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/Tasks/WBSTTask_Idle.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// ============================================================
// Constructor
// ============================================================
FWBSTTask_Idle::FWBSTTask_Idle()
{
	// TICKLESS — the core optimization for large-scale AI.
	// With 200 companions idling, this saves 200 * Tick() calls
	// per frame. Duration-based transitions are handled by the
	// State Tree editor's built-in conditions, not by C++.
	bShouldCallTick = false;

	// Don't restart Idle if the tree re-selects it.
	// The companion is already standing still — no need to
	// re-execute StopMovement() and re-log.
	bShouldStateChangeOnReselect = false;
}

// ============================================================
// Link
// ============================================================
bool FWBSTTask_Idle::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(AIControllerHandle);
	Linker.LinkExternalData(PawnHandle);
	return true;
}

// ============================================================
// EnterState — stop movement, then return Running
// ============================================================
EStateTreeRunStatus FWBSTTask_Idle::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	const APawn& Pawn = Context.GetExternalData(PawnHandle);

	// --- Conditional movement halt ---
	// Only call StopMovement if the companion is actually moving.
	// At scale, this prevents hundreds of redundant navigation
	// resets and avoids unnecessary network replication updates
	// for characters that are already stationary.
	if (const ACharacter* Character = Cast<ACharacter>(&Pawn))
	{
		if (const UCharacterMovementComponent* MoveComp =
			Character->GetCharacterMovement())
		{
			const float CurrentSpeed = MoveComp->Velocity.Size();

			if (CurrentSpeed > MinVelocityThreshold)
			{
				// Character is moving — stop it.
				// const_cast required because GetCharacterMovement()
				// returns non-const but our Pawn ref is const.
				const_cast<UCharacterMovementComponent*>(MoveComp)
					->StopMovementImmediately();

				// Also cancel any active pathfinding request.
				AAIController& Controller =
					const_cast<AAIController&>(
						Context.GetExternalData(AIControllerHandle));
				Controller.StopMovement();

				if (EnableDebugLog)
				{
					UE_LOG(LogTemp, Log,
						TEXT("WBSTTask_Idle::EnterState — %s stopped "
						     "(was moving at %.0f cm/s)."),
						*Pawn.GetName(), CurrentSpeed);
				}
			}
			else if (EnableDebugLog)
			{
				UE_LOG(LogTemp, Log,
					TEXT("WBSTTask_Idle::EnterState — %s entered Idle "
					     "(already stationary)."),
					*Pawn.GetName());
			}
		}
	}

	// Return Running — the companion stays in Idle until a
	// Transition fires. Since bShouldCallTick is false,
	// there will be zero per-frame cost.
	return EStateTreeRunStatus::Running;
}

// ============================================================
// ExitState — cleanup (future: stop idle animations)
// ============================================================
void FWBSTTask_Idle::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	if (EnableDebugLog)
	{
		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		UE_LOG(LogTemp, Log,
			TEXT("WBSTTask_Idle::ExitState — %s leaving Idle state."),
			*Pawn.GetName());
	}

	// Future: stop idle montage, release Smart Object claim, etc.
}
