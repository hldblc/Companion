// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Companion/CPCompanion.h"

// Sets default values
ACPCompanion::ACPCompanion()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACPCompanion::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACPCompanion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACPCompanion::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

