// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeLinker.h"
#include "WBSTEval_PlayerContext.generated.h"

class APawn;

/**
 * Instance Data for the Player Context Evaluator.
 *
 * Outputs:
 *   PlayerPawn     — the specific player this companion belongs to
 *   DistanceToPlayer — distance between companion and its owner
 *
 * These are bound in the State Tree editor to drive transitions
 * and task inputs (e.g., "if DistanceToPlayer > 500, transition to Follow").
 */
USTRUCT(BlueprintType)
struct FWBSTEvalPlayerContextInstanceData
{
	GENERATED_BODY()

	/** The player pawn that owns this companion. Null if not yet assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	TObjectPtr<APawn> PlayerPawn = nullptr;

	/** Distance (cm) between the companion and its owning player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	float DistanceToPlayer = 0.0f;
};

/**
 * Player Context Evaluator — multiplayer-safe.
 *
 * Instead of GetPlayerPawn(World, 0) which FAILS on dedicated servers,
 * this evaluator reads the companion's OwnerPlayerPawn property.
 * That property is set by the server when the companion is spawned.
 *
 * Uses an external data handle to access the companion Pawn through
 * the Schema, then casts to AWBCompChar to read OwnerPlayerPawn.
 *
 * Performance notes:
 *   - TreeStart() caches the owner player reference once
 *   - Tick() only computes distance (cheap FVector::Dist)
 *   - If owner player dies/disconnects, Tick() detects nullptr
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Player Context Evaluator", Category = "WB|AI"))
struct COMPANION_API FWBSTEval_PlayerContext : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWBSTEvalPlayerContextInstanceData;

	virtual const UScriptStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	/** Resolve external data handles at initialization. */
	virtual bool Link(FStateTreeLinker& Linker) override;

	/** Called once when the State Tree starts. Caches the owner player. */
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;

	/** Called per frame. Computes distance to the cached owner player. */
	virtual void Tick(FStateTreeExecutionContext& Context,
	                  const float DeltaTime) const override;

protected:
	/**
	 * External data handle to the companion Pawn.
	 * Resolved by the Schema — gives us the possessed pawn
	 * (AWBCompChar) without any manual actor lookups.
	 */
	TStateTreeExternalDataHandle<APawn> CompanionPawnHandle;
};
