#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RagdollMisc.generated.h"

struct FRagdollSnapshot;
class URagdollSyncComponent;

USTRUCT()
struct FBoneItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int16 BoneIndex = INDEX_NONE;
	UPROPERTY()
	FVector_NetQuantize10 Position = FVector::ZeroVector;
	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	UPROPERTY()
	float ServerTime = 0.f;

	void PostReplicatedAdd(const FRagdollSnapshot& InArraySerializer);
	void PostReplicatedChange(const FRagdollSnapshot& InArraySerializer);
	void PreReplicatedRemove(const FRagdollSnapshot& InArraySerializer);
};

USTRUCT()
struct FRagdollSnapshot : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FBoneItem> Bones;

	// For back-reference(Not Replicated).
	UPROPERTY(Transient, NotReplicated)
	URagdollSyncComponent* OwnerComp = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FBoneItem, FRagdollSnapshot>(Bones, DeltaParms, *this);
	}

	// Server only: Mark only changes as dirty to put them in the replication queue.
	bool AddOrUpdateBone(const int16 BoneIndex, const FVector& InPosition, const FQuat& InRotation,
	                     const float PosEps, const float       RotEpsDeg, const float   ServerNow)
	{
		FBoneItem* Found = Bones.FindByPredicate([&](const FBoneItem& It) { return It.BoneIndex == BoneIndex; });

		auto NearlySame = [&](const FBoneItem& It)-> bool
		{
			const bool  bPosOk    = FVector(It.Position - InPosition).SizeSquared() < FMath::Square(PosEps);
			const float AngErrDeg = It.Rotation.AngularDistance(InRotation) * (180.f / PI);
			return bPosOk && AngErrDeg < RotEpsDeg;
		};

		if (!Found)
		{
			FBoneItem NewItem;
			NewItem.BoneIndex  = BoneIndex;
			NewItem.Position   = InPosition;
			NewItem.Rotation   = InRotation;
			NewItem.ServerTime = ServerNow;
			const int32 NewIdx = Bones.Add(MoveTemp(NewItem));
			MarkItemDirty(Bones[NewIdx]); // Notify of new addition.
			return true;
		}

		if (!NearlySame(*Found))
		{
			Found->Position   = InPosition;
			Found->Rotation   = InRotation;
			Found->ServerTime = ServerNow;
			MarkItemDirty(*Found); // Notify of change(= delta send).
			return true;
		}

		return false; // No change -> Don't send.
	}
};

// Template specialization for FRagdollSnapshot.
template <>
struct TStructOpsTypeTraits<FRagdollSnapshot> : TStructOpsTypeTraitsBase2<FRagdollSnapshot>
{
	enum { WithNetDeltaSerializer = true };
};


USTRUCT()
struct FInterpBoneState
{
	GENERATED_BODY()

	FTransform Start        = FTransform::Identity;
	FTransform Target       = FTransform::Identity;
	float      StartTime    = 0.0;
	float      EndTime      = 0.0;
	float      ServerTime   = 0.f;
	bool       bInitialized = false;
	bool       bUpdated     = false;
};
