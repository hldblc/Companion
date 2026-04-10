// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WBCompChar.generated.h"

/**
 * AWBCompChar — The AI Companion character.
 *
 * In a multiplayer survival game, each player owns one companion.
 * The companion stores an explicit reference to its owning player's pawn.
 * This reference is REPLICATED so both server and clients know the relationship.
 *
 * CRITICAL: Never use GetPlayerPawn(World, 0) for multiplayer AI.
 * Player index 0 is meaningless on a dedicated server.
 */
UCLASS()
class COMPANION_API AWBCompChar : public ACharacter
{
	GENERATED_BODY()

public:
	AWBCompChar();

	// ----------------------------------------------------------
	// Owner Player Reference — the core multiplayer-safe link
	// ----------------------------------------------------------

	/**
	 * Set the owning player's pawn. Called once when the companion
	 * is spawned by the game mode or player controller.
	 *
	 * Server-authoritative: only call this on the server.
	 * The UPROPERTY is Replicated, so clients receive the value
	 * automatically through Unreal's replication system.
	 */
	UFUNCTION(BlueprintCallable, Category = "WB|Companion")
	void SetOwnerPlayerPawn(APawn* InPlayerPawn);

	/** Get the owning player's pawn. Safe to call on server and clients. */
	UFUNCTION(BlueprintPure, Category = "WB|Companion")
	APawn* GetOwnerPlayerPawn() const { return OwnerPlayerPawn; }

protected:
	virtual void BeginPlay() override;

	/** Required for replication of OwnerPlayerPawn. */
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	/**
	 * The player pawn that owns this companion.
	 * Replicated so clients can read it for UI, targeting, etc.
	 *
	 * UPROPERTY breakdown:
	 *   Replicated = synced from server to all clients
	 *   BlueprintReadOnly = Blueprints can read but not overwrite
	 *   Category = organizes in the Details panel
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "WB|Companion",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APawn> OwnerPlayerPawn;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WB|Debug")
	FString DebugText;
};
