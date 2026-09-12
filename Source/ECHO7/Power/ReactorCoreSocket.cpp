// Copyright Epic Games, Inc. All Rights Reserved.

#include "Power/ReactorCoreSocket.h"

#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Engine/Engine.h"
#include "Power/MainReactor.h"

AReactorCoreSocket::AReactorCoreSocket()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "ReactorCoreSocketInteractionPrompt", "Lắp lõi năng lượng");
}

void AReactorCoreSocket::Interact_Implementation(AActor* Interactor)
{
	AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	if (!Character || bIsOccupied)
	{
		return;
	}

	if (!IsValid(MainReactor))
	{
		UE_LOG(LogECHO7, Warning, TEXT("Reactor Core Socket %d has no MainReactor assigned."), SocketIndex);
		return;
	}

	if (MainReactor->IsReactorOnline()
		|| MainReactor->GetInstalledCoreCount() >= MainReactor->GetRequiredCoreCount())
	{
		return;
	}

	if (!Character->ConsumeCarriedEnergyCore())
	{
		UE_LOG(LogECHO7, Display, TEXT("Cần lõi năng lượng"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Cần lõi năng lượng"));
		}
		return;
	}

	if (!MainReactor->RegisterInstalledCore())
	{
		UE_LOG(LogECHO7, Warning, TEXT("Reactor Core Socket %d could not register its Energy Core."), SocketIndex);
		return;
	}

	bIsOccupied = true;
	SetActorEnableCollision(false);
	OnCoreInserted.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("Đã lắp lõi năng lượng"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Đã lắp lõi năng lượng"));
	}
}

bool AReactorCoreSocket::CanInteract_Implementation(AActor* Interactor) const
{
	const AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	return !bIsOccupied && Character && Character->HasEnergyCore();
}
