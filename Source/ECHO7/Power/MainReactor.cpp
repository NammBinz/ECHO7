// Copyright Epic Games, Inc. All Rights Reserved.

#include "Power/MainReactor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ECHO7.h"
#include "Engine/Engine.h"

AMainReactor::AMainReactor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMeshComponent->SetupAttachment(SceneRoot);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AMainReactor::RegisterInstalledCore()
{
	if (bIsOnline || InstalledCoreCount >= RequiredCoreCount)
	{
		return false;
	}

	InstalledCoreCount = FMath::Min(InstalledCoreCount + 1, RequiredCoreCount);
	OnReactorProgressChanged.Broadcast(InstalledCoreCount, RequiredCoreCount);

	const FString ProgressMessage = FString::Printf(
		TEXT("Lõi lò phản ứng: %d/%d"),
		InstalledCoreCount,
		RequiredCoreCount);
	UE_LOG(LogECHO7, Display, TEXT("%s"), *ProgressMessage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, ProgressMessage);
	}

	if (InstalledCoreCount >= RequiredCoreCount)
	{
		ActivateReactor();
	}

	return true;
}

void AMainReactor::ActivateReactor()
{
	if (bIsOnline)
	{
		return;
	}

	bIsOnline = true;
	OnReactorActivated.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("HỆ THỐNG NGUỒN ĐÃ HOẠT ĐỘNG"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("HỆ THỐNG NGUỒN ĐÃ HOẠT ĐỘNG"));
	}
}
