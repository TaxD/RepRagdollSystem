// Fill out your copyright notice in the Description page of Project Settings.


#include "RepRagdollSystemPlayerController.h"

#include "EnhancedInputSubsystems.h"

void ARepRagdollSystemPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		if (auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}

bool ARepRagdollSystemPlayerController::GetNetIORate(float& InBps, float& OutBps) const
{
	InBps = OutBps = 0.f;
	if (const UNetConnection* Conn = GetNetConnection())
	{
		InBps  = Conn->InBytesPerSecond;
		OutBps = Conn->OutBytesPerSecond;
		return true;
	}
	return false;
}
