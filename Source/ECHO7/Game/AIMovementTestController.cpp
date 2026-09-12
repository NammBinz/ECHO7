// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/AIMovementTestController.h"

#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Game/ChallengeProgressManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/ECHO7StoryOverlayWidget.h"

namespace AIMovementTest
{
	constexpr float TutorialTypewriterUpdateInterval = 0.02f;
	constexpr int32 TutorialTypingSoundEveryNCharacters = 2;
	const FText TestStartedMessage = NSLOCTEXT("ECHO7", "AIMovementTestStarted", "BẮT ĐẦU THỬ NGHIỆM");
	const FText MovePhaseMessage = NSLOCTEXT("ECHO7", "AIMovementTestMove", "DI CHUYỂN");
	const FText MovementDetectedMessage = NSLOCTEXT("ECHO7", "AIMovementTestMovementDetected", "PHÁT HIỆN CHUYỂN ĐỘNG");
	const FText TestFailedMessage = NSLOCTEXT("ECHO7", "AIMovementTestFailed", "THỬ NGHIỆM THẤT BẠI");
	const FText TestCompletedMessage = NSLOCTEXT("ECHO7", "AIMovementTestCompleted", "THỬ NGHIỆM HOÀN TẤT");

	bool IsSafeLightComponent(const ULightComponent* LightComponent)
	{
		return IsValid(LightComponent)
			&& !LightComponent->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed);
	}
}

AAIMovementTestController::AAIMovementTestController()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	StartZone = CreateDefaultSubobject<UBoxComponent>(TEXT("Start Zone"));
	StartZone->SetupAttachment(SceneRoot);
	StartZone->SetBoxExtent(FVector(150.0f));
	StartZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StartZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	StartZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	StartZone->SetGenerateOverlapEvents(true);
	StartZone->SetHiddenInGame(true);
	StartZone->OnComponentBeginOverlap.AddDynamic(this, &AAIMovementTestController::HandleStartZoneBeginOverlap);

	FinishZone = CreateDefaultSubobject<UBoxComponent>(TEXT("Finish Zone"));
	FinishZone->SetupAttachment(SceneRoot);
	FinishZone->SetRelativeLocation(FVector(1000.0f, 0.0f, 0.0f));
	FinishZone->SetBoxExtent(FVector(150.0f));
	FinishZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FinishZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	FinishZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FinishZone->SetGenerateOverlapEvents(true);
	FinishZone->SetHiddenInGame(true);
	FinishZone->OnComponentBeginOverlap.AddDynamic(this, &AAIMovementTestController::HandleFinishZoneBeginOverlap);

	ObjectiveWhileTesting = NSLOCTEXT("ECHO7", "AIMovementTestObjectiveWhileTesting", "Tìm khu vực phát sinh lỗi.");
	ObjectiveAfterCompletion = NSLOCTEXT("ECHO7", "AIMovementTestObjectiveAfterCompletion", "Chờ hệ thống khởi động...");
	ObjectiveAfterScanCorridor = NSLOCTEXT("ECHO7", "AIMovementTestObjectiveAfterScanCorridor", "Khôi phục dữ liệu hệ thống.");
	TutorialLines = {
		NSLOCTEXT("ECHO7", "AIMovementTutorialFault", "PHÁT HIỆN BẤT THƯỜNG TRONG HỆ THỐNG QUÉT CHUYỂN ĐỘNG"),
		NSLOCTEXT("ECHO7", "AIMovementTutorialFragmented", "MỘT KHU VỰC DỮ LIỆU ĐANG BỊ PHÂN MẢNH"),
		NSLOCTEXT("ECHO7", "AIMovementTutorialMove", "DI CHUYỂN KHI HỆ THỐNG CHO PHÉP"),
		NSLOCTEXT("ECHO7", "AIMovementTutorialScan", "KHI ĐANG QUÉT, HÃY DỪNG LẠI"),
		NSLOCTEXT("ECHO7", "AIMovementTutorialFinish", "VƯỢT QUA KHU VỰC QUÉT VÀ TÌM NƠI PHÁT SINH LỖI")
	};
}

void AAIMovementTestController::StartTest(APawn* PlayerPawn)
{
	if (!IsValid(PlayerPawn)
		|| !Cast<AECHO7Character>(PlayerPawn)
		|| !PlayerPawn->IsPlayerControlled()
		|| CurrentState != EAIMovementTestState::Idle
		|| bTutorialActive
		|| bScanCorridorCleared)
	{
		UE_LOG(LogECHO7, Verbose, TEXT("ECHO7_C3 StartTest ignored. Player=%s State=%d Tutorial=%d CorridorCleared=%d"),
			*GetNameSafe(PlayerPawn), static_cast<int32>(CurrentState), bTutorialActive, bScanCorridorCleared);
		return;
	}

	ActivePlayer = PlayerPawn;
	if (!bHasCachedStartTransform)
	{
		CachedStartTransform = PlayerPawn->GetActorTransform();
		bHasCachedStartTransform = true;
	}

	CurrentState = EAIMovementTestState::Starting;
	UpdatePhaseLights();
	ResetScanLightsImmediately();
	if (ChallengeProgressManager && !ObjectiveWhileTesting.IsEmpty())
	{
		ChallengeProgressManager->SetObjective(ObjectiveWhileTesting);
	}

	ShowMessage(AIMovementTest::TestStartedMessage, FMath::Max(1.0f, InitialDelay));
	OnTestStarted.Broadcast();

	const float SafeInitialDelay = FMath::Max(0.0f, InitialDelay);
	if (SafeInitialDelay <= 0.0f)
	{
		BeginMovePhase();
		return;
	}

	GetWorldTimerManager().SetTimer(InitialDelayTimer, this, &AAIMovementTestController::BeginMovePhase, SafeInitialDelay, false);
}

void AAIMovementTestController::FailTest()
{
	if (bScanCorridorCleared || CurrentState != EAIMovementTestState::Scanning || !IsValid(ActivePlayer))
	{
		return;
	}

	ClearChallengeTimers();
	CurrentState = EAIMovementTestState::Failed;
	UpdatePhaseLights();
	ResetScanLightsImmediately();
	ShowMessage(AIMovementTest::MovementDetectedMessage, 0.6f, true);
	PlayOptionalSound(FailureSound.Get());
	OnTestFailed.Broadcast();

	const float SafeFailureRestartDelay = FMath::Max(0.0f, FailureRestartDelay);
	constexpr float FailureMessageDelay = 0.65f;
	if (SafeFailureRestartDelay > FailureMessageDelay)
	{
		GetWorldTimerManager().SetTimer(FailureMessageTimer, this, &AAIMovementTestController::ShowFailureMessage, FailureMessageDelay, false);
	}
	else
	{
		ShowFailureMessage();
	}

	if (SafeFailureRestartDelay <= 0.0f)
	{
		RestartAfterFailure();
		return;
	}

	GetWorldTimerManager().SetTimer(FailureRestartTimer, this, &AAIMovementTestController::RestartAfterFailure, SafeFailureRestartDelay, false);
}

void AAIMovementTestController::BeginPlay()
{
	Super::BeginPlay();

	// Rebind on the live level instance. This avoids relying solely on a
	// constructor-time dynamic binding after native-class reinstancing.
	if (StartZone)
	{
		StartZone->OnComponentBeginOverlap.RemoveDynamic(this, &AAIMovementTestController::HandleStartZoneBeginOverlap);
		StartZone->OnComponentBeginOverlap.AddDynamic(this, &AAIMovementTestController::HandleStartZoneBeginOverlap);
	}
	if (FinishZone)
	{
		FinishZone->OnComponentBeginOverlap.RemoveDynamic(this, &AAIMovementTestController::HandleFinishZoneBeginOverlap);
		FinishZone->OnComponentBeginOverlap.AddDynamic(this, &AAIMovementTestController::HandleFinishZoneBeginOverlap);
	}

	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 BeginPlay: StartZone=%s FinishZone=%s overlap handlers bound."),
		*GetNameSafe(StartZone), *GetNameSafe(FinishZone));
	CacheDefaultRoomLights();
	CacheWarningLightComponents();
	StopWarningLights();
	UpdatePhaseLights();
	RestoreDefaultRoomLights();
}

void AAIMovementTestController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(StartZone))
	{
		StartZone->OnComponentBeginOverlap.RemoveDynamic(this, &AAIMovementTestController::HandleStartZoneBeginOverlap);
	}
	if (IsValid(FinishZone))
	{
		FinishZone->OnComponentBeginOverlap.RemoveDynamic(this, &AAIMovementTestController::HandleFinishZoneBeginOverlap);
	}

	ClearChallengeTimers();
	ClearTutorialTimers();
	ResetScanLightsImmediately();
	RemoveTutorialOverlay();
	UnlockTutorialInput();
	TutorialOverlayWidget = nullptr;
	ActivePlayer = nullptr;

	// Release runtime-only weak caches while this class instance still has a
	// known-valid native layout. Never delete referenced actors/components.
	CachedWarningLightComponents.Empty();
	CachedDefaultRoomLightStates.Empty();
	bWarningLightComponentsCached = false;
	bDefaultRoomLightsCached = false;

	OnTestStarted.Clear();
	OnMovePhaseStarted.Clear();
	OnScanPhaseStarted.Clear();
	OnTestFailed.Clear();
	OnTestCompleted.Clear();
	OnScanCorridorCompleted.Clear();
	Super::EndPlay(EndPlayReason);
}

void AAIMovementTestController::HandleStartZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* OverlappingPlayer = GetPlayerPawnFromOverlap(OtherActor);
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 StartZone overlap: Actor=%s Player=%s State=%d CorridorCleared=%d"),
		*GetNameSafe(OtherActor), *GetNameSafe(OverlappingPlayer), static_cast<int32>(CurrentState), bScanCorridorCleared);
	if (bScanCorridorCleared)
	{
		return;
	}
	BeginTutorialOrStartTest(OverlappingPlayer);
}

void AAIMovementTestController::HandleFinishZoneBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* OverlappingPlayer = GetPlayerPawnFromOverlap(OtherActor);
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 FinishZone overlap: Actor=%s Player=%s ActivePlayer=%s State=%d CorridorCleared=%d"),
		*GetNameSafe(OtherActor), *GetNameSafe(OverlappingPlayer), *GetNameSafe(ActivePlayer), static_cast<int32>(CurrentState), bScanCorridorCleared);
	if (!IsValid(OverlappingPlayer))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_C3 FinishZone ignored: overlap actor is not the controlled ECHO7 player."));
		return;
	}
	if (OverlappingPlayer != ActivePlayer)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_C3 FinishZone ignored: overlapping player is not this test's active player."));
		return;
	}
	if (bScanCorridorCleared)
	{
		UE_LOG(LogECHO7, Verbose, TEXT("ECHO7_C3 FinishZone ignored: movement scan corridor was already completed."));
		return;
	}
	if (!IsTestActive())
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_C3 FinishZone ignored: test is not active (State=%d)."), static_cast<int32>(CurrentState));
		return;
	}

	if (bCompleteChallengeOnFinishZone)
	{
		CompleteTest();
	}
	else
	{
		CompleteScanCorridor();
	}
}

void AAIMovementTestController::BeginMovePhase()
{
	if (bScanCorridorCleared
		|| (CurrentState != EAIMovementTestState::Starting && CurrentState != EAIMovementTestState::Scanning))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(InitialDelayTimer);
	GetWorldTimerManager().ClearTimer(ScanPhaseTimer);
	GetWorldTimerManager().ClearTimer(ScanGraceTimer);
	GetWorldTimerManager().ClearTimer(MovementDetectionTimer);
	bMovementDetectionActive = false;

	CurrentState = EAIMovementTestState::Move;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 Entering Move state."));
	UpdatePhaseLights();
	ResetScanLightsImmediately();
	ShowMessage(AIMovementTest::MovePhaseMessage, 1.5f);
	PlayOptionalSound(MoveSound.Get());
	OnMovePhaseStarted.Broadcast();

	const float MoveDuration = GetRandomDuration(MinMoveDuration, MaxMoveDuration);
	if (MoveDuration <= 0.0f)
	{
		BeginScanningPhase();
		return;
	}

	GetWorldTimerManager().SetTimer(MovePhaseTimer, this, &AAIMovementTestController::BeginScanningPhase, MoveDuration, false);
}

void AAIMovementTestController::BeginScanningPhase()
{
	if (bScanCorridorCleared || CurrentState != EAIMovementTestState::Move || !IsValid(ActivePlayer))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MovePhaseTimer);
	CurrentState = EAIMovementTestState::Scanning;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 Entering Scanning state."));
	bMovementDetectionActive = false;
	ScanReferenceLocation = ActivePlayer->GetActorLocation();
	UpdatePhaseLights();
	DimDefaultRoomLightsForScan();
	StartWarningLights();
	ShowMessageLines({
		NSLOCTEXT("ECHO7", "AIMovementTestScanningHeader", "ĐANG QUÉT"),
		NSLOCTEXT("ECHO7", "AIMovementTestScanningInstruction", "KHÔNG ĐƯỢC DI CHUYỂN")
	}, 2.0f, true);
	PlayOptionalSound(ScanSound.Get());
	OnScanPhaseStarted.Broadcast();

	const float ScanDuration = GetRandomDuration(MinScanDuration, MaxScanDuration);
	if (ScanDuration <= 0.0f)
	{
		BeginMovePhase();
		return;
	}

	GetWorldTimerManager().SetTimer(ScanPhaseTimer, this, &AAIMovementTestController::BeginMovePhase, ScanDuration, false);

	const float SafeScanGracePeriod = FMath::Max(0.0f, ScanGracePeriod);
	if (SafeScanGracePeriod <= 0.0f)
	{
		BeginMovementDetection();
		return;
	}

	GetWorldTimerManager().SetTimer(ScanGraceTimer, this, &AAIMovementTestController::BeginMovementDetection, SafeScanGracePeriod, false);
}

void AAIMovementTestController::BeginTutorialOrStartTest(APawn* PlayerPawn)
{
	if (!IsValid(PlayerPawn) || CurrentState != EAIMovementTestState::Idle || bTutorialActive || bScanCorridorCleared)
	{
		return;
	}
	if (bTutorialCompleted || TutorialLines.IsEmpty())
	{
		bTutorialCompleted = true;
		StartTest(PlayerPawn);
		return;
	}

	ActivePlayer = PlayerPawn;
	bTutorialActive = true;
	NextTutorialLineIndex = 0;
	VisibleTutorialLines.Reset();
	TutorialTypewriter.Reset();
	LockTutorialInput();
	if (PrepareTutorialOverlay())
	{
		TutorialOverlayWidget->ClearContent();
		TutorialOverlayWidget->PlayGlitch(TutorialGlitchDuration);
	}
	DisplayNextTutorialLine();
}

void AAIMovementTestController::DisplayNextTutorialLine()
{
	if (!bTutorialActive || !TutorialLines.IsValidIndex(NextTutorialLineIndex))
	{
		return;
	}

	TutorialTypewriter.Begin(TutorialLines[NextTutorialLineIndex]);
	++NextTutorialLineIndex;
	RefreshTutorialDisplay();
	PlayOptionalSound(TutorialLineSound.Get());

	if (TutorialTypewriter.IsEmpty())
	{
		CompleteCurrentTutorialLine();
		return;
	}

	TutorialLineStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	GetWorldTimerManager().SetTimer(
		TutorialTypewriterTimer,
		this,
		&AAIMovementTestController::UpdateTutorialTypewriter,
		AIMovementTest::TutorialTypewriterUpdateInterval,
		true);
}

void AAIMovementTestController::UpdateTutorialTypewriter()
{
	if (!bTutorialActive || TutorialTypewriter.IsEmpty() || !GetWorld())
	{
		return;
	}

	int32 PreviousVisibleCount = 0;
	int32 NewVisibleCount = 0;
	if (TutorialTypewriter.Advance(
		GetWorld()->GetTimeSeconds() - TutorialLineStartTime,
		TutorialCharactersPerSecond,
		PreviousVisibleCount,
		NewVisibleCount))
	{
		RefreshTutorialDisplay();
		if (TutorialTypewriter.ShouldPlayTypingSoundForRange(PreviousVisibleCount, NewVisibleCount, AIMovementTest::TutorialTypingSoundEveryNCharacters))
		{
			PlayOptionalSound(TutorialTypeSound.Get());
		}
	}

	if (TutorialTypewriter.IsComplete())
	{
		CompleteCurrentTutorialLine();
	}
}

void AAIMovementTestController::CompleteCurrentTutorialLine()
{
	if (!bTutorialActive || !TutorialLines.IsValidIndex(NextTutorialLineIndex - 1))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TutorialTypewriterTimer);
	VisibleTutorialLines.Add(TutorialLines[NextTutorialLineIndex - 1]);
	TutorialTypewriter.Reset();
	RefreshTutorialDisplay();

	if (TutorialLines.IsValidIndex(NextTutorialLineIndex))
	{
		const float SafePause = FMath::Max(0.0f, TutorialLinePause);
		if (SafePause <= 0.0f)
		{
			DisplayNextTutorialLine();
		}
		else
		{
			GetWorldTimerManager().SetTimer(TutorialLinePauseTimer, this, &AAIMovementTestController::DisplayNextTutorialLine, SafePause, false);
		}
		return;
	}

	const float SafeEndDelay = FMath::Max(0.0f, TutorialEndDelay);
	if (SafeEndDelay <= 0.0f)
	{
		FinishTutorial();
	}
	else
	{
		GetWorldTimerManager().SetTimer(TutorialEndTimer, this, &AAIMovementTestController::FinishTutorial, SafeEndDelay, false);
	}
}

void AAIMovementTestController::RefreshTutorialDisplay()
{
	TArray<FText> DisplayLines = VisibleTutorialLines;
	if (bTutorialActive && !TutorialTypewriter.IsEmpty())
	{
		DisplayLines.Add(TutorialTypewriter.GetVisibleText());
	}
	if (TutorialOverlayWidget)
	{
		TutorialOverlayWidget->SetStoryLines(DisplayLines, TutorialFontSize);
	}
	else if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowTutorialLines(
			DisplayLines,
			FMath::Max(0.1f, FMath::Max(0.0f, TutorialEndDelay) + FMath::Max(0.0f, TutorialLinePause) + 0.5f),
			TutorialFontSize);
	}
}

void AAIMovementTestController::FinishTutorial()
{
	if (!bTutorialActive || !IsValid(ActivePlayer))
	{
		return;
	}

	ClearTutorialTimers();
	bTutorialActive = false;
	bTutorialCompleted = true;
	if (ChallengeProgressManager && !TutorialOverlayWidget)
	{
		ChallengeProgressManager->ClearStoryMessage();
	}
	if (TutorialOverlayWidget)
	{
		TutorialOverlayWidget->ClearContent();
		TutorialOverlayWidget->PlayGlitch(TutorialGlitchDuration);
	}
	GetWorldTimerManager().SetTimer(
		TutorialTransitionTimer,
		this,
		&AAIMovementTestController::BeginCountdown,
		FMath::Max(0.01f, TutorialGlitchDuration),
		false);
}

void AAIMovementTestController::BeginCountdown()
{
	CountdownValue = 3;
	AdvanceCountdown();
}

void AAIMovementTestController::AdvanceCountdown()
{
	if (!bTutorialCompleted || !IsValid(ActivePlayer))
	{
		return;
	}
	if (CountdownValue <= 0)
	{
		StartTestAfterCountdown();
		return;
	}
	if (TutorialOverlayWidget)
	{
		TutorialOverlayWidget->ShowCountdown(CountdownValue, CountdownFontSize);
	}
	--CountdownValue;
	GetWorldTimerManager().SetTimer(
		CountdownTimer,
		this,
		&AAIMovementTestController::AdvanceCountdown,
		FMath::Max(0.05f, CountdownStepDuration),
		false);
}

void AAIMovementTestController::StartTestAfterCountdown()
{
	RemoveTutorialOverlay();
	UnlockTutorialInput();
	StartTest(ActivePlayer);
}

void AAIMovementTestController::ClearTutorialTimers()
{
	GetWorldTimerManager().ClearTimer(TutorialTypewriterTimer);
	GetWorldTimerManager().ClearTimer(TutorialLinePauseTimer);
	GetWorldTimerManager().ClearTimer(TutorialEndTimer);
	GetWorldTimerManager().ClearTimer(TutorialTransitionTimer);
	GetWorldTimerManager().ClearTimer(CountdownTimer);
}

bool AAIMovementTestController::PrepareTutorialOverlay()
{
	APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer ? ActivePlayer->GetController() : nullptr);
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return false;
	}
	if (!TutorialOverlayWidget)
	{
		TutorialOverlayWidget = CreateWidget<UECHO7StoryOverlayWidget>(PlayerController, UECHO7StoryOverlayWidget::StaticClass());
	}
	return TutorialOverlayWidget
		&& (TutorialOverlayWidget->IsInViewport() || TutorialOverlayWidget->AddToPlayerScreen(80));
}

void AAIMovementTestController::RemoveTutorialOverlay()
{
	if (TutorialOverlayWidget)
	{
		TutorialOverlayWidget->RemoveFromParent();
	}
}

void AAIMovementTestController::LockTutorialInput()
{
	if (bTutorialInputLocked)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer ? ActivePlayer->GetController() : nullptr);
	if (!IsValid(PlayerController))
	{
		return;
	}
	TutorialPlayerController = PlayerController;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	bTutorialInputLocked = true;
}

void AAIMovementTestController::UnlockTutorialInput()
{
	if (bTutorialInputLocked && TutorialPlayerController.IsValid())
	{
		TutorialPlayerController->SetIgnoreMoveInput(false);
		TutorialPlayerController->SetIgnoreLookInput(false);
	}
	TutorialPlayerController.Reset();
	bTutorialInputLocked = false;
}

void AAIMovementTestController::BeginMovementDetection()
{
	if (bScanCorridorCleared || CurrentState != EAIMovementTestState::Scanning)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ScanGraceTimer);
	if (!IsValid(ActivePlayer))
	{
		return;
	}

	// Grace-period movement is intentionally ignored; strict detection starts from this fresh position.
	ScanReferenceLocation = ActivePlayer->GetActorLocation();
	bMovementDetectionActive = true;

	const float SafeCheckInterval = FMath::Clamp(MovementCheckInterval, 0.05f, 0.10f);
	GetWorldTimerManager().SetTimer(MovementDetectionTimer, this, &AAIMovementTestController::CheckPlayerMovement, SafeCheckInterval, true);
}

void AAIMovementTestController::CheckPlayerMovement()
{
	if (!bScanCorridorCleared
		&& CurrentState == EAIMovementTestState::Scanning
		&& bMovementDetectionActive
		&& HasPlayerMovedBeyondTolerance())
	{
		FailTest();
	}
}

void AAIMovementTestController::ShowFailureMessage()
{
	if (CurrentState == EAIMovementTestState::Failed)
	{
		ShowMessage(AIMovementTest::TestFailedMessage, FMath::Max(0.5f, FailureRestartDelay), true);
	}
}

void AAIMovementTestController::RestartAfterFailure()
{
	if (bScanCorridorCleared || CurrentState != EAIMovementTestState::Failed || !IsValid(ActivePlayer))
	{
		return;
	}

	const FTransform RestartTransform = IsValid(RespawnPoint) ? RespawnPoint->GetActorTransform() : CachedStartTransform;
	if (bHasCachedStartTransform || IsValid(RespawnPoint))
	{
		ActivePlayer->TeleportTo(RestartTransform.GetLocation(), RestartTransform.Rotator(), false, true);
	}
	StopPlayerMovement();

	CurrentState = EAIMovementTestState::Idle;
	UpdatePhaseLights();
	StartTest(ActivePlayer);
}

void AAIMovementTestController::CompleteTest()
{
	if (!IsTestActive())
	{
		return;
	}

	ClearChallengeTimers();
	CurrentState = EAIMovementTestState::Completed;
	UpdatePhaseLights();
	ResetScanLightsImmediately();
	PlayOptionalSound(CompletionSound.Get());
	OnTestCompleted.Broadcast();

	if (!bChallengeCompletionAttempted)
	{
		bChallengeCompletionAttempted = true;
		if (ChallengeProgressManager)
		{
			const bool bChallengeCompleted = ChallengeProgressManager->CompleteChallenge(ChallengeIndex);
			if (bChallengeCompleted && !ObjectiveAfterCompletion.IsEmpty())
			{
				ChallengeProgressManager->SetObjective(ObjectiveAfterCompletion);
			}
		}
		else
		{
			UE_LOG(LogECHO7, Warning, TEXT("AIMovementTestController completed without a ChallengeProgressManager."));
		}
	}

	ShowMessage(AIMovementTest::TestCompletedMessage, FMath::Max(0.0f, CompletionMessageDuration));
}

void AAIMovementTestController::CompleteScanCorridor()
{
	if (!IsTestActive() || bScanCorridorCleared)
	{
		return;
	}

	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 Finishing movement scan section. Clearing scan, failure, and tutorial timers."));
	bScanCorridorCleared = true;
	ClearChallengeTimers();
	ClearTutorialTimers();
	RemoveTutorialOverlay();
	// FinishZone ends the challenge's dangerous corridor. It must never retain
	// a tutorial-owned input lock, even if a transition was interrupted.
	UnlockTutorialInput();
	CurrentState = EAIMovementTestState::Completed;
	UpdatePhaseLights();
	ResetScanLightsImmediately();
	PlayOptionalSound(CompletionSound.Get());
	OnScanCorridorCompleted.Broadcast();

	if (ChallengeProgressManager)
	{
		// Clear the active scan warning before displaying the safe next objective.
		ChallengeProgressManager->ClearStoryMessage();
		if (!ObjectiveAfterScanCorridor.IsEmpty())
		{
			ChallengeProgressManager->SetObjective(ObjectiveAfterScanCorridor);
		}
	}
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 Movement scan section complete. FaultRepairTerminal can now interact when its controller reference is this actor."));
	ShowMessage(NSLOCTEXT("ECHO7", "AIMovementTestScanCorridorCleared", "KHU VỰC QUÉT ĐÃ ĐƯỢC VƯỢT QUA"), FMath::Max(0.0f, CompletionMessageDuration));
}

void AAIMovementTestController::ClearChallengeTimers()
{
	bMovementDetectionActive = false;
	GetWorldTimerManager().ClearTimer(InitialDelayTimer);
	GetWorldTimerManager().ClearTimer(MovePhaseTimer);
	GetWorldTimerManager().ClearTimer(ScanPhaseTimer);
	GetWorldTimerManager().ClearTimer(ScanGraceTimer);
	GetWorldTimerManager().ClearTimer(MovementDetectionTimer);
	GetWorldTimerManager().ClearTimer(FailureMessageTimer);
	GetWorldTimerManager().ClearTimer(FailureRestartTimer);
}

void AAIMovementTestController::ResetScanLightsImmediately()
{
	StopWarningLights();
	RestoreDefaultRoomLights();
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Immediate scan light reset."));
}

void AAIMovementTestController::StartWarningLights()
{
	StopWarningLights();
	if (!bEnableWarningLights || bScanCorridorCleared || CurrentState != EAIMovementTestState::Scanning)
	{
		return;
	}
	if (!bWarningLightComponentsCached)
	{
		CacheWarningLightComponents();
	}

	int32 ValidLightCount = 0;
	int32 NullLightCount = 0;
	for (const TWeakObjectPtr<ULightComponent>& CachedWarningLight : CachedWarningLightComponents)
	{
		ULightComponent* LightComponent = CachedWarningLight.Get();
		if (!AIMovementTest::IsSafeLightComponent(LightComponent))
		{
			++NullLightCount;
			continue;
		}

		LightComponent->SetLightColor(WarningLightColor, true);
		LightComponent->SetVisibility(true, true);
		LightComponent->MarkRenderStateDirty();
		++ValidLightCount;
	}

	if (ValidLightCount == 0)
	{
		UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Warning lights not started. ActorRefs=%d LegacyPointRefs=%d Components=0 NullComponents=%d"),
			WarningLightActors.Num(), WarningLights.Num(), NullLightCount);
		return;
	}

	bWarningLightsLit = true;
	SetWarningLightIntensity(FMath::Max(0.0f, WarningMaxIntensity));
	GetWorldTimerManager().SetTimer(
		WarningLightBlinkTimer,
		this,
		&AAIMovementTestController::HandleWarningLightBlink,
		FMath::Max(0.01f, WarningBlinkInterval),
		true);
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Warning lights started. ActorRefs=%d LegacyPointRefs=%d Components=%d NullComponents=%d Interval=%.2fs Intensity=%.1f"),
		WarningLightActors.Num(), WarningLights.Num(), ValidLightCount, NullLightCount,
		FMath::Max(0.01f, WarningBlinkInterval), FMath::Max(0.0f, WarningMaxIntensity));
}

void AAIMovementTestController::StopWarningLights()
{
	const bool bWasBlinking = bWarningLightsLit || GetWorldTimerManager().IsTimerActive(WarningLightBlinkTimer);
	GetWorldTimerManager().ClearTimer(WarningLightBlinkTimer);
	bWarningLightsLit = false;
	for (const TWeakObjectPtr<ULightComponent>& CachedWarningLight : CachedWarningLightComponents)
	{
		ULightComponent* LightComponent = CachedWarningLight.Get();
		if (AIMovementTest::IsSafeLightComponent(LightComponent))
		{
			LightComponent->SetIntensity(0.0f);
			LightComponent->SetVisibility(false, true);
			LightComponent->MarkRenderStateDirty();
		}
	}

	if (bWasBlinking)
	{
		UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Warning lights stopped. Components=%d"), CachedWarningLightComponents.Num());
	}
}

void AAIMovementTestController::HandleWarningLightBlink()
{
	if (!bEnableWarningLights || bScanCorridorCleared || CurrentState != EAIMovementTestState::Scanning)
	{
		StopWarningLights();
		return;
	}

	bWarningLightsLit = !bWarningLightsLit;
	SetWarningLightIntensity(bWarningLightsLit ? FMath::Max(0.0f, WarningMaxIntensity) : 0.0f);
}

void AAIMovementTestController::SetWarningLightIntensity(float Intensity) const
{
	for (const TWeakObjectPtr<ULightComponent>& CachedWarningLight : CachedWarningLightComponents)
	{
		ULightComponent* LightComponent = CachedWarningLight.Get();
		if (AIMovementTest::IsSafeLightComponent(LightComponent))
		{
			LightComponent->SetIntensity(Intensity);
		}
	}
}

void AAIMovementTestController::CacheWarningLightComponents()
{
	if (bWarningLightComponentsCached)
	{
		return;
	}

	CachedWarningLightComponents.Reset();
	TSet<ULightComponent*> UniqueLightComponents;
	int32 NullActorCount = 0;
	int32 ActorsWithoutLights = 0;
	int32 DuplicateComponentCount = 0;

	auto CacheActorLights = [this, &UniqueLightComponents, &NullActorCount, &ActorsWithoutLights, &DuplicateComponentCount](AActor* LightActor)
	{
		if (!IsValid(LightActor) || LightActor->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
		{
			++NullActorCount;
			return;
		}

		TInlineComponentArray<ULightComponent*> LightComponents;
		LightActor->GetComponents(LightComponents);
		if (LightComponents.IsEmpty())
		{
			++ActorsWithoutLights;
			return;
		}

		for (ULightComponent* LightComponent : LightComponents)
		{
			if (!AIMovementTest::IsSafeLightComponent(LightComponent))
			{
				continue;
			}
			if (UniqueLightComponents.Contains(LightComponent))
			{
				++DuplicateComponentCount;
				continue;
			}

			UniqueLightComponents.Add(LightComponent);
			CachedWarningLightComponents.Add(LightComponent);
		}
	};

	for (AActor* WarningLightActor : WarningLightActors)
	{
		CacheActorLights(WarningLightActor);
	}
	for (APointLight* LegacyPointLight : WarningLights)
	{
		CacheActorLights(LegacyPointLight);
	}

	bWarningLightComponentsCached = true;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Warning light components cached. ActorRefs=%d LegacyPointRefs=%d Components=%d NullActors=%d ActorsWithoutLights=%d Duplicates=%d"),
		WarningLightActors.Num(), WarningLights.Num(), CachedWarningLightComponents.Num(), NullActorCount, ActorsWithoutLights, DuplicateComponentCount);
}

void AAIMovementTestController::CacheDefaultRoomLights()
{
	if (bDefaultRoomLightsCached)
	{
		return;
	}

	CachedDefaultRoomLightStates.Reset();
	TSet<ULightComponent*> UniqueLightComponents;
	int32 NullActorCount = 0;
	int32 ActorsWithoutLights = 0;
	int32 DuplicateComponentCount = 0;

	auto CacheActorLights = [this, &UniqueLightComponents, &NullActorCount, &ActorsWithoutLights, &DuplicateComponentCount](AActor* LightActor)
	{
		if (!IsValid(LightActor) || LightActor->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
		{
			++NullActorCount;
			return;
		}

		TInlineComponentArray<ULightComponent*> LightComponents;
		LightActor->GetComponents(LightComponents);
		if (LightComponents.IsEmpty())
		{
			++ActorsWithoutLights;
			return;
		}

		for (ULightComponent* LightComponent : LightComponents)
		{
			if (!AIMovementTest::IsSafeLightComponent(LightComponent))
			{
				continue;
			}
			if (UniqueLightComponents.Contains(LightComponent))
			{
				++DuplicateComponentCount;
				continue;
			}

			UniqueLightComponents.Add(LightComponent);
			FECHO7DefaultRoomLightState& CachedState = CachedDefaultRoomLightStates.AddDefaulted_GetRef();
			CachedState.LightComponent = LightComponent;
			CachedState.Intensity = LightComponent->Intensity;
			CachedState.Color = LightComponent->GetLightColor();
			CachedState.bVisible = LightComponent->IsVisible();
		}
	};

	for (AActor* DefaultRoomLightActor : DefaultRoomLightActors)
	{
		CacheActorLights(DefaultRoomLightActor);
	}
	for (APointLight* LegacyPointLight : DefaultRoomLights)
	{
		CacheActorLights(LegacyPointLight);
	}

	bDefaultRoomLightsCached = true;
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Default room lights cached. ActorRefs=%d LegacyPointRefs=%d Components=%d NullActors=%d ActorsWithoutLights=%d Duplicates=%d"),
		DefaultRoomLightActors.Num(), DefaultRoomLights.Num(), CachedDefaultRoomLightStates.Num(), NullActorCount, ActorsWithoutLights, DuplicateComponentCount);
}

void AAIMovementTestController::DimDefaultRoomLightsForScan()
{
	if (bScanCorridorCleared || CurrentState != EAIMovementTestState::Scanning)
	{
		RestoreDefaultRoomLights();
		return;
	}
	if (!bDefaultRoomLightsCached)
	{
		CacheDefaultRoomLights();
	}

	const float SafeMultiplier = FMath::Clamp(DefaultLightScanMultiplier, 0.0f, 1.0f);
	int32 DimmedLightCount = 0;
	int32 NullLightCount = 0;
	for (const FECHO7DefaultRoomLightState& CachedState : CachedDefaultRoomLightStates)
	{
		ULightComponent* LightComponent = CachedState.LightComponent.Get();
		if (!AIMovementTest::IsSafeLightComponent(LightComponent))
		{
			++NullLightCount;
			continue;
		}

		LightComponent->SetVisibility(CachedState.bVisible, true);
		LightComponent->SetIntensity(CachedState.Intensity * SafeMultiplier);
		++DimmedLightCount;
	}

	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Default room lights dimmed for Scanning. Dimmed=%d Null=%d Multiplier=%.2f"),
		DimmedLightCount, NullLightCount, SafeMultiplier);
}

void AAIMovementTestController::RestoreDefaultRoomLights()
{
	if (!bDefaultRoomLightsCached)
	{
		return;
	}

	int32 RestoredLightCount = 0;
	int32 NullLightCount = 0;
	for (const FECHO7DefaultRoomLightState& CachedState : CachedDefaultRoomLightStates)
	{
		ULightComponent* LightComponent = CachedState.LightComponent.Get();
		if (!AIMovementTest::IsSafeLightComponent(LightComponent))
		{
			++NullLightCount;
			continue;
		}

		// Remove the dimmed render state before restoring the exact cached values,
		// then force the final state to the render thread in this frame.
		LightComponent->SetVisibility(false, true);
		LightComponent->SetLightColor(CachedState.Color, true);
		LightComponent->SetIntensity(CachedState.Intensity);
		LightComponent->SetVisibility(CachedState.bVisible, true);
		LightComponent->MarkRenderStateDirty();
		++RestoredLightCount;
	}

	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3_LIGHT Default room lights restored. Restored=%d Null=%d"),
		RestoredLightCount, NullLightCount);
}

void AAIMovementTestController::UpdatePhaseLights()
{
	const bool bMovePhase = CurrentState == EAIMovementTestState::Move;
	const bool bScanPhase = CurrentState == EAIMovementTestState::Scanning;

	// Disable both groups first so a light accidentally assigned to both arrays
	// receives the active phase's final, deterministic visibility.
	SetLightsVisible(MovePhaseLights, false);
	SetLightsVisible(ScanPhaseLights, false);

	if (bMovePhase)
	{
		SetLightsVisible(MovePhaseLights, true);
	}
	else if (bScanPhase)
	{
		SetLightsVisible(ScanPhaseLights, true);
	}
}

void AAIMovementTestController::SetLightsVisible(const TArray<TObjectPtr<ALight>>& Lights, bool bVisible) const
{
	for (ALight* Light : Lights)
	{
		if (!IsValid(Light))
		{
			continue;
		}

		if (ULightComponent* LightComponent = Light->GetLightComponent())
		{
			LightComponent->SetVisibility(bVisible, true);
		}
	}
}

void AAIMovementTestController::PlayOptionalSound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AAIMovementTestController::ShowMessage(const FText& Message, float Duration, bool bWarning) const
{
	if (!ChallengeProgressManager)
	{
		return;
	}

	if (bWarning)
	{
		ChallengeProgressManager->ShowWarning(Message, Duration);
	}
	else
	{
		ChallengeProgressManager->ShowStoryMessage(Message, Duration);
	}
}

void AAIMovementTestController::ShowMessageLines(const TArray<FText>& Lines, float Duration, bool bWarning) const
{
	if (!ChallengeProgressManager)
	{
		return;
	}

	if (bWarning)
	{
		ChallengeProgressManager->ShowWarningLines(Lines, Duration);
	}
	else
	{
		ChallengeProgressManager->ShowStoryMessageLines(Lines, Duration);
	}
}

void AAIMovementTestController::StopPlayerMovement() const
{
	if (const ACharacter* Character = Cast<ACharacter>(ActivePlayer))
	{
		if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
		{
			CharacterMovement->StopMovementImmediately();
		}
	}
}

APawn* AAIMovementTestController::GetPlayerPawnFromOverlap(AActor* OtherActor) const
{
	APawn* PlayerPawn = Cast<AECHO7Character>(OtherActor);
	return IsValid(PlayerPawn) && PlayerPawn->IsPlayerControlled() ? PlayerPawn : nullptr;
}

bool AAIMovementTestController::IsTestActive() const
{
	return CurrentState == EAIMovementTestState::Starting
		|| CurrentState == EAIMovementTestState::Move
		|| CurrentState == EAIMovementTestState::Scanning;
}

bool AAIMovementTestController::HasPlayerMovedBeyondTolerance() const
{
	if (!IsValid(ActivePlayer))
	{
		return false;
	}

	FVector HorizontalDisplacement = ActivePlayer->GetActorLocation() - ScanReferenceLocation;
	HorizontalDisplacement.Z = 0.0f;
	const float SafeMovementTolerance = FMath::Max(0.0f, MovementTolerance);
	return HorizontalDisplacement.SizeSquared() > FMath::Square(SafeMovementTolerance);
}

float AAIMovementTestController::GetRandomDuration(float MinimumDuration, float MaximumDuration) const
{
	const float Minimum = FMath::Max(0.0f, FMath::Min(MinimumDuration, MaximumDuration));
	const float Maximum = FMath::Max(Minimum, FMath::Max(MinimumDuration, MaximumDuration));
	return FMath::IsNearlyEqual(Minimum, Maximum) ? Minimum : FMath::FRandRange(Minimum, Maximum);
}
