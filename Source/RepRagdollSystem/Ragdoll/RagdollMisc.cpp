#include "RagdollMisc.h"

#include "RagdollSyncComponent.h"

void FBoneItem::PostReplicatedAdd(const FRagdollSnapshot& InArraySerializer)
{
	if (URagdollSyncComponent* Comp = InArraySerializer.OwnerComp)
	{
		Comp->ApplySnapshot(*this);
	}
}

void FBoneItem::PostReplicatedChange(const FRagdollSnapshot& InArraySerializer)
{
	if (URagdollSyncComponent* Comp = InArraySerializer.OwnerComp)
	{
		Comp->ApplySnapshot(*this);
	}
}

void FBoneItem::PreReplicatedRemove(const FRagdollSnapshot& InArraySerializer)
{
	if (URagdollSyncComponent* Comp = InArraySerializer.OwnerComp)
	{
		Comp->ClearInterpState(BoneIndex);
	}
}
