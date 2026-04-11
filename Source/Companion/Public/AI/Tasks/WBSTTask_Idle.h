// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeLinker.h"
#include "StateTreeTaskBase.h"
#include "WBSTTask_Idle.generated.h"

class APawn;

/**
 * Instance data for the Idle task.
 *
 * Kept minimal — no ElapsedIdleTime tracking.
 * Duration-based transitions are handled by the State Tree editor's
 * built-in Duration condition, not by C++ ticking.
 *
 * At scale (hundreds of companions), removing per-frame Tick()
 * from tasks that don't need it saves significant CPU.
 */
USTRUCT(BlueprintType)
struct FWBSTTaskIdleInstanceData
{
	GENERATED_BODY()

	// Reserved for future use (animation state, idle variation index, etc.)
};

/**
 * Idle Task — Tickless, multiplayer-safe.
 *
 * This task does exactly two things:
 *   EnterState: Stops movement (only if the character is actually moving)
 *   ExitState:  Logs departure (cleanup for future animation work)
 *
 * There is NO Tick(). The companion stays in Idle until a Transition
 * in the State Tree editor fires (driven by evaluator data like
 * DistanceToPlayer, or by the built-in Duration condition).
 *
 * Performance: O(0) per frame. Zero CPU cost while idling.
 * Compare to the old version which ticked every frame just to
 * increment a timer — now multiply that by 200 companions.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Idle Task", Category = "WB|AI|Tasks"))
struct COMPANION_API FWBSTTask_Idle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWBSTTaskIdleInstanceData;

	FWBSTTask_Idle();

	virtual const UScriptStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

	// NO Tick() override — intentionally tickless.

protected:
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool EnableDebugLog = true;

	/**
	 * Minimum velocity (cm/s) below which we skip StopMovement.
	 * Avoids redundant network updates when the companion is
	 * already standing still. Default 1.0 = practically zero.
	 */
	UPROPERTY(EditAnywhere, Category = "Config")
	float MinVelocityThreshold = 1.0f;

	TStateTreeExternalDataHandle<AAIController> AIControllerHandle;
	TStateTreeExternalDataHandle<APawn> PawnHandle;
};
