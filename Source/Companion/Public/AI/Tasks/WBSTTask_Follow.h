// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeLinker.h"
#include "StateTreeTaskBase.h"
#include "AI/Data/Enums/WBCompanionEnums.h"
#include "WBSTTask_Follow.generated.h"

class APawn;

/**
 * Instance data for the Follow task.
 *
 * INPUT properties are bound in the State Tree editor to the
 * evaluator's outputs. The task never directly references the
 * evaluator — it just declares "I need these values" and the
 * binding system fills them in. This is dependency inversion.
 *
 * RUNTIME properties track internal state that changes during
 * execution.
 */
USTRUCT(BlueprintType)
struct FWBSTTaskFollowInstanceData
{
	GENERATED_BODY()

	// --- INPUTS (bound in editor to evaluator outputs) ---

	/** The actor to follow. Bind to: Evaluator.PlayerPawn */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** Distance to target. Bind to: Evaluator.DistanceToPlayer */
	UPROPERTY(EditAnywhere, Category = "Input")
	float DistanceToTarget = 0.0f;

	// --- RUNTIME (internal state) ---

	/** Current speed tier — prevents redundant speed changes */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	EWBFollowSpeedTier CurrentSpeedTier = EWBFollowSpeedTier::None;

	/** Original MaxWalkSpeed, restored on ExitState */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	float OriginalMaxWalkSpeed = 0.0f;
};

/**
 * Follow Task — variable-speed pursuit with three speed tiers.
 *
 * This task replaces the built-in Move To with full control over
 * movement speed based on distance to the target.
 *
 * Architecture:
 *   EnterState: Save original speed, issue MoveToActor (auto-tracks)
 *   Tick: Read distance, pick speed tier, set MaxWalkSpeed if changed
 *   ExitState: Cancel move, restore original speed
 *
 * This task DOES tick, unlike the Idle task. Following a moving
 * target requires per-frame speed adjustment. However, the actual
 * pathfinding update is handled by MoveToActor's built-in tracking
 * — we don't re-request navigation every frame.
 *
 * All speed thresholds and values are editor-configurable.
 * A designer can tune walk/run/sprint speeds and distances
 * per companion type without touching C++.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Follow Task", Category = "WB|AI"))
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
	// --- Speed Configuration ---

	/** Movement speed (cm/s) when target is in the walk zone. */
	UPROPERTY(EditAnywhere, Category = "Speed")
	float WalkSpeed = 200.0f;

	/** Movement speed (cm/s) when target is in the run zone. */
	UPROPERTY(EditAnywhere, Category = "Speed")
	float RunSpeed = 450.0f;

	/** Movement speed (cm/s) when target is in the sprint zone. */
	UPROPERTY(EditAnywhere, Category = "Speed")
	float SprintSpeed = 700.0f;

	// --- Distance Thresholds ---

	/** Distance (cm) at which companion switches from walk to run. */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float RunDistanceThreshold = 600.0f;

	/** Distance (cm) at which companion switches from run to sprint. */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float SprintDistanceThreshold = 1200.0f;

	// --- Navigation ---

	/** How close the companion gets before stopping. */
	UPROPERTY(EditAnywhere, Category = "Navigation")
	float AcceptableRadius = 200.0f;

	/**
	 * Buffer zone (cm) for speed tier transitions.
	 * Prevents flickering when hovering at a distance boundary.
	 * E.g., with threshold 1200 and hysteresis 120:
	 *   Enter sprint at 1200+, leave sprint at 1080-.
	 */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float SpeedTierHysteresis = 120.0f;

	// --- Debug ---

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool EnableDebugLog = true;

	// --- External Data Handles ---

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
	TStateTreeExternalDataHandle<APawn> PawnHandle;

private:
	/** Determine the correct speed tier based on distance. */
	EWBFollowSpeedTier CalculateSpeedTier(
		float Distance, EWBFollowSpeedTier CurrentTier) const;

	/** Get the speed value for a given tier. */
	float GetSpeedForTier(EWBFollowSpeedTier Tier) const;
};
