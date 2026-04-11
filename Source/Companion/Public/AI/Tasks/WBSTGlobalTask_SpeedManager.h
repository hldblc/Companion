// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeLinker.h"
#include "AI/Data/Enums/WBCompanionEnums.h"
#include "WBSTGlobalTask_SpeedManager.generated.h"

class APawn;

/**
 * Instance data for the Speed Manager global task.
 *
 * INPUT: DistanceToTarget is bound in the editor to the evaluator.
 * RUNTIME: CurrentSpeedTier and OriginalMaxWalkSpeed track state.
 */
USTRUCT(BlueprintType)
struct FWBSTGlobalTaskSpeedManagerInstanceData
{
	GENERATED_BODY()

	// --- INPUT (bound to evaluator in editor) ---

	/** Distance to the owning player. Bind to: Evaluator.DistanceToPlayer */
	UPROPERTY(EditAnywhere, Category = "Input")
	float DistanceToTarget = 0.0f;

	// --- RUNTIME ---

	/** Current speed tier — prevents redundant MaxWalkSpeed writes */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	EWBFollowSpeedTier CurrentSpeedTier = EWBFollowSpeedTier::None;

	/** Original MaxWalkSpeed, saved on tree start, restored on stop */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	float OriginalMaxWalkSpeed = 0.0f;

	/** Whether we've captured the original speed yet */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	bool bHasCapturedOriginalSpeed = false;
};

/**
 * Global Task: Speed Manager
 *
 * Runs across ALL states for the lifetime of the State Tree.
 * Reads distance from the evaluator and adjusts the companion's
 * MaxWalkSpeed based on three speed tiers with hysteresis.
 *
 * Why Global Task instead of per-state logic:
 *   - Single Responsibility: tasks do navigation, this does speed
 *   - No duplication: Follow, Patrol, Gather all get correct speed
 *   - One place to fix bugs, tune values, add new tiers
 *   - Designers can adjust all speed behavior from one node
 *
 * CRITICAL: Global Tasks must return Running from EnterState.
 * Returning Succeeded or Failed immediately stops the entire tree.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Speed Manager", Category = "WB|AI|GlobalTasks"))
struct COMPANION_API FWBSTGlobalTask_SpeedManager : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWBSTGlobalTaskSpeedManagerInstanceData;

	FWBSTGlobalTask_SpeedManager();

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
	// --- Speed Configuration (designer-tunable) ---

	UPROPERTY(EditAnywhere, Category = "Speed")
	float WalkSpeed = 225.0f;

	UPROPERTY(EditAnywhere, Category = "Speed")
	float RunSpeed = 450.0f;

	UPROPERTY(EditAnywhere, Category = "Speed")
	float SprintSpeed = 700.0f;

	// --- Distance Thresholds ---

	/** Distance (cm) at which companion switches from walk to run */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float RunDistanceThreshold = 600.0f;

	/** Distance (cm) at which companion switches from run to sprint */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float SprintDistanceThreshold = 1200.0f;

	/**
	 * Hysteresis buffer (cm) to prevent flickering at tier boundaries.
	 * Enter tier at threshold, leave tier at (threshold - buffer).
	 */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float SpeedTierHysteresis = 120.0f;

	/**
	 * Minimum distance below which speed is always Walk.
	 * When the companion is very close to the player (e.g., in Idle),
	 * we don't want sprint speed applied.
	 */
	UPROPERTY(EditAnywhere, Category = "Distance")
	float MinActiveDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool EnableDebugLog = true;

	// --- External Data ---

	TStateTreeExternalDataHandle<APawn> PawnHandle;

private:
	EWBFollowSpeedTier CalculateSpeedTier(
		float Distance, EWBFollowSpeedTier CurrentTier) const;

	float GetSpeedForTier(EWBFollowSpeedTier Tier) const;
};
