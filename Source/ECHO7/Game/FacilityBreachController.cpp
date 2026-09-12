// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/FacilityBreachController.h"

#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "ECHO7.h"
#include "Engine/Light.h"
#include "Environment/SciFiDoor.h"
#include "Game/ChallengeProgressManager.h"
#include "Kismet/GameplayStatics.h"
#include "Power/MainReactor.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace FacilityBreach
{
	const FText WarningMessage = NSLOCTEXT("ECHO7", "FacilityBreachWarning", "CẢNH BÁO");
	const FText BreachMessage = NSLOCTEXT("ECHO7", "FacilityBreachIsolation", "SỰ CỐ KHU CÁCH LY");
	const FText SubjectMessage = NSLOCTEXT("ECHO7", "FacilityBreachSubject", "ĐỐI TƯỢNG 07 - TRẠNG THÁI: ĐANG HOẠT ĐỘNG");
	const FText EvacuationObjective = NSLOCTEXT("ECHO7", "FacilityBreachEvacuationObjective", "Quay lại khu vực sơ tán khẩn cấp.");
}

AFacilityBreachController::AFacilityBreachController()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

void AFacilityBreachController::BeginPlay()
{
	Super::BeginPlay();

	if (!ChallengeProgressManager || !bReactToFinalChallenge)
	{
		if (!ChallengeProgressManager)
		{
			UE_LOG(LogECHO7, Warning, TEXT("FacilityBreachController has no ChallengeProgressManager assigned."));
		}
		return;
	}

	ChallengeProgressManager->OnChallengeCompleted.AddUniqueDynamic(this, &AFacilityBreachController::HandleChallengeCompleted);

	const bool bManagerAlreadyComplete = ChallengeProgressManager->TotalChallengeCount > 0
		&& ChallengeProgressManager->CurrentCompletedChallengeCount >= ChallengeProgressManager->TotalChallengeCount;
	const bool bReactorAlreadyOnline = IsValid(ChallengeProgressManager->MainReactor)
		&& ChallengeProgressManager->MainReactor->IsReactorOnline();
	if (bManagerAlreadyComplete || bReactorAlreadyOnline)
	{
		StartBreachSequence();
	}
}

void AFacilityBreachController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->OnChallengeCompleted.RemoveDynamic(this, &AFacilityBreachController::HandleChallengeCompleted);
	}

	ClearBreachTimers();
	Super::EndPlay(EndPlayReason);
}

void AFacilityBreachController::HandleChallengeCompleted(int32 ChallengeIndex, int32 CurrentProgress, int32 TotalProgress)
{
	if (bReactToFinalChallenge && TotalProgress > 0 && CurrentProgress >= TotalProgress)
	{
		StartBreachSequence();
	}
}

void AFacilityBreachController::StartBreachSequence()
{
	if (bBreachStarted)
	{
		return;
	}

	bBreachStarted = true;
	const float SafeCalmDelay = FMath::Max(0.0f, CalmDelay);
	if (SafeCalmDelay <= 0.0f)
	{
		BeginEmergencyBreach();
		return;
	}

	GetWorldTimerManager().SetTimer(CalmDelayTimer, this, &AFacilityBreachController::BeginEmergencyBreach, SafeCalmDelay, false);
}

void AFacilityBreachController::BeginEmergencyBreach()
{
	if (!bBreachStarted || bBreachActive)
	{
		return;
	}

	bBreachActive = true;
	SetLightsEnabled(NormalLights, false);
	SetLightsEnabled(EmergencyLights, true);
	bEmergencyLightsVisible = true;
	LockConfiguredDoors();
	UnlockEscapeRouteDoors();
	PlayOptionalSound(AlarmSound.Get());
	OnBreachStarted.Broadcast();

	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowWarning(FacilityBreach::WarningMessage, FMath::Max(0.0f, WarningDuration));
	}

	const float SafeWarningDuration = FMath::Max(0.0f, WarningDuration);
	if (SafeWarningDuration <= 0.0f)
	{
		ShowBreachMessage();
	}
	else
	{
		GetWorldTimerManager().SetTimer(WarningTimer, this, &AFacilityBreachController::ShowBreachMessage, SafeWarningDuration, false);
	}

	const float SafeFlickerDuration = FMath::Max(0.0f, FlickerDuration);
	if (bUseEmergencyFlicker && SafeFlickerDuration > 0.0f)
	{
		const float SafeFlickerInterval = FMath::Max(0.01f, FlickerInterval);
		GetWorldTimerManager().SetTimer(FlickerTimer, this, &AFacilityBreachController::ToggleEmergencyLights, SafeFlickerInterval, true);
		GetWorldTimerManager().SetTimer(FlickerEndTimer, this, &AFacilityBreachController::FinishEmergencyFlicker, SafeFlickerDuration, false);
	}
}

void AFacilityBreachController::ShowBreachMessage()
{
	if (!bBreachActive)
	{
		return;
	}

	PlayOptionalSound(BreachSound.Get());
	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowWarning(FacilityBreach::BreachMessage, FMath::Max(0.0f, BreachMessageDuration));
	}

	const float SafeBreachMessageDuration = FMath::Max(0.0f, BreachMessageDuration);
	if (SafeBreachMessageDuration <= 0.0f)
	{
		ShowSubjectMessage();
	}
	else
	{
		GetWorldTimerManager().SetTimer(BreachMessageTimer, this, &AFacilityBreachController::ShowSubjectMessage, SafeBreachMessageDuration, false);
	}
}

void AFacilityBreachController::ShowSubjectMessage()
{
	if (!bBreachActive || bBreachSequenceCompleted)
	{
		return;
	}

	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowWarning(FacilityBreach::SubjectMessage, FMath::Max(0.0f, SubjectMessageDuration));
	}

	const float SafeSubjectMessageDuration = FMath::Max(0.0f, SubjectMessageDuration);
	if (SafeSubjectMessageDuration <= 0.0f)
	{
		FinishBreachSequence();
		return;
	}

	GetWorldTimerManager().SetTimer(
		SubjectMessageTimer,
		this,
		&AFacilityBreachController::FinishBreachSequence,
		SafeSubjectMessageDuration,
		false);
}

void AFacilityBreachController::FinishBreachSequence()
{
	if (!bBreachActive || bBreachSequenceCompleted)
	{
		return;
	}

	bBreachSequenceCompleted = true;
	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->SetObjective(FacilityBreach::EvacuationObjective);
	}

	OnBreachSequenceCompleted.Broadcast();
}

void AFacilityBreachController::ToggleEmergencyLights()
{
	if (!bBreachActive)
	{
		return;
	}

	bEmergencyLightsVisible = !bEmergencyLightsVisible;
	SetLightsEnabled(EmergencyLights, bEmergencyLightsVisible);
}

void AFacilityBreachController::FinishEmergencyFlicker()
{
	GetWorldTimerManager().ClearTimer(FlickerTimer);
	bEmergencyLightsVisible = true;
	SetLightsEnabled(EmergencyLights, true);
}

void AFacilityBreachController::SetLightsEnabled(const TArray<TObjectPtr<ALight>>& Lights, bool bEnabled) const
{
	for (ALight* Light : Lights)
	{
		if (!IsValid(Light))
		{
			continue;
		}

		if (ULightComponent* LightComponent = Light->GetLightComponent())
		{
			LightComponent->SetVisibility(bEnabled);
		}
	}
}

void AFacilityBreachController::UnlockEscapeRouteDoors()
{
	for (ASciFiDoor* Door : DoorsToUnlock)
	{
		if (IsValid(Door))
		{
			Door->UnlockDoor();
		}
	}
}

void AFacilityBreachController::LockConfiguredDoors()
{
	for (ASciFiDoor* Door : DoorsToCloseOrLock)
	{
		if (IsValid(Door))
		{
			Door->LockDoor();
		}
	}
}

void AFacilityBreachController::PlayOptionalSound(USoundBase* Sound) const
{
	if (IsValid(Sound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AFacilityBreachController::ClearBreachTimers()
{
	GetWorldTimerManager().ClearTimer(CalmDelayTimer);
	GetWorldTimerManager().ClearTimer(WarningTimer);
	GetWorldTimerManager().ClearTimer(BreachMessageTimer);
	GetWorldTimerManager().ClearTimer(SubjectMessageTimer);
	GetWorldTimerManager().ClearTimer(FlickerTimer);
	GetWorldTimerManager().ClearTimer(FlickerEndTimer);
}
