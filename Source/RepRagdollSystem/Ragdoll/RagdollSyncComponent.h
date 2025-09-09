// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RagdollMisc.h"
#include "Components/ActorComponent.h"
#include "RagdollSyncComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class REPRAGDOLLSYSTEM_API URagdollSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URagdollSyncComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float                        DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// List of "core bones" that the server will send.
	UPROPERTY(EditAnywhere, Category="RagdollSync")
	TArray<FName> DrivenBones = {
		TEXT("pelvis"),
		TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_05"),
		TEXT("head"),
		TEXT("clavicle_l"), TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"),
		TEXT("clavicle_r"), TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"),
		TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"),
		TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")
	};

	UPROPERTY(EditAnywhere, Category="RagdollSync", meta=(Units="hz", ClampMin="2.0", ClampMax="60.0"))
	float SendHz = 20.f;

	UPROPERTY(EditAnywhere, Category="RagdollSync", meta=(Units="cm"))
	float PositionChangeThreshold = 1.f;
	UPROPERTY(EditAnywhere, Category="RagdollSync", meta=(Units="deg", ClampMin="1.0", ClampMax="180.0"))
	float RotationChangeThreshold = 1.f;

	// Teleport threshold for large errors.
	UPROPERTY(EditAnywhere, Category="RagdollSync", meta=(Units="cm"))
	float TeleportPositionThreshold = 30.f;
	UPROPERTY(EditAnywhere, Category="RagdollSync", meta=(Units="deg", ClampMin="1.0", ClampMax="180.0"))
	float TeleportRotationThreshold = 45.f;


	// Playback delay (seconds) applied on clients so they render a little behind the server time
	// to absorb jitter. Default is roughly one send interval.
	UPROPERTY(EditAnywhere, Category="RagdollSync|Smoothing", meta=(Units="s", ClampMin="0.0", ClampMax="0.25"))
	float InterpDelaySeconds = 0.06f;

	UPROPERTY(EditAnywhere, Category="RagdollSync|Smoothing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EaseStrength = 0.2f;

public:
	UFUNCTION(BlueprintCallable)
	void StartRagdoll();
	UFUNCTION(BlueprintCallable)
	void StopRagdoll();

	void ApplySnapshot(const FBoneItem& Item);
	void ClearInterpState(const int16 BoneIndex) { InterpStates.Remove(BoneIndex); }

private:
	UPROPERTY(VisibleAnywhere, Category="RagdollSync")
	USkeletalMeshComponent* Mesh = nullptr;

	FTimerHandle SendTimer;
	void         ServerTickUpdate();

	FTimerHandle DormantTimer;


	UPROPERTY(ReplicatedUsing=OnRep_RagdollEnabled)
	bool bRagdollEnabled = false;
	UFUNCTION()
	void OnRep_RagdollEnabled();

	UPROPERTY(Replicated)
	FRagdollSnapshot CurrentSnapShot;

	// per-bone interpolation state.
	UPROPERTY(Transient)
	TMap<int16, FInterpBoneState> InterpStates;


	UPROPERTY(Transient)
	TMap<int16, FName> DrivenBindings;

	float GetServerTimeSeconds() const;

	static FTransform InterpTransform(const FTransform& A, const FTransform& B, float Alpha);
};
