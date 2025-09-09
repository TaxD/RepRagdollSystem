// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RepRagdollSystemPlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class REPRAGDOLLSYSTEM_API ARepRagdollSystemPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

public:
	UFUNCTION(BlueprintPure, Category="Net")
	bool GetNetIORate(float& InBps, float& OutBps) const;
};
