// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeLinker.h"
#include "StateTreeTaskBase.h"
#include "WBSTTask_Follow.generated.h"

class APawn;

/**
 * Instance data for the Follow task.
 */
USTRUCT(BlueprintType)
struct FWBSTTaskFollowInstanceData
{
	GENERATED_BODY()

	/** The actor to follow. Bind to: Evaluator.PlayerPawn */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;
};

/**
 * Follow Task — pure navigation with move-request monitoring.
 *
 * This task does NOT manage speed (that's the SpeedManager's job).
 * It handles only navigation:
 *   EnterState: calls MoveToActor
 *   Tick: checks if the move request completed, re-issues if needed
 *   ExitState: calls StopMovement
 *
 * Why does this task tick?
 * MoveToActor auto-tracks a moving target, BUT when the companion
 * reaches the AcceptableRadius, the PathFollowingComponent marks
 * the request as complete and stops. If the player walks away again,
 * no new request is issued without a tick to detect the completion
 * and re-request. This tick is lightweight — one function call to
 * check path status, no distance math, no speed logic.
 *
 * Single Responsibility: this task navigates. Period.
 * Speed is handled by SpeedManager Global Task.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Follow Task", Category = "WB|AI|Tasks"))
struct COMPANION_API FWBSTTask_Follow : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWBSTTaskFollowInstanceData;

	FWBSTTask_Follow();

	virtual const UScriptStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		float DeltaTime
	) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

protected:
	/** How close the companion gets before stopping (cm). */
	UPROPERTY(EditAnywhere, Category = "Navigation")
	float AcceptableRadius = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool EnableDebugLog = true;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
	TStateTreeExternalDataHandle<APawn> PawnHandle;

private:
	/** Issue a MoveToActor request. Used by both EnterState and Tick. */
	void RequestMoveTo(AAIController& Controller, AActor* Target,
	                   const APawn& Pawn) const;
};
