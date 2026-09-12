// Copyright Epic Games, Inc. All Rights Reserved.


#include "ECHO7PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/ECHO7EndingController.h"
#include "InputMappingContext.h"
#include "ECHO7CameraManager.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace ECHO7Startup
{
	constexpr float CameraManagerRetryInterval = 0.05f;
	constexpr int32 MaxCameraManagerAttempts = 20;
}

AECHO7PlayerController::AECHO7PlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AECHO7CameraManager::StaticClass();
}

void AECHO7PlayerController::BeginPlay()
{
	Super::BeginPlay();
	TryStartStartupFade();

	
	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogECHO7, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AECHO7PlayerController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartupFadeRetryTimer);
	ActiveEndingRestartController.Reset();
	Super::EndPlay(EndPlayReason);
}

void AECHO7PlayerController::TryStartStartupFade()
{
	if (bStartupFadeStarted || !IsLocalPlayerController())
	{
		return;
	}

	if (PlayerCameraManager)
	{
		const float SafeFadeDuration = FMath::Max(0.01f, StartupFadeDuration);
		PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, SafeFadeDuration, FLinearColor::Black, false, false);
		bStartupFadeStarted = true;
		GetWorldTimerManager().ClearTimer(StartupFadeRetryTimer);
		return;
	}

	++StartupFadeAttemptCount;
	if (StartupFadeAttemptCount < ECHO7Startup::MaxCameraManagerAttempts)
	{
		GetWorldTimerManager().SetTimer(
			StartupFadeRetryTimer,
			this,
			&AECHO7PlayerController::TryStartStartupFade,
			ECHO7Startup::CameraManagerRetryInterval,
			false);
		return;
	}

	UE_LOG(LogECHO7, Warning, TEXT("Startup fade skipped because PlayerCameraManager was unavailable."));
}

void AECHO7PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		FInputKeyBinding& RestartBinding = InputComponent->BindKey(
			EKeys::Enter,
			IE_Pressed,
			this,
			&AECHO7PlayerController::HandleEndingRestartKey);
		// This handler is dormant outside the final ending and must not consume
		// Enter from any future gameplay binding.
		RestartBinding.bConsumeInput = false;
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

void AECHO7PlayerController::SetActiveEndingRestartController(AECHO7EndingController* EndingController)
{
	if (IsLocalPlayerController() && IsValid(EndingController))
	{
		ActiveEndingRestartController = EndingController;
	}
}

void AECHO7PlayerController::ClearActiveEndingRestartController(const AECHO7EndingController* EndingController)
{
	if (!EndingController || ActiveEndingRestartController.Get() == EndingController)
	{
		ActiveEndingRestartController.Reset();
	}
}

void AECHO7PlayerController::HandleEndingRestartKey()
{
	if (AECHO7EndingController* EndingController = ActiveEndingRestartController.Get())
	{
		EndingController->HandleRestartInput();
	}
}

bool AECHO7PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
