// Fill out your copyright notice in the Description page of Project Settings.


#include "Ragdoll.h"

ARagdoll::ARagdoll()
{
	bReplicates = true;
	SetNetUpdateFrequency(30.f);
	SetNetDormancy(DORM_Initial);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	RootComponent         = SkeletalMeshComponent;
	SkeletalMeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));

	RagdollSyncComponent = CreateDefaultSubobject<URagdollSyncComponent>(TEXT("RagdollSyncComponent"));
}
