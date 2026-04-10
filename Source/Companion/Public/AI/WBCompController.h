// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WBCompController.generated.h"

class UStateTreeAIComponent;

/**
 * AWBCompController — AI Controller for the companion character.
 *
 * Owns the UStateTreeAIComponent which drives all AI behavior.
 * Does NOT tick — the StateTreeAIComponent has its own tick.
 */
UCLASS()
class COMPANION_API AWBCompController : public AAIController
{
	GENERATED_BODY()

public:
	AWBCompController();

	FORCEINLINE UStateTreeAIComponent* GetStateTreeAIComponent() const
	{
		return StateTreeAIComponent;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|StateTree",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;
};
