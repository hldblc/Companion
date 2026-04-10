// Copyright 2026 Halit Bilici. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WBCompanionEnums.generated.h"

// =============================================================
// WBCompanionEnums.h — Centralized enum definitions
//
// All gameplay enums for the companion AI system live here.
// This prevents circular #include dependencies between task,
// evaluator, and condition headers. Any file that needs an
// enum includes this single header.
//
// Convention:
//   - Prefix all enums with EWB
//   - Use UENUM(BlueprintType) so they're usable in Blueprints
//   - Use uint8 as the underlying type for memory efficiency
// =============================================================

/**
 * Speed tiers for the Follow task.
 * Determines how fast the companion moves based on distance
 * to the target player.
 */
UENUM(BlueprintType)
enum class EWBFollowSpeedTier : uint8
{
	None = 0   UMETA(Hidden),
	Walk       UMETA(DisplayName = "Walk"),
	Run        UMETA(DisplayName = "Run"),
	Sprint     UMETA(DisplayName = "Sprint")
};

/**
 * Companion behavior state — high-level state the companion is in.
 * Useful for UI, animation blueprints, and gameplay systems that
 * need to know what the companion is doing without querying the
 * State Tree directly.
 */
UENUM(BlueprintType)
enum class EWBCompanionState : uint8
{
	Idle = 0   UMETA(DisplayName = "Idle"),
	Following  UMETA(DisplayName = "Following"),
	Sprinting  UMETA(DisplayName = "Sprinting"),
	Gathering  UMETA(DisplayName = "Gathering"),
	Patrolling UMETA(DisplayName = "Patrolling"),
	Combat     UMETA(DisplayName = "Combat")
};
