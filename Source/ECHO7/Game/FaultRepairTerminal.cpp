// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/FaultRepairTerminal.h"

#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Game/AIMovementTestController.h"
#include "Game/ChallengeProgressManager.h"
#include "Game/EvacuationZone.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/ECHO7FaultPuzzleWidget.h"
#include "UI/ECHO7StoryOverlayWidget.h"

namespace FaultRepairTerminal
{
	constexpr float TypewriterUpdateInterval = 0.02f;
	const FText MissingPuzzleTexture = NSLOCTEXT("ECHO7", "FaultRepairMissingPuzzleTexture", "THIẾU DỮ LIỆU HÌNH ẢNH HỆ THỐNG");
}

AFaultRepairTerminal::AFaultRepairTerminal()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "FaultRepairInteractionPrompt", "Khôi phục dữ liệu hệ thống");
	ObjectiveAfterRepair = NSLOCTEXT("ECHO7", "FaultRepairObjectiveAfterRepair", "Quay lại Phòng Điều Khiển Trung Tâm.");
	PostRepairStoryLines = {
		NSLOCTEXT("ECHO7", "FaultRepairStoryRecovered", "DỮ LIỆU ĐÃ ĐƯỢC KHÔI PHỤC"),
		NSLOCTEXT("ECHO7", "FaultRepairStoryRepaired", "KHU VỰC LỖI ĐÃ ĐƯỢC SỬA CHỮA THÀNH CÔNG"),
		NSLOCTEXT("ECHO7", "FaultRepairStoryScanner", "HỆ THỐNG QUÉT CHUYỂN ĐỘNG ĐÃ NGỪNG HOẠT ĐỘNG"),
		NSLOCTEXT("ECHO7", "FaultRepairStoryStable", "TOÀN BỘ HỆ THỐNG CƠ SỞ ĐÃ ỔN ĐỊNH"),
		NSLOCTEXT("ECHO7", "FaultRepairStoryUnknown", "PHÁT HIỆN KẾT NỐI KHÔNG XÁC ĐỊNH"),
		NSLOCTEXT("ECHO7", "FaultRepairStorySignal", "NGUỒN TÍN HIỆU: PHÒNG ĐIỀU KHIỂN TRUNG TÂM"),
		NSLOCTEXT("ECHO7", "FaultRepairStoryReturn", "HÃY QUAY LẠI ĐỂ XÁC MINH")
	};
}

void AFaultRepairTerminal::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	if (!PuzzleSourceTexture)
	{
		ShowWarning(FaultRepairTerminal::MissingPuzzleTexture);
		return;
	}

	const AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	APlayerController* PlayerController = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal could not find a local PlayerController."));
		return;
	}

	BeginPuzzleOpenTransition(PlayerController);
}

bool AFaultRepairTerminal::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor)
		&& Cast<AECHO7Character>(Interactor) != nullptr
		&& !bSolved
		&& !bPuzzleOpen
		&& !bPostRepairStoryActive
		&& IsValid(ChallengeProgressManager)
		&& IsValid(MovementTestController)
		&& MovementTestController->HasClearedScanCorridor();
}

void AFaultRepairTerminal::BeginPlay()
{
	Super::BeginPlay();

	if (!MovementTestController)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_C3 FaultRepairTerminal has no MovementTestController reference and cannot be used."));
		return;
	}

	MovementTestController->OnScanCorridorCompleted.AddUniqueDynamic(this, &AFaultRepairTerminal::HandleScanCorridorCompleted);
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 FaultRepairTerminal bound to %s. ScanCorridorCleared=%d"),
		*GetNameSafe(MovementTestController), MovementTestController->HasClearedScanCorridor());
}

void AFaultRepairTerminal::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(MovementTestController))
	{
		MovementTestController->OnScanCorridorCompleted.RemoveDynamic(this, &AFaultRepairTerminal::HandleScanCorridorCompleted);
	}
	MovementTestController = nullptr;
	ClearTerminalTimers();
	ClosePuzzleWidget(false);
	if (StoryOverlayWidget)
	{
		StoryOverlayWidget->RemoveFromParent();
		StoryOverlayWidget = nullptr;
	}
	RestoreGameplayInput();
	Super::EndPlay(EndPlayReason);
}

void AFaultRepairTerminal::BeginPuzzleOpenTransition(APlayerController* PlayerController)
{
	if (bPuzzleOpen || ActivePuzzleWidget || !PrepareStoryOverlay(PlayerController))
	{
		return;
	}

	CapturePlayerInput(PlayerController, false);
	StoryOverlayWidget->ClearContent();
	StoryOverlayWidget->PlayGlitch(GlitchDuration);
	GetWorldTimerManager().SetTimer(
		PuzzleTransitionTimer,
		this,
		&AFaultRepairTerminal::OpenPuzzleAfterGlitch,
		FMath::Max(0.01f, GlitchDuration),
		false);
}

void AFaultRepairTerminal::OpenPuzzleAfterGlitch()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!IsValid(PlayerController) || ActivePuzzleWidget || bSolved)
	{
		RestoreGameplayInput();
		return;
	}

	if (StoryOverlayWidget)
	{
		StoryOverlayWidget->RemoveFromParent();
	}

	ActivePuzzleWidget = CreateWidget<UECHO7FaultPuzzleWidget>(PlayerController, UECHO7FaultPuzzleWidget::StaticClass());
	if (!ActivePuzzleWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal failed to create its puzzle widget."));
		RestoreGameplayInput();
		return;
	}

	ActivePuzzleWidget->OnPuzzleSolved.AddUniqueDynamic(this, &AFaultRepairTerminal::HandlePuzzleSolved);
	ActivePuzzleWidget->OnPuzzleCancelled.AddUniqueDynamic(this, &AFaultRepairTerminal::HandlePuzzleCancelled);
	ActivePuzzleWidget->SetTileGap(TileGap);
	ActivePuzzleWidget->InitializePuzzle(PuzzleSourceTexture, bShuffleOnOpen, bPreserveProgressWhenClosed ? SavedTileOrder : TArray<int32>());
	if (!ActivePuzzleWidget->AddToPlayerScreen(100))
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal failed to add its puzzle widget to the player screen."));
		ActivePuzzleWidget->OnPuzzleSolved.RemoveDynamic(this, &AFaultRepairTerminal::HandlePuzzleSolved);
		ActivePuzzleWidget->OnPuzzleCancelled.RemoveDynamic(this, &AFaultRepairTerminal::HandlePuzzleCancelled);
		ActivePuzzleWidget = nullptr;
		RestoreGameplayInput();
		return;
	}

	bPuzzleOpen = true;
	CapturePlayerInput(PlayerController, true);
	ActivePuzzleWidget->SetKeyboardFocus();
}

void AFaultRepairTerminal::BeginPuzzleCloseTransition()
{
	ClosePuzzleWidget(bPreserveProgressWhenClosed);
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PrepareStoryOverlay(PlayerController))
	{
		RestoreGameplayInput();
		return;
	}
	StoryOverlayWidget->ClearContent();
	StoryOverlayWidget->PlayGlitch(GlitchDuration);
	GetWorldTimerManager().SetTimer(
		PuzzleTransitionTimer,
		this,
		&AFaultRepairTerminal::FinishPuzzleCloseTransition,
		FMath::Max(0.01f, GlitchDuration),
		false);
}

void AFaultRepairTerminal::FinishPuzzleCloseTransition()
{
	if (StoryOverlayWidget)
	{
		StoryOverlayWidget->RemoveFromParent();
	}
	RestoreGameplayInput();
}

void AFaultRepairTerminal::ClosePuzzleWidget(bool bPreserveProgress)
{
	if (ActivePuzzleWidget)
	{
		if (bPreserveProgress)
		{
			SavedTileOrder = ActivePuzzleWidget->GetTileOrder();
		}
		else
		{
			SavedTileOrder.Reset();
		}
		ActivePuzzleWidget->OnPuzzleSolved.RemoveDynamic(this, &AFaultRepairTerminal::HandlePuzzleSolved);
		ActivePuzzleWidget->OnPuzzleCancelled.RemoveDynamic(this, &AFaultRepairTerminal::HandlePuzzleCancelled);
		ActivePuzzleWidget->RemoveFromParent();
		ActivePuzzleWidget = nullptr;
	}
	bPuzzleOpen = false;
}

void AFaultRepairTerminal::BeginPostRepairStory()
{
	bPostRepairStoryActive = true;
	VisiblePostRepairLines.Reset();
	PostRepairTypewriter.Reset();
	NextPostRepairLineIndex = 0;
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PrepareStoryOverlay(PlayerController))
	{
		FinishPostRepairStory();
		return;
	}

	CapturePlayerInput(PlayerController, false);
	StoryOverlayWidget->ClearContent();
	StoryOverlayWidget->PlayGlitch(GlitchDuration);
	GetWorldTimerManager().SetTimer(
		StoryTransitionTimer,
		this,
		&AFaultRepairTerminal::BeginNextPostRepairLine,
		FMath::Max(0.01f, GlitchDuration),
		false);
}

void AFaultRepairTerminal::BeginNextPostRepairLine()
{
	if (!bPostRepairStoryActive)
	{
		return;
	}
	if (!PostRepairStoryLines.IsValidIndex(NextPostRepairLineIndex))
	{
		const float SafeReadDuration = FMath::Max(0.0f, PostRepairReadDuration);
		if (SafeReadDuration <= 0.0f)
		{
			BeginReturnTransition();
		}
		else
		{
			GetWorldTimerManager().SetTimer(StoryReadTimer, this, &AFaultRepairTerminal::BeginReturnTransition, SafeReadDuration, false);
		}
		return;
	}

	PostRepairTypewriter.Begin(PostRepairStoryLines[NextPostRepairLineIndex]);
	++NextPostRepairLineIndex;
	RefreshPostRepairStory();
	if (PostRepairTypewriter.IsEmpty())
	{
		CompleteCurrentPostRepairLine();
		return;
	}

	PostRepairLineStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	GetWorldTimerManager().SetTimer(
		StoryTypewriterTimer,
		this,
		&AFaultRepairTerminal::UpdatePostRepairTypewriter,
		FaultRepairTerminal::TypewriterUpdateInterval,
		true);
}

void AFaultRepairTerminal::UpdatePostRepairTypewriter()
{
	if (!bPostRepairStoryActive || PostRepairTypewriter.IsEmpty() || !GetWorld())
	{
		return;
	}
	int32 PreviousVisibleCharacters = 0;
	int32 NewVisibleCharacters = 0;
	if (PostRepairTypewriter.Advance(
		GetWorld()->GetTimeSeconds() - PostRepairLineStartTime,
		PostRepairCharactersPerSecond,
		PreviousVisibleCharacters,
		NewVisibleCharacters))
	{
		RefreshPostRepairStory();
	}
	if (PostRepairTypewriter.IsComplete())
	{
		CompleteCurrentPostRepairLine();
	}
}

void AFaultRepairTerminal::CompleteCurrentPostRepairLine()
{
	if (!bPostRepairStoryActive || !PostRepairStoryLines.IsValidIndex(NextPostRepairLineIndex - 1))
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(StoryTypewriterTimer);
	VisiblePostRepairLines.Add(PostRepairStoryLines[NextPostRepairLineIndex - 1]);
	PostRepairTypewriter.Reset();
	RefreshPostRepairStory();

	const float SafePause = FMath::Max(0.0f, PostRepairLinePause);
	if (SafePause <= 0.0f)
	{
		BeginNextPostRepairLine();
	}
	else
	{
		GetWorldTimerManager().SetTimer(StoryLinePauseTimer, this, &AFaultRepairTerminal::BeginNextPostRepairLine, SafePause, false);
	}
}

void AFaultRepairTerminal::RefreshPostRepairStory()
{
	if (!StoryOverlayWidget)
	{
		return;
	}
	TArray<FText> DisplayLines = VisiblePostRepairLines;
	if (bPostRepairStoryActive && !PostRepairTypewriter.IsEmpty())
	{
		DisplayLines.Add(PostRepairTypewriter.GetVisibleText());
	}
	StoryOverlayWidget->SetStoryLines(DisplayLines, PostRepairStoryFontSize);
}

void AFaultRepairTerminal::BeginReturnTransition()
{
	if (!bPostRepairStoryActive)
	{
		return;
	}
	if (StoryOverlayWidget)
	{
		StoryOverlayWidget->PlayGlitch(GlitchDuration);
	}
	GetWorldTimerManager().SetTimer(
		StoryTransitionTimer,
		this,
		&AFaultRepairTerminal::FinishPostRepairStory,
		FMath::Max(0.01f, GlitchDuration),
		false);
}

void AFaultRepairTerminal::FinishPostRepairStory()
{
	ClearTerminalTimers();
	bPostRepairStoryActive = false;
	if (StoryOverlayWidget)
	{
		StoryOverlayWidget->RemoveFromParent();
	}
	RestoreGameplayInput();
	if (ChallengeProgressManager && !ObjectiveAfterRepair.IsEmpty())
	{
		ChallengeProgressManager->SetObjective(ObjectiveAfterRepair);
	}
}

bool AFaultRepairTerminal::PrepareStoryOverlay(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
	{
		return false;
	}
	if (!StoryOverlayWidget)
	{
		StoryOverlayWidget = CreateWidget<UECHO7StoryOverlayWidget>(PlayerController, UECHO7StoryOverlayWidget::StaticClass());
	}
	if (!StoryOverlayWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal failed to create its story overlay."));
		return false;
	}
	if (!StoryOverlayWidget->IsInViewport() && !StoryOverlayWidget->AddToPlayerScreen(90))
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal failed to add its story overlay to the player screen."));
		StoryOverlayWidget = nullptr;
		return false;
	}
	return true;
}

void AFaultRepairTerminal::CapturePlayerInput(APlayerController* PlayerController, bool bForPuzzle)
{
	if (!IsValid(PlayerController))
	{
		return;
	}

	// SetIgnore* calls stack in Unreal. The opening transition, puzzle UI, and
	// post-repair story share one terminal-owned lock, rather than adding a new
	// stack entry at every transition.
	if (bInputCaptured && ActivePlayerController != PlayerController)
	{
		RestoreGameplayInput();
	}

	ActivePlayerController = PlayerController;
	if (!bInputCaptured)
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		bInputCaptured = true;
	}

	if (bForPuzzle)
	{
		PlayerController->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void AFaultRepairTerminal::RestoreGameplayInput()
{
	if (!bInputCaptured)
	{
		return;
	}

	if (IsValid(ActivePlayerController))
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

APlayerController* AFaultRepairTerminal::GetLocalPlayerController() const
{
	if (IsValid(ActivePlayerController) && ActivePlayerController->IsLocalController())
	{
		return ActivePlayerController;
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	return IsValid(PlayerController) && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

void AFaultRepairTerminal::ClearTerminalTimers()
{
	GetWorldTimerManager().ClearTimer(PuzzleTransitionTimer);
	GetWorldTimerManager().ClearTimer(StoryTransitionTimer);
	GetWorldTimerManager().ClearTimer(StoryTypewriterTimer);
	GetWorldTimerManager().ClearTimer(StoryLinePauseTimer);
	GetWorldTimerManager().ClearTimer(StoryReadTimer);
}

void AFaultRepairTerminal::ShowWarning(const FText& Message) const
{
	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowWarning(Message, 3.0f);
	}
}

void AFaultRepairTerminal::HandlePuzzleSolved()
{
	if (!bPuzzleOpen || bSolved)
	{
		return;
	}
	if (ChallengeProgressManager)
	{
		// The run ends at puzzle success, before challenge registration or the
		// post-repair presentation can add time to the final result.
		ChallengeProgressManager->StopRunTimer();
	}
	ClosePuzzleWidget(false);
	bSolved = true;
	if (!ChallengeProgressManager || !ChallengeProgressManager->CompleteChallenge(3))
	{
		UE_LOG(LogECHO7, Warning, TEXT("FaultRepairTerminal could not complete Challenge 3. Verify ChallengeProgressManager setup."));
	}
	BeginPostRepairStory();
}

void AFaultRepairTerminal::HandlePuzzleCancelled()
{
	if (!bPuzzleOpen || bSolved)
	{
		return;
	}
	BeginPuzzleCloseTransition();
}

void AFaultRepairTerminal::HandleScanCorridorCompleted()
{
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_C3 FaultRepairTerminal enabled: its movement scan corridor requirement is now satisfied."));
}
