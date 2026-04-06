// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WBCompChar.generated.h"

UCLASS()
class COMPANION_API AWBCompChar : public ACharacter
{
	GENERATED_BODY()
public:
	AWBCompChar();
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:
	virtual void BeginPlay() override;


};
