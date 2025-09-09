// Fill out your copyright notice in the Description page of Project Settings.


#include "RagdollSyncComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

URagdollSyncComponent::URagdollSyncComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick          = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}


void URagdollSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	Mesh = GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("RagdollSyncComponent: No SkeletalMeshComponent found in Owner!"));
		return;
	}

	CurrentSnapShot.OwnerComp = this;

	DrivenBindings.Reset();
	for (const FName& N : DrivenBones)
	{
		const int32 Index = Mesh->GetBoneIndex(N);
		if (Index != INDEX_NONE)
		{
			DrivenBindings.Add(Index, N);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RagdollSyncComponent: Driven bone not found: %s"), *N.ToString());
		}
	}
}

void URagdollSyncComponent::TickComponent(float                        DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority() || !Mesh || !bRagdollEnabled) return;

	const float ServerNow   = GetServerTimeSeconds();
	const float PlaybackNow = ServerNow - InterpDelaySeconds; // render slightly behind server to hide jitter

	for (auto& Pair : InterpStates)
	{
		const int16       BoneIndex = Pair.Key;
		FInterpBoneState& State     = Pair.Value;
		if (!State.bInitialized || !State.bUpdated) continue;

		const FName*   BoneNamePtr = DrivenBindings.Find(BoneIndex);
		FBodyInstance* BI          = Mesh->GetBodyInstance(BoneNamePtr ? *BoneNamePtr : NAME_None);
		if (!BI) continue;

		const float Duration = FMath::Max(1e-3, State.EndTime - State.StartTime);
		const float RawAlpha = FMath::Clamp((PlaybackNow - State.StartTime) / Duration, 0.f, 1.f);
		const float Alpha    = FMath::Lerp(RawAlpha, FMath::SmoothStep(0.f, 1.f, RawAlpha), EaseStrength);

		const FTransform Current = InterpTransform(State.Start, State.Target, Alpha);

		const FTransform PrevWT       = BI->GetUnrealWorldTransform();
		const float      PositonError = FVector::Dist(PrevWT.GetLocation(), Current.GetLocation());
		const float      AngularError = PrevWT.GetRotation().AngularDistance(Current.GetRotation()) * (180.f / PI);

		const ETeleportType TeleportType =
				(PositonError > TeleportPositionThreshold || AngularError > TeleportRotationThreshold)
					? ETeleportType::TeleportPhysics
					: ETeleportType::None;

		BI->SetBodyTransform(Current, TeleportType);


		if (RawAlpha >= 1.0)
		{
			State.bUpdated = false;
		}
	}
}

void URagdollSyncComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URagdollSyncComponent, bRagdollEnabled);
	DOREPLIFETIME(URagdollSyncComponent, CurrentSnapShot);
}

void URagdollSyncComponent::StartRagdoll()
{
	if (!GetOwner()->HasAuthority() || !Mesh) return;

	GetOwner()->SetNetDormancy(DORM_Awake);
	GetOwner()->SetReplicateMovement(false);
	bRagdollEnabled = true;
	OnRep_RagdollEnabled();

	const float Interval = 1.f / FMath::Max(2.f, SendHz);
	GetWorld()->GetTimerManager().SetTimer(SendTimer, this, &URagdollSyncComponent::ServerTickUpdate,
	                                       Interval, true);
}

void URagdollSyncComponent::StopRagdoll()
{
	if (!GetOwner()->HasAuthority() || !Mesh) return;

	GetWorld()->GetTimerManager().ClearTimer(SendTimer);
	GetOwner()->SetReplicateMovement(true);
	bRagdollEnabled = false;
	OnRep_RagdollEnabled();

	CurrentSnapShot.Bones.Reset();
	CurrentSnapShot.MarkArrayDirty();
	GetOwner()->FlushNetDormancy();
	GetOwner()->SetNetDormancy(DORM_DormantAll);

	InterpStates.Reset();
}

void URagdollSyncComponent::ApplySnapshot(const FBoneItem& Item)
{
	// Server ignores this(prioritizes its own simulation).
	if (GetOwner()->HasAuthority() || !Mesh) return;

	FInterpBoneState& State = InterpStates.FindOrAdd(Item.BoneIndex);

	if (Item.ServerTime <= State.ServerTime) return;
	State.ServerTime = Item.ServerTime;

	const float ServerNow   = GetServerTimeSeconds();
	const float PlaybackNow = ServerNow - InterpDelaySeconds;

	const FTransform Target(Item.Rotation, FVector(Item.Position), Mesh->GetComponentTransform().GetScale3D());

	if (!State.bInitialized)
	{
		State.Start        = Target;
		State.Target       = Target;
		State.StartTime    = Item.ServerTime;
		State.EndTime      = Item.ServerTime + InterpDelaySeconds;
		State.bInitialized = true;
		State.bUpdated     = true;
	}
	else
	{
		const float      PrevDuration = FMath::Max(1e-3f, State.EndTime - State.StartTime);
		const float      AlphaNow     = FMath::Clamp((PlaybackNow - State.StartTime) / PrevDuration, 0.f, 1.f);
		const FTransform Current      = InterpTransform(State.Start, State.Target, AlphaNow);

		State.Start     = Current;
		State.Target    = Target;
		State.StartTime = PlaybackNow; // start new segment from where we currently are (playback timeline)

		// Aim to land at the new snapshot's server time + smoothing window,
		// but never set an end time in the past.
		const float DesiredEnd = Item.ServerTime + InterpDelaySeconds;
		State.EndTime          = FMath::Max(PlaybackNow + 1e-3f, DesiredEnd);
		State.bUpdated         = true;
	}
}


void URagdollSyncComponent::ServerTickUpdate()
{
	if (!Mesh) return;

	bool        bAnyDirty = false;
	const float ServerNow = GetWorld()->GetTimeSeconds();

	for (const TPair<short, FName>& Bone : DrivenBindings)
	{
		FBodyInstance* BI = Mesh->GetBodyInstance(Bone.Value);
		if (!BI || !BI->IsInstanceSimulatingPhysics()) continue;

		const FTransform WT = BI->GetUnrealWorldTransform();
		bAnyDirty |= CurrentSnapShot.AddOrUpdateBone(
			Bone.Key,
			WT.GetLocation(),
			WT.GetRotation(),
			PositionChangeThreshold,
			RotationChangeThreshold,
			ServerNow
		);
	}

	if (AActor* OwnerActor = GetOwner())
	{
		if (bAnyDirty && OwnerActor->NetDormancy == DORM_DormantAll)
		{
			GetWorld()->GetTimerManager().ClearTimer(DormantTimer);
			OwnerActor->SetNetDormancy(DORM_Awake);
			OwnerActor->FlushNetDormancy();
		}
		else if (!bAnyDirty && OwnerActor->NetDormancy != DORM_DormantAll)
		{
			GetWorld()->GetTimerManager().SetTimer(DormantTimer, [OwnerActor]
			{
				if (OwnerActor) OwnerActor->SetNetDormancy(DORM_DormantAll);
			}, .5f, false);
		}
	}
}

void URagdollSyncComponent::OnRep_RagdollEnabled()
{
	if (Mesh) Mesh->SetSimulatePhysics(bRagdollEnabled);

	if (!bRagdollEnabled) InterpStates.Reset();
	SetComponentTickEnabled(bRagdollEnabled);
}

float URagdollSyncComponent::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	const AGameStateBase* GS = World->GetGameState();
	if (!GS) return World->GetTimeSeconds();

	return GS->GetServerWorldTimeSeconds();
}

FTransform URagdollSyncComponent::InterpTransform(const FTransform& A, const FTransform& B, const float Alpha)
{
	FTransform Out;
	Out.SetLocation(FMath::InterpCircularOut(A.GetLocation(), B.GetLocation(), Alpha));
	Out.SetRotation(FQuat::Slerp(A.GetRotation(), B.GetRotation(), Alpha).GetNormalized());
	Out.SetScale3D(FMath::InterpCircularOut(A.GetScale3D(), B.GetScale3D(), Alpha));
	return Out;
}
