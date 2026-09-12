// Copyright Epic Games, Inc. All Rights Reserved.

#include "Power/EnergyCore.h"

#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Engine/Engine.h"

AEnergyCore::AEnergyCore()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "EnergyCoreInteractionPrompt", "Nhặt lõi năng lượng");
}

void AEnergyCore::Interact_Implementation(AActor* Interactor)
{
	AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	if (!Character)
	{
		return;
	}

	if (!Character->PickupEnergyCore())
	{
		UE_LOG(LogECHO7, Display, TEXT("Bạn đã mang một lõi năng lượng"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Bạn đã mang một lõi năng lượng"));
		}
		return;
	}

	bIsPickedUp = true;
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	OnCorePickedUp.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("Đã nhặt lõi năng lượng"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Đã nhặt lõi năng lượng"));
	}
}

bool AEnergyCore::CanInteract_Implementation(AActor* Interactor) const
{
	const AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	return !bIsPickedUp && Character && !Character->HasEnergyCore();
}
