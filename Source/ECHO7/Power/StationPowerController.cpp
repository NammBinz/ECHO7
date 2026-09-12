// Copyright Epic Games, Inc. All Rights Reserved.

#include "Power/StationPowerController.h"

#include "Components/LightComponent.h"
#include "ECHO7.h"
#include "Engine/Engine.h"
#include "Engine/Light.h"
#include "Engine/World.h"
#include "Power/MainReactor.h"
#include "TimerManager.h"

AStationPowerController::AStationPowerController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStationPowerController::BeginPlay()
{
	Super::BeginPlay();

	SetEmergencyLightsOn();
	SetLightsEnabled(PoweredLights, false);

	if (!MainReactor)
	{
		UE_LOG(LogECHO7, Warning, TEXT("StationPowerController has no MainReactor assigned."));
		return;
	}

	MainReactor->OnReactorActivated.AddUniqueDynamic(this, &AStationPowerController::HandleReactorActivated);
	if (MainReactor->IsReactorOnline())
	{
		StartPowerRestoration();
	}
}

void AStationPowerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (MainReactor)
	{
		MainReactor->OnReactorActivated.RemoveDynamic(this, &AStationPowerController::HandleReactorActivated);
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void AStationPowerController::HandleReactorActivated()
{
	StartPowerRestoration();
}

void AStationPowerController::StartPowerRestoration()
{
	if (bPowerRestored || bPowerRestorationInProgress)
	{
		return;
	}

	bPowerRestorationInProgress = true;
	OnPowerRestorationStarted.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("ĐANG KHÔI PHỤC NGUỒN ĐIỆN..."));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("ĐANG KHÔI PHỤC NGUỒN ĐIỆN..."));
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(FirstEmergencyOffTimer, this, &AStationPowerController::SetEmergencyLightsOff, 0.2f, false);
	TimerManager.SetTimer(FirstEmergencyOnTimer, this, &AStationPowerController::SetEmergencyLightsOn, 0.35f, false);
	TimerManager.SetTimer(SecondEmergencyOffTimer, this, &AStationPowerController::SetEmergencyLightsOff, 0.5f, false);
	TimerManager.SetTimer(SecondEmergencyOnTimer, this, &AStationPowerController::SetEmergencyLightsOn, 0.7f, false);
	TimerManager.SetTimer(FinalEmergencyOffTimer, this, &AStationPowerController::SetEmergencyLightsOff, 1.0f, false);
	TimerManager.SetTimer(PoweredLightsOnTimer, this, &AStationPowerController::EnablePoweredLights, 1.15f, false);
	TimerManager.SetTimer(CompletionTimer, this, &AStationPowerController::CompletePowerRestoration, 1.4f, false);
}

void AStationPowerController::SetEmergencyLightsOn()
{
	SetLightsEnabled(EmergencyLights, true);
}

void AStationPowerController::SetEmergencyLightsOff()
{
	SetLightsEnabled(EmergencyLights, false);
}

void AStationPowerController::EnablePoweredLights()
{
	SetLightsEnabled(PoweredLights, true);
}

void AStationPowerController::CompletePowerRestoration()
{
	if (bPowerRestored)
	{
		return;
	}

	SetEmergencyLightsOff();
	EnablePoweredLights();

	for (AActor* Actor : ActorsToEnableOnPower)
	{
		if (IsValid(Actor))
		{
			Actor->SetActorHiddenInGame(false);
		}
	}

	for (AActor* Actor : ActorsToDisableOnPower)
	{
		if (IsValid(Actor))
		{
			Actor->SetActorHiddenInGame(true);
		}
	}

	bPowerRestored = true;
	bPowerRestorationInProgress = false;
	OnPowerRestored.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("NGUỒN ĐIỆN ĐÃ ĐƯỢC KHÔI PHỤC"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("NGUỒN ĐIỆN ĐÃ ĐƯỢC KHÔI PHỤC"));
	}
}

void AStationPowerController::SetLightsEnabled(const TArray<TObjectPtr<ALight>>& Lights, bool bEnabled) const
{
	for (ALight* Light : Lights)
	{
		if (IsValid(Light))
		{
			if (ULightComponent* LightComponent = Light->GetLightComponent())
			{
				LightComponent->SetVisibility(bEnabled);
			}
		}
	}
}
