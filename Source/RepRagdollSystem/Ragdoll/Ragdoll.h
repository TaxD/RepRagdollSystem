// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RagdollSyncComponent.h"
#include "GameFramework/Actor.h"
#include "Ragdoll.generated.h"


UCLASS()
class REPRAGDOLLSYSTEM_API ARagdoll : public AActor
{
	GENERATED_BODY()

public:
	ARagdoll();

	USkeletalMeshComponent* GetMesh() const { return SkeletalMeshComponent; }

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<URagdollSyncComponent> RagdollSyncComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
};
