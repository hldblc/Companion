// Copyright 2026 Halit Bilici. All Rights Reserved.

#include "AI/Tasks/WBSTTask_Follow.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

// ============================================================
// Constructor
// ============================================================
FWBSTTask_Follow::FWBSTTask_Follow()
{
	// This task DOES tick — but only to monitor the move request.
	// When MoveToActor reaches the AcceptableRadius, the
	// PathFollowingComponent marks the request as "completed"
	// and stops. If the player walks away again, we need to
	// detect that and re-issue MoveToActor.
	//
	// The tick is lightweight: one GetMoveStatus() call per frame.
	// No distance math, no speed logic (that's SpeedManager's job).
	bShouldCallTick = true;

	// If tree re-selects Follow, restart the move request.
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
// EnterState — start following
// ============================================================
EStateTreeRunStatus FWBSTTask_Follow::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	if (!Data.TargetActor)
	{
		if (EnableDebugLog)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("WBSTTask_Follow::EnterState — No target actor!"));
		}
		return EStateTreeRunStatus::Failed;
	}

	AAIController& Controller = const_cast<AAIController&>(
		Context.GetExternalData(AIControllerHandle));
	const APawn& Pawn = Context.GetExternalData(PawnHandle);

	RequestMoveTo(Controller, Data.TargetActor, Pawn);

	return EStateTreeRunStatus::Running;
}

// ============================================================
// Tick — monitor move request, re-issue if completed
// ============================================================
EStateTreeRunStatus FWBSTTask_Follow::Tick(
	FStateTreeExecutionContext& Context,
	float DeltaTime) const
{
	FInstanceDataType& Data =
		Context.GetInstanceData<FInstanceDataType>(*this);

	if (!Data.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	AAIController& Controller = const_cast<AAIController&>(
		Context.GetExternalData(AIControllerHandle));

	// Check if the current move request is still active.
	// GetMoveStatus() is a single enum check — extremely cheap.
	const EPathFollowingStatus::Type MoveStatus =
		Controller.GetMoveStatus();

	if (MoveStatus != EPathFollowingStatus::Moving)
	{
		// The move request completed (reached goal) or failed.
		// Re-issue to keep following the moving target.
		// This handles the case where the companion reached the
		// player, the PathFollowing stopped, and the player
		// walked away again.
		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		RequestMoveTo(Controller, Data.TargetActor, Pawn);
	}

	return EStateTreeRunStatus::Running;
}

// ============================================================
// ExitState — cancel the move
// ============================================================
void FWBSTTask_Follow::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	AAIController& Controller = const_cast<AAIController&>(
		Context.GetExternalData(AIControllerHandle));
	Controller.StopMovement();

	if (EnableDebugLog)
	{
		const APawn& Pawn = Context.GetExternalData(PawnHandle);
		UE_LOG(LogTemp, Log,
			TEXT("WBSTTask_Follow::ExitState — %s stopped following."),
			*Pawn.GetName());
	}
}

// ============================================================
// RequestMoveTo — shared helper for EnterState and Tick
// ============================================================
void FWBSTTask_Follow::RequestMoveTo(
	AAIController& Controller, AActor* Target,
	const APawn& Pawn) const
{
	const EPathFollowingRequestResult::Type Result =
		Controller.MoveToActor(
			Target,              // Goal actor (auto-tracked)
			AcceptableRadius,    // Stop this close
			true,                // bStopOnOverlap
			true,                // bUsePathfinding
			false,               // bCanStrafe
			nullptr,             // FilterClass
			true                 // bAllowPartialPath
		);

	if (EnableDebugLog)
	{
		UE_LOG(LogTemp, Log,
			TEXT("WBSTTask_Follow — %s → %s (radius: %.0f, nav: %s)"),
			*Pawn.GetName(),
			*Target->GetName(),
			AcceptableRadius,
			Result == EPathFollowingRequestResult::RequestSuccessful
				? TEXT("OK")
				: Result == EPathFollowingRequestResult::AlreadyAtGoal
					? TEXT("At goal")
					: TEXT("Failed"));
	}
}
