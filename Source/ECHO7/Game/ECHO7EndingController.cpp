// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/ECHO7EndingController.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "ECHO7.h"
#include "ECHO7PlayerController.h"
#include "Game/ChallengeProgressManager.h"
#include "Game/ECHO7Typewriter.h"
#include "Game/EvacuationZone.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/ECHO7EndingWidget.h"

namespace ECHO7Ending
{
	constexpr float TypewriterUpdateInterval = 0.02f;
	constexpr float FinalBlackFadeUpdateInterval = 0.02f;
	const FText EmptyEndingLinesFallback = NSLOCTEXT("ECHO7", "EmptyEndingLinesFallback", "CHƯA CÓ NỘI DUNG KẾT THÚC");
}

AECHO7EndingController::AECHO7EndingController()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

void AECHO7EndingController::BeginPlay()
{
	Super::BeginPlay();

	if (bTestEndingOnBeginPlay)
	{
		const float SafeTestDelay = FMath::Max(0.0f, TestEndingDelay);
		if (SafeTestDelay <= 0.0f)
		{
			StartEndingSequence();
		}
		else
		{
			GetWorldTimerManager().SetTimer(TestEndingTimer, this, &AECHO7EndingController::StartEndingSequence, SafeTestDelay, false);
		}
		return;
	}

	if (!IsValid(EvacuationZone))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController has no EvacuationZone assigned."));
		return;
	}

	EvacuationZone->OnEvacuationCompleted.AddUniqueDynamic(this, &AECHO7EndingController::HandleEvacuationCompleted);

	if (EvacuationZone->bEvacuationCompleted)
	{
		StartEndingSequence();
	}
}

void AECHO7EndingController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(EvacuationZone))
	{
		EvacuationZone->OnEvacuationCompleted.RemoveDynamic(this, &AECHO7EndingController::HandleEvacuationCompleted);
	}

	ClearEndingTimers();
	DisableEndingRestartInput();
	if (bHasStableControlRotation)
	{
		if (APlayerController* PlayerController = GetLocalPlayerController())
		{
			PlayerController->SetControlRotation(StableControlRotation);
		}
	}

	UnlockLocalPlayerInput();
	if (EndingWidget)
	{
		EndingWidget->RemoveFromParent();
		EndingWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AECHO7EndingController::HandleEvacuationCompleted()
{
	StartEndingSequence();
}

void AECHO7EndingController::StartEndingSequence()
{
	if (bEndingStarted)
	{
		return;
	}

	CaptureCompletionTime();

	// Do not lock the player until the ending widget and a local controller are usable.
	if (!PrepareEndingWidget() || !LockLocalPlayerInput())
	{
		if (EndingWidget)
		{
			EndingWidget->RemoveFromParent();
			EndingWidget = nullptr;
		}
		return;
	}

	bEndingStarted = true;
	BeginGlitchSequence();
}

void AECHO7EndingController::BeginGlitchSequence()
{
	if (!bEndingStarted)
	{
		return;
	}

	bGlitchActive = true;
	bTeleportPerformed = false;
	if (EndingWidget)
	{
		EndingWidget->SetStoryVisible(false);
		EndingWidget->SetFinalBlackOpacity(0.0f);
		EndingWidget->ClearGlitchVisuals();
		EndingWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	bHasStableControlRotation = false;
	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		StableControlRotation = PlayerController->GetControlRotation();
		bHasStableControlRotation = true;
	}

	ApplyGlitchPulse();

	const float SafeGlitchDuration = FMath::Max(0.0f, GlitchDuration);
	const float SafeTeleportTime = FMath::Clamp(TeleportTime, 0.0f, SafeGlitchDuration);
	if (SafeTeleportTime <= 0.0f)
	{
		TeleportPlayerDuringGlitch();
	}
	else
	{
		GetWorldTimerManager().SetTimer(TeleportTimer, this, &AECHO7EndingController::TeleportPlayerDuringGlitch, SafeTeleportTime, false);
	}

	if (SafeGlitchDuration <= 0.0f)
	{
		EndGlitchSequence();
	}
	else
	{
		GetWorldTimerManager().SetTimer(GlitchEndTimer, this, &AECHO7EndingController::EndGlitchSequence, SafeGlitchDuration, false);
	}
}

void AECHO7EndingController::ApplyGlitchPulse()
{
	if (!bEndingStarted || !bGlitchActive || !EndingWidget)
	{
		return;
	}

	const float ColorChoice = FMath::FRand();
	const float BrightChance = FMath::Clamp(BrightFlashChance, 0.0f, 1.0f);
	FLinearColor FlashColor = FLinearColor::Black;
	float FlashOpacity = FMath::FRandRange(0.35f, FMath::Max(0.35f, MaxDarkFlashOpacity));
	if (ColorChoice < BrightChance)
	{
		FlashColor = FLinearColor(0.82f, 0.92f, 1.0f, 1.0f);
		FlashOpacity = FMath::FRandRange(0.18f, 0.35f);
	}
	else if (ColorChoice < 0.42f)
	{
		FlashColor = FLinearColor(0.82f, 0.06f, 0.08f, 1.0f);
		FlashOpacity = FMath::FRandRange(0.16f, 0.32f);
	}
	else if (ColorChoice < 0.62f)
	{
		FlashColor = FLinearColor(0.02f, 0.72f, 0.90f, 1.0f);
		FlashOpacity = FMath::FRandRange(0.14f, 0.28f);
	}
	else if (ColorChoice < 0.78f)
	{
		FlashColor = FLinearColor::Black;
		FlashOpacity = FMath::FRandRange(FMath::Max(0.45f, MaxDarkFlashOpacity * 0.65f), FMath::Max(0.45f, MaxDarkFlashOpacity));
	}

	EndingWidget->SetGlitchFlash(FlashColor, FlashOpacity);
	EndingWidget->SetRandomGlitchStrips(FlashColor, FMath::RandRange(1, 3));

	if (bUseCameraJitter && bHasStableControlRotation)
	{
		if (APlayerController* PlayerController = GetLocalPlayerController())
		{
			const float SafeStrength = FMath::Max(0.0f, CameraJitterStrength);
			PlayerController->SetControlRotation(StableControlRotation + FRotator(
				FMath::FRandRange(-SafeStrength, SafeStrength),
				FMath::FRandRange(-SafeStrength, SafeStrength),
				0.0f));
		}
	}

	ScheduleNextGlitchPulse();
}

void AECHO7EndingController::ScheduleNextGlitchPulse()
{
	if (!bEndingStarted || !bGlitchActive)
	{
		return;
	}

	const float SafeMinInterval = FMath::Max(0.005f, FMath::Min(MinGlitchInterval, MaxGlitchInterval));
	const float SafeMaxInterval = FMath::Max(SafeMinInterval, FMath::Max(MinGlitchInterval, MaxGlitchInterval));
	GetWorldTimerManager().SetTimer(GlitchPulseTimer, this, &AECHO7EndingController::ApplyGlitchPulse, FMath::FRandRange(SafeMinInterval, SafeMaxInterval), false);
}

void AECHO7EndingController::TeleportPlayerDuringGlitch()
{
	if (!bEndingStarted || bTeleportPerformed)
	{
		return;
	}

	bTeleportPerformed = true;
	APlayerController* PlayerController = GetLocalPlayerController();
	APawn* PlayerPawn = GetEndingPlayerPawn();
	if (!IsValid(EndingTeleportPoint))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController has no EndingTeleportPoint assigned; skipping ending teleport."));
	}
	else if (!IsValid(PlayerPawn))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController could not teleport because the player pawn is unavailable."));
	}
	else
	{
		StopPlayerMovement(PlayerPawn);

		const FTransform DestinationTransform = EndingTeleportPoint->GetActorTransform();
		const FRotator DestinationRotation = bMatchTeleportPointRotation
			? DestinationTransform.Rotator()
			: PlayerPawn->GetActorRotation();
		PlayerPawn->SetActorLocationAndRotation(
			DestinationTransform.GetLocation(),
			DestinationRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);

		if (bMatchTeleportPointRotation && IsValid(PlayerController))
		{
			PlayerController->SetControlRotation(DestinationRotation);
			StableControlRotation = DestinationRotation;
			bHasStableControlRotation = true;
		}

		StopPlayerMovement(PlayerPawn);
	}

}

void AECHO7EndingController::EndGlitchSequence()
{
	if (!bEndingStarted || !bGlitchActive)
	{
		return;
	}

	if (!bTeleportPerformed)
	{
		TeleportPlayerDuringGlitch();
	}

	bGlitchActive = false;
	GetWorldTimerManager().ClearTimer(GlitchPulseTimer);
	GetWorldTimerManager().ClearTimer(TeleportTimer);
	if (EndingWidget)
	{
		EndingWidget->ClearGlitchVisuals();
	}
	if (bHasStableControlRotation)
	{
		if (APlayerController* PlayerController = GetLocalPlayerController())
		{
			PlayerController->SetControlRotation(StableControlRotation);
		}
	}

	const float SafeVisibleDuration = FMath::Max(0.0f, EndingSceneVisibleDuration);
	if (SafeVisibleDuration <= 0.0f)
	{
		StartFinalBlackFade();
		return;
	}

	GetWorldTimerManager().SetTimer(EndingSceneVisibleTimer, this, &AECHO7EndingController::StartFinalBlackFade, SafeVisibleDuration, false);
}

void AECHO7EndingController::StartFinalBlackFade()
{
	if (!bEndingStarted)
	{
		return;
	}

	FinalBlackFadeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (EndingWidget)
	{
		EndingWidget->SetFinalBlackOpacity(0.0f);
	}

	const float SafeFadeDuration = FMath::Max(0.0f, FinalBlackFadeDuration);
	if (SafeFadeDuration <= 0.0f)
	{
		CompleteFinalBlackFade();
		return;
	}

	UpdateFinalBlackFade();
	GetWorldTimerManager().SetTimer(FinalBlackFadeTimer, this, &AECHO7EndingController::UpdateFinalBlackFade, ECHO7Ending::FinalBlackFadeUpdateInterval, true);
}

void AECHO7EndingController::UpdateFinalBlackFade()
{
	if (!bEndingStarted)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float SafeFadeDuration = FMath::Max(0.0f, FinalBlackFadeDuration);
	const float ElapsedTime = World ? World->GetTimeSeconds() - FinalBlackFadeStartTime : SafeFadeDuration;
	const float FadeAlpha = SafeFadeDuration > 0.0f ? FMath::Clamp(ElapsedTime / SafeFadeDuration, 0.0f, 1.0f) : 1.0f;
	if (EndingWidget)
	{
		EndingWidget->SetFinalBlackOpacity(FadeAlpha);
	}

	if (FadeAlpha >= 1.0f)
	{
		CompleteFinalBlackFade();
	}
}

void AECHO7EndingController::CompleteFinalBlackFade()
{
	GetWorldTimerManager().ClearTimer(FinalBlackFadeTimer);
	if (!bEndingStarted)
	{
		return;
	}

	if (EndingWidget)
	{
		EndingWidget->SetFinalBlackOpacity(1.0f);
		EndingWidget->ClearGlitchVisuals();
	}

	const float SafeTextDelay = FMath::Max(0.0f, TextDelayAfterBlack);
	if (SafeTextDelay <= 0.0f)
	{
		BeginEndingText();
		return;
	}

	GetWorldTimerManager().SetTimer(TextStartTimer, this, &AECHO7EndingController::BeginEndingText, SafeTextDelay, false);
}

void AECHO7EndingController::BeginEndingText()
{
	if (!bEndingStarted || bEndingTextVisible)
	{
		return;
	}

	bEndingTextVisible = true;
	if (!EndingWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController has no prepared ending widget to display."));
		CompleteEndingText();
		return;
	}

	EndingWidget->SetEndingLineCount(EndingLines.Num());
	EndingWidget->ResetEndingText();
	EndingWidget->SetFinalBlackOpacity(1.0f);
	EndingWidget->SetStoryVisible(true);
	EndingWidget->SetRenderOpacity(1.0f);
	EndingWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CompletedEndingLines.Reset();
	CurrentEndingLineIndex = 0;
	bEndingTextCompleted = false;

	if (EndingLines.IsEmpty())
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController EndingLines is empty."));
		EndingWidget->SetEndingText(CompletedEndingLines, ECHO7Ending::EmptyEndingLinesFallback);
		CompleteEndingText();
		return;
	}

	BeginNextEndingLine();
}

void AECHO7EndingController::BeginNextEndingLine()
{
	if (!bEndingStarted || bEndingTextCompleted)
	{
		return;
	}

	if (!EndingLines.IsValidIndex(CurrentEndingLineIndex))
	{
		CompleteEndingText();
		return;
	}

	ActiveLineTypewriter.Begin(EndingLines[CurrentEndingLineIndex].Text);

	if (EndingWidget)
	{
		EndingWidget->SetEndingText(CompletedEndingLines, FText::GetEmpty());
	}

	if (!EndingLines[CurrentEndingLineIndex].Text.IsEmpty())
	{
		PlayOptionalSound(LineStartSound.Get());
	}

	if (ActiveLineTypewriter.IsEmpty())
	{
		CompleteCurrentEndingLine();
		return;
	}

	ActiveLineStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	GetWorldTimerManager().SetTimer(
		TypewriterTimer,
		this,
		&AECHO7EndingController::UpdateTypewriter,
		ECHO7Ending::TypewriterUpdateInterval,
		true);
}

void AECHO7EndingController::UpdateTypewriter()
{
	if (!bEndingStarted || bEndingTextCompleted || ActiveLineTypewriter.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 PreviousVisibleGraphemeCount = 0;
	int32 NewVisibleGraphemeCount = 0;
	if (ActiveLineTypewriter.Advance(World->GetTimeSeconds() - ActiveLineStartTime, CharactersPerSecond, PreviousVisibleGraphemeCount, NewVisibleGraphemeCount))
	{
		if (EndingWidget)
		{
			EndingWidget->SetEndingText(
				CompletedEndingLines,
				ActiveLineTypewriter.GetVisibleText());
		}

		if (ActiveLineTypewriter.ShouldPlayTypingSoundForRange(PreviousVisibleGraphemeCount, NewVisibleGraphemeCount, TypingSoundEveryNCharacters))
		{
			PlayOptionalSound(TypingSound.Get());
		}
	}

	if (ActiveLineTypewriter.IsComplete())
	{
		CompleteCurrentEndingLine();
	}
}

void AECHO7EndingController::CompleteCurrentEndingLine()
{
	if (!bEndingStarted || bEndingTextCompleted || !EndingLines.IsValidIndex(CurrentEndingLineIndex))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TypewriterTimer);
	CompletedEndingLines.Add(EndingLines[CurrentEndingLineIndex].Text);
	if (EndingWidget)
	{
		EndingWidget->SetEndingText(CompletedEndingLines, FText::GetEmpty());
	}

	const float DelayAfterLine = FMath::Max(0.0f, EndingLines[CurrentEndingLineIndex].DelayAfterLine);
	++CurrentEndingLineIndex;
	if (DelayAfterLine <= 0.0f)
	{
		BeginNextEndingLine();
		return;
	}

	GetWorldTimerManager().SetTimer(
		LineDelayTimer,
		this,
		&AECHO7EndingController::BeginNextEndingLine,
		DelayAfterLine,
		false);
}

void AECHO7EndingController::CompleteEndingText()
{
	if (bEndingTextCompleted)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TypewriterTimer);
	GetWorldTimerManager().ClearTimer(LineDelayTimer);
	bEndingTextCompleted = true;
	EnableEndingRestartInput();
	OnEndingTextCompleted.Broadcast();
}

void AECHO7EndingController::CaptureCompletionTime()
{
	FrozenCompletionTimeText = NSLOCTEXT("ECHO7", "EndingMissingCompletionTime", "00:00");
	AChallengeProgressManager* ProgressManager = Cast<AChallengeProgressManager>(
		UGameplayStatics::GetActorOfClass(this, AChallengeProgressManager::StaticClass()));
	if (!IsValid(ProgressManager))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_TIMER ending could not find ChallengeProgressManager; using 00:00."));
		return;
	}

	FrozenCompletionTimeText = ProgressManager->GetFormattedRunTime();
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_TIMER ending received completion time %s (frozen=%d)."),
		*FrozenCompletionTimeText.ToString(), ProgressManager->IsRunTimerCompleted());
}

void AECHO7EndingController::EnableEndingRestartInput()
{
	if (bRestartInputEnabled || !bEndingTextCompleted || !EndingWidget || !EndingWidget->IsInViewport())
	{
		return;
	}

	AECHO7PlayerController* PlayerController = Cast<AECHO7PlayerController>(GetLocalPlayerController());
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_RESTART final ending restart could not be enabled: local ECHO7PlayerController unavailable."));
		return;
	}

	PlayerController->SetActiveEndingRestartController(this);
	EndingWidget->SetRestartInputEnabled(true);
	FInputModeGameAndUI InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = false;
	EndingWidget->SetUserFocus(PlayerController);
	bRestartInputEnabled = true;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_RESTART ending restart enabled (Enter)."));
}

void AECHO7EndingController::DisableEndingRestartInput()
{
	if (EndingWidget)
	{
		EndingWidget->SetRestartInputEnabled(false);
	}
	if (AECHO7PlayerController* PlayerController = Cast<AECHO7PlayerController>(GetLocalPlayerController()))
	{
		PlayerController->ClearActiveEndingRestartController(this);
	}

	bRestartInputEnabled = false;
}

void AECHO7EndingController::HandleRestartInput()
{
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_RESTART restart key received (Enter; enabled=%d, ending-complete=%d, restart-requested=%d)."),
		bRestartInputEnabled,
		bEndingTextCompleted,
		bRestartRequested);

	if (!bRestartInputEnabled || !bEndingTextCompleted || bRestartRequested)
	{
		return;
	}

	RestartCurrentLevel();
}

void AECHO7EndingController::RestartCurrentLevel()
{
	if (bRestartRequested)
	{
		return;
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (CurrentLevelName.IsEmpty())
	{
		UE_LOG(LogECHO7, Error, TEXT("ECHO7_RESTART could not restart because the current level name is empty."));
		return;
	}
	bRestartRequested = true;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_RESTART restart requested."));

	// Leave the outgoing controller in a normal gameplay state as well as relying
	// on the full level reload to construct fresh input state.
	APlayerController* PlayerController = GetLocalPlayerController();
	DisableEndingRestartInput();
	UnlockLocalPlayerInput();
	if (IsValid(PlayerController))
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
	}

	UE_LOG(LogECHO7, Log, TEXT("ECHO7_RESTART map name reloaded: %s."), *CurrentLevelName);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevelName), true);
}

void AECHO7EndingController::PlayOptionalSound(USoundBase* Sound) const
{
	if (IsValid(Sound))
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

bool AECHO7EndingController::PrepareEndingWidget()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController could not prepare the ending widget because no local PlayerController is available."));
		return false;
	}

	if (!EndingWidget)
	{
		EndingWidget = CreateWidget<UECHO7EndingWidget>(PlayerController, UECHO7EndingWidget::StaticClass());
	}

	if (!EndingWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController failed to create UECHO7EndingWidget."));
		return false;
	}

	EndingWidget->ResetEndingText();
	EndingWidget->SetCompletionTime(FrozenCompletionTimeText);
	EndingWidget->SetRestartInputEnabled(false);
	EndingWidget->SetStoryVisible(false);
	EndingWidget->SetFinalBlackOpacity(0.0f);
	EndingWidget->ClearGlitchVisuals();
	EndingWidget->SetRenderOpacity(1.0f);
	EndingWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (!EndingWidget->IsInViewport() && !EndingWidget->AddToPlayerScreen(1000))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController could not add UECHO7EndingWidget to the player screen."));
		EndingWidget = nullptr;
		return false;
	}

	return true;
}

bool AECHO7EndingController::LockLocalPlayerInput()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController could not lock input because no local PlayerController is available."));
		return false;
	}

	LockedPlayerController = PlayerController;
	PlayerController->SetCinematicMode(true, false, false, true, true);

	if (APawn* PlayerPawn = PlayerController->GetPawn())
	{
		LockedPlayerPawn = PlayerPawn;
		PlayerPawn->DisableInput(PlayerController);
		StopPlayerMovement(PlayerPawn);
	}
	else
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7EndingController could not disable pawn input because the local player has no pawn."));
	}

	bInputLocked = true;
	return true;
}

void AECHO7EndingController::UnlockLocalPlayerInput()
{
	if (!bInputLocked)
	{
		return;
	}

	APlayerController* PlayerController = LockedPlayerController.Get();
	APawn* PlayerPawn = LockedPlayerPawn.Get();
	if (IsValid(PlayerController))
	{
		if (IsValid(PlayerPawn))
		{
			PlayerPawn->EnableInput(PlayerController);
		}

		// Only used during destruction/teardown, never after ending text completes.
		PlayerController->SetCinematicMode(false, false, false, true, true);
	}

	LockedPlayerPawn.Reset();
	LockedPlayerController.Reset();
	bInputLocked = false;
}

APlayerController* AECHO7EndingController::GetLocalPlayerController() const
{
	APlayerController* PlayerController = LockedPlayerController.Get();
	if (!IsValid(PlayerController))
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	return IsValid(PlayerController) && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

APawn* AECHO7EndingController::GetEndingPlayerPawn() const
{
	if (APawn* PlayerPawn = LockedPlayerPawn.Get())
	{
		return PlayerPawn;
	}

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		return PlayerController->GetPawn();
	}

	return nullptr;
}

void AECHO7EndingController::StopPlayerMovement(APawn* PlayerPawn) const
{
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
	{
		PlayerCharacter->StopJumping();
		if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}
	}
}

void AECHO7EndingController::ClearEndingTimers()
{
	GetWorldTimerManager().ClearTimer(GlitchPulseTimer);
	GetWorldTimerManager().ClearTimer(TeleportTimer);
	GetWorldTimerManager().ClearTimer(GlitchEndTimer);
	GetWorldTimerManager().ClearTimer(EndingSceneVisibleTimer);
	GetWorldTimerManager().ClearTimer(FinalBlackFadeTimer);
	GetWorldTimerManager().ClearTimer(TextStartTimer);
	GetWorldTimerManager().ClearTimer(TypewriterTimer);
	GetWorldTimerManager().ClearTimer(LineDelayTimer);
	GetWorldTimerManager().ClearTimer(TestEndingTimer);
}
