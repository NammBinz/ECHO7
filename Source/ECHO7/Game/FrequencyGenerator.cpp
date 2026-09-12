// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/FrequencyGenerator.h"

#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Environment/SciFiDoor.h"
#include "Game/FrequencySyncController.h"
#include "GameFramework/PlayerController.h"
#include "UI/ECHO7FrequencyWidget.h"

AFrequencyGenerator::AFrequencyGenerator()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "FrequencyGeneratorInteractionPrompt", "Hiệu chỉnh máy phát");
	GeneratorDisplayName = NSLOCTEXT("ECHO7", "FrequencyGeneratorDisplayName", "TRẠM NĂNG LƯỢNG");
}

void AFrequencyGenerator::SetStageEnabled(bool bEnabled)
{
	const bool bShouldBeEnabled = bEnabled && !bStabilized;
	if (bStageEnabled == bShouldBeEnabled)
	{
		return;
	}

	bStageEnabled = bShouldBeEnabled;
	if (!bStageEnabled && bCalibrationOpen)
	{
		CloseCalibration();
	}
}

void AFrequencyGenerator::Interact_Implementation(AActor* Interactor)
{
	StartCalibration(Interactor);
}

bool AFrequencyGenerator::StartCalibration(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	const AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	APlayerController* PlayerController = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencyGenerator could not find a local PlayerController."));
		return false;
	}

	OpenCalibration(PlayerController);
	return bCalibrationOpen;
}

bool AFrequencyGenerator::CanInteract_Implementation(AActor* Interactor) const
{
	return bStageEnabled && IsValid(Interactor) && !bStabilized && !bCalibrationOpen && Cast<AECHO7Character>(Interactor) != nullptr;
}

void AFrequencyGenerator::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	CloseCalibration();
	Super::EndPlay(EndPlayReason);
}

void AFrequencyGenerator::OpenCalibration(APlayerController* PlayerController)
{
	if (!PlayerController || bCalibrationOpen || ActiveFrequencyWidget)
	{
		return;
	}

	CurrentFrequency = FMath::Clamp(StartingFrequency, GetMinimumFrequency(), GetMaximumFrequency());
	StableTime = 0.0f;
	bWasInTargetRange = false;

	ActiveFrequencyWidget = CreateWidget<UECHO7FrequencyWidget>(PlayerController, UECHO7FrequencyWidget::StaticClass());
	if (!ActiveFrequencyWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencyGenerator failed to create its frequency widget."));
		return;
	}

	ActivePlayerController = PlayerController;
	ActiveFrequencyWidget->OnCalibrationTick.AddDynamic(this, &AFrequencyGenerator::HandleCalibrationTick);
	ActiveFrequencyWidget->OnCalibrationCancelled.AddDynamic(this, &AFrequencyGenerator::HandleCalibrationCancelled);
	ActiveFrequencyWidget->OnCalibrationSucceeded.AddDynamic(this, &AFrequencyGenerator::HandleCalibrationSucceeded);
	ActiveFrequencyWidget->ConfigureOpeningGlitch(CalibrationOpenGlitchDuration);
	ActiveFrequencyWidget->InitializeCalibration(
		GetMinimumFrequency(),
		GetMaximumFrequency(),
		GetClampedTargetFrequency(),
		GetClampedTolerance(),
		CurrentFrequency,
		GetRequiredDuration());

	if (!ActiveFrequencyWidget->AddToPlayerScreen(20))
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencyGenerator failed to add its frequency widget to the player screen."));
		ActiveFrequencyWidget->OnCalibrationTick.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationTick);
		ActiveFrequencyWidget->OnCalibrationCancelled.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationCancelled);
		ActiveFrequencyWidget->OnCalibrationSucceeded.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationSucceeded);
		ActiveFrequencyWidget = nullptr;
		ActivePlayerController = nullptr;
		return;
	}

	bCalibrationOpen = true;
	PlayerController->bShowMouseCursor = false;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	bInputCaptured = true;

	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	ActiveFrequencyWidget->SetKeyboardFocus();
	UpdateFrequencyWidget();
	OnCalibrationStarted.Broadcast();
}

void AFrequencyGenerator::CloseCalibration()
{
	if (ActiveFrequencyWidget)
	{
		ActiveFrequencyWidget->OnCalibrationTick.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationTick);
		ActiveFrequencyWidget->OnCalibrationCancelled.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationCancelled);
		ActiveFrequencyWidget->OnCalibrationSucceeded.RemoveDynamic(this, &AFrequencyGenerator::HandleCalibrationSucceeded);
		ActiveFrequencyWidget->RemoveFromParent();
		ActiveFrequencyWidget = nullptr;
	}

	RestorePlayerInput();
	bCalibrationOpen = false;
}

void AFrequencyGenerator::RestorePlayerInput()
{
	if (bInputCaptured && IsValid(ActivePlayerController))
	{
		ActivePlayerController->SetIgnoreMoveInput(false);
		ActivePlayerController->SetIgnoreLookInput(false);
		ActivePlayerController->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		ActivePlayerController->SetInputMode(InputMode);
	}

	bInputCaptured = false;
	ActivePlayerController = nullptr;
}

void AFrequencyGenerator::HandleCalibrationTick(bool bIncreaseFrequency, float DeltaSeconds)
{
	if (!bStageEnabled || !bCalibrationOpen || bStabilized)
	{
		return;
	}

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float Rate = bIncreaseFrequency ? FMath::Max(0.0f, IncreaseRate) : -FMath::Max(0.0f, DecreaseRate);
	CurrentFrequency = FMath::Clamp(CurrentFrequency + Rate * SafeDeltaSeconds, GetMinimumFrequency(), GetMaximumFrequency());

	const bool bIsInTargetRange = IsFrequencyInTargetRange();
	if (bIsInTargetRange)
	{
		if (!bWasInTargetRange)
		{
			OnFrequencyEnteredTarget.Broadcast();
		}

		StableTime += SafeDeltaSeconds;
	}
	else
	{
		if (bWasInTargetRange)
		{
			OnFrequencyLeftTarget.Broadcast();
		}

		StableTime = 0.0f;
	}

	bWasInTargetRange = bIsInTargetRange;
	UpdateFrequencyWidget();

	if (StableTime >= GetRequiredDuration())
	{
		StabilizeGenerator();
	}
}

void AFrequencyGenerator::HandleCalibrationCancelled()
{
	CloseCalibration();
}

void AFrequencyGenerator::HandleCalibrationSucceeded()
{
	CloseCalibration();
}

void AFrequencyGenerator::StabilizeGenerator()
{
	if (bStabilized)
	{
		return;
	}

	bStabilized = true;
	bStageEnabled = false;
	ApplyStageCompletionActions();
	OnGeneratorStabilized.Broadcast();

	if (FrequencySyncController)
	{
		FrequencySyncController->NotifyGeneratorStabilized(this);
	}
	else
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencyGenerator stabilized without a FrequencySyncController."));
	}

	if (ActiveFrequencyWidget)
	{
		ActiveFrequencyWidget->ShowSuccess();
	}
	else
	{
		CloseCalibration();
	}
}

void AFrequencyGenerator::ApplyStageCompletionActions()
{
	bool bUnlockedDoor = false;
	for (ASciFiDoor* Door : DoorsToUnlock)
	{
		if (!IsValid(Door))
		{
			continue;
		}

		Door->UnlockDoor();
		if (bOpenUnlockedDoorsImmediately)
		{
			Door->OpenDoor();
		}
		bUnlockedDoor = true;
	}
	if (bUnlockedDoor)
	{
		OnStageUnlocked.Broadcast();
	}

	bool bDisabledObstacle = false;
	for (AActor* ActorToDisable : ActorsToDisable)
	{
		if (IsValid(ActorToDisable))
		{
			ActorToDisable->SetActorHiddenInGame(true);
			ActorToDisable->SetActorEnableCollision(false);
			bDisabledObstacle = true;
		}
	}
	if (bDisabledObstacle)
	{
		OnStageObstacleDisabled.Broadcast();
	}

	for (AActor* ActorToEnable : ActorsToEnable)
	{
		if (IsValid(ActorToEnable))
		{
			ActorToEnable->SetActorHiddenInGame(false);
			ActorToEnable->SetActorEnableCollision(true);
		}
	}

	OnStageCompleted.Broadcast();
}

void AFrequencyGenerator::UpdateFrequencyWidget() const
{
	if (ActiveFrequencyWidget)
	{
		ActiveFrequencyWidget->UpdateCalibrationState(CurrentFrequency, StableTime);
	}
}

float AFrequencyGenerator::GetMinimumFrequency() const
{
	return FMath::Min(MinFrequency, MaxFrequency);
}

float AFrequencyGenerator::GetMaximumFrequency() const
{
	const float MinimumFrequency = GetMinimumFrequency();
	const float MaximumFrequency = FMath::Max(MinFrequency, MaxFrequency);
	return FMath::IsNearlyEqual(MinimumFrequency, MaximumFrequency) ? MinimumFrequency + 1.0f : MaximumFrequency;
}

float AFrequencyGenerator::GetClampedTargetFrequency() const
{
	return FMath::Clamp(TargetFrequency, GetMinimumFrequency(), GetMaximumFrequency());
}

float AFrequencyGenerator::GetClampedTolerance() const
{
	const float Target = GetClampedTargetFrequency();
	return FMath::Min(FMath::Max(0.0f, TargetTolerance), FMath::Min(Target - GetMinimumFrequency(), GetMaximumFrequency() - Target));
}

float AFrequencyGenerator::GetRequiredDuration() const
{
	return FMath::Max(0.01f, RequiredStableDuration);
}

bool AFrequencyGenerator::IsFrequencyInTargetRange() const
{
	const float Target = GetClampedTargetFrequency();
	const float Tolerance = GetClampedTolerance();
	return CurrentFrequency >= Target - Tolerance && CurrentFrequency <= Target + Tolerance;
}
