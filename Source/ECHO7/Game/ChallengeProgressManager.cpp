// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/ChallengeProgressManager.h"

#include "Components/SceneComponent.h"
#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Interaction/InteractableInterface.h"
#include "Interaction/InteractionComponent.h"
#include "Power/MainReactor.h"
#include "TimerManager.h"
#include "UI/ECHO7HUDWidget.h"

namespace ECHO7RunTimer
{
	constexpr float HUDRefreshInterval = 0.10f;
}

AChallengeProgressManager::AChallengeProgressManager()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	CurrentObjective = NSLOCTEXT("ECHO7", "InitialObjective", "Khôi phục hệ thống của cơ sở.");
	StartingStoryMessages = {
		NSLOCTEXT("ECHO7", "IntroFacility", "CƠ SỞ NGHIÊN CỨU ECHO-7"),
		NSLOCTEXT("ECHO7", "IntroEmergency", "GIAO THỨC KHẨN CẤP ĐÃ KÍCH HOẠT"),
		NSLOCTEXT("ECHO7", "IntroOffline", "CÁC HỆ THỐNG CHÍNH ĐANG NGOẠI TUYẾN")
	};
}

void AChallengeProgressManager::BeginPlay()
{
	Super::BeginPlay();
	StartRunTimer();

	if (MainReactor)
	{
		CurrentCompletedChallengeCount = MainReactor->GetInstalledCoreCount();
	}

	CreateHUD();
	BindInteractionPrompt();
	UpdateHUD();
	DisplayNextStartingMessage();
}

void AChallengeProgressManager::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartingStoryTimer);
	GetWorldTimerManager().ClearTimer(RunTimerRefreshTimer);
	if (BoundInteractionComponent)
	{
		BoundInteractionComponent->OnFocusedInteractableChanged.RemoveDynamic(this, &AChallengeProgressManager::HandleFocusedInteractableChanged);
		BoundInteractionComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AChallengeProgressManager::StartRunTimer()
{
	if (bRunTimerRunning || bRunTimerCompleted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogECHO7, Warning, TEXT("ECHO7_TIMER could not start because no world is available."));
		return;
	}

	RunStartTime = World->GetRealTimeSeconds();
	FinalCompletionTime = 0.0;
	bRunTimerRunning = true;
	World->GetTimerManager().SetTimer(
		RunTimerRefreshTimer,
		this,
		&AChallengeProgressManager::RefreshRunTimerHUD,
		ECHO7RunTimer::HUDRefreshInterval,
		true);
	RefreshRunTimerHUD();
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_TIMER run timer started."));
}

void AChallengeProgressManager::StopRunTimer()
{
	if (!bRunTimerRunning || bRunTimerCompleted)
	{
		return;
	}

	FinalCompletionTime = GetElapsedRunTime();
	bRunTimerRunning = false;
	bRunTimerCompleted = true;
	GetWorldTimerManager().ClearTimer(RunTimerRefreshTimer);
	RefreshRunTimerHUD();
	UE_LOG(LogECHO7, Log, TEXT("ECHO7_TIMER final timer stopped at %s."), *GetFormattedRunTime().ToString());
}

double AChallengeProgressManager::GetElapsedRunTime() const
{
	if (bRunTimerCompleted)
	{
		return FMath::Max(0.0, FinalCompletionTime);
	}

	const UWorld* World = GetWorld();
	if (!bRunTimerRunning || !World)
	{
		return 0.0;
	}

	return FMath::Max(0.0, static_cast<double>(World->GetRealTimeSeconds()) - RunStartTime);
}

FText AChallengeProgressManager::GetFormattedRunTime() const
{
	const int64 TotalSeconds = FMath::Max<int64>(0, FMath::FloorToInt64(GetElapsedRunTime()));
	const int64 Hours = TotalSeconds / 3600;
	const int64 Minutes = (TotalSeconds % 3600) / 60;
	const int64 Seconds = TotalSeconds % 60;

	if (Hours > 0)
	{
		return FText::FromString(FString::Printf(
			TEXT("%02lld:%02lld:%02lld"),
			static_cast<long long>(Hours),
			static_cast<long long>(Minutes),
			static_cast<long long>(Seconds)));
	}

	return FText::FromString(FString::Printf(
		TEXT("%02lld:%02lld"),
		static_cast<long long>(TotalSeconds / 60),
		static_cast<long long>(Seconds)));
}

bool AChallengeProgressManager::CompleteChallenge(int32 ChallengeIndex)
{
	if (ChallengeIndex < 1 || ChallengeIndex > TotalChallengeCount || CompletedChallengeIndices.Contains(ChallengeIndex))
	{
		return false;
	}

	if (!MainReactor)
	{
		if (!bMissingReactorWarningLogged)
		{
			UE_LOG(LogECHO7, Warning, TEXT("ChallengeProgressManager has no MainReactor assigned."));
			bMissingReactorWarningLogged = true;
		}
		return false;
	}

	if (!MainReactor->RegisterInstalledCore())
	{
		return false;
	}

	CompletedChallengeIndices.Add(ChallengeIndex);
	CurrentCompletedChallengeCount = MainReactor->GetInstalledCoreCount();
	const int32 TotalProgress = MainReactor->GetRequiredCoreCount();
	UpdateHUD();
	ShowStoryMessageLines({
		NSLOCTEXT("ECHO7", "ChallengeCompletedHeader", "KHU VỰC ĐÃ ĐƯỢC KHÔI PHỤC"),
		FText::Format(
			NSLOCTEXT("ECHO7", "ChallengeCompletedProgress", "KHÔI PHỤC HỆ THỐNG: {0}/{1}"),
			FText::AsNumber(CurrentCompletedChallengeCount),
			FText::AsNumber(TotalProgress))
	}, 3.0f);

	OnChallengeCompleted.Broadcast(ChallengeIndex, CurrentCompletedChallengeCount, TotalProgress);
	return true;
}

void AChallengeProgressManager::SetObjective(const FText& NewObjective)
{
	CurrentObjective = NewObjective;
	if (HUDWidget)
	{
		HUDWidget->SetObjectiveText(CurrentObjective);
	}
}

void AChallengeProgressManager::ShowStoryMessage(const FText& Message, float Duration)
{
	if (HUDWidget)
	{
		HUDWidget->ShowMessage(Message, Duration);
	}
}

void AChallengeProgressManager::ShowStoryMessageLines(const TArray<FText>& Lines, float Duration)
{
	if (HUDWidget)
	{
		HUDWidget->ShowMessageLines(Lines, Duration);
	}
}

void AChallengeProgressManager::ClearStoryMessage()
{
	if (HUDWidget)
	{
		HUDWidget->ClearTemporaryMessage();
	}
}

void AChallengeProgressManager::ShowTutorialLines(const TArray<FText>& Lines, float Duration, int32 FontSize)
{
	if (HUDWidget)
	{
		HUDWidget->ShowTutorialLines(Lines, Duration, FontSize);
	}
}

void AChallengeProgressManager::ShowWarning(const FText& Message, float Duration)
{
	if (HUDWidget)
	{
		HUDWidget->ShowWarning(Message, Duration);
	}
}

void AChallengeProgressManager::ShowWarningLines(const TArray<FText>& Lines, float Duration)
{
	if (HUDWidget)
	{
		HUDWidget->ShowWarningLines(Lines, Duration);
	}
}

void AChallengeProgressManager::CreateHUD()
{
	if (HUDWidget)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		UE_LOG(LogECHO7, Warning, TEXT("ChallengeProgressManager could not create its HUD because no local PlayerController is available."));
		return;
	}

	HUDWidget = CreateWidget<UECHO7HUDWidget>(PlayerController, UECHO7HUDWidget::StaticClass());
	if (!HUDWidget)
	{
		UE_LOG(LogECHO7, Error, TEXT("ChallengeProgressManager failed to create UECHO7HUDWidget."));
		return;
	}

	if (!HUDWidget->AddToPlayerScreen(0))
	{
		UE_LOG(LogECHO7, Warning, TEXT("ChallengeProgressManager could not add UECHO7HUDWidget to the player screen."));
		HUDWidget = nullptr;
	}
}

void AChallengeProgressManager::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetObjectiveText(CurrentObjective);
		HUDWidget->SetProgress(CurrentCompletedChallengeCount, TotalChallengeCount);
		HUDWidget->SetRunTimerText(GetFormattedRunTime());
	}
}

void AChallengeProgressManager::RefreshRunTimerHUD()
{
	if (HUDWidget)
	{
		HUDWidget->SetRunTimerText(GetFormattedRunTime());
	}
}

void AChallengeProgressManager::DisplayNextStartingMessage()
{
	if (!HUDWidget || !StartingStoryMessages.IsValidIndex(StartingStoryMessageIndex))
	{
		return;
	}

	HUDWidget->ShowMessage(StartingStoryMessages[StartingStoryMessageIndex], StartingMessageDuration);
	++StartingStoryMessageIndex;

	if (StartingStoryMessages.IsValidIndex(StartingStoryMessageIndex))
	{
		GetWorldTimerManager().SetTimer(
			StartingStoryTimer,
			this,
			&AChallengeProgressManager::DisplayNextStartingMessage,
			StartingMessageDuration + StartingMessageGap,
			false);
	}
}

void AChallengeProgressManager::BindInteractionPrompt()
{
	AECHO7Character* PlayerCharacter = Cast<AECHO7Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	UInteractionComponent* InteractionComponent = PlayerCharacter->GetInteractionComponent();
	if (!InteractionComponent)
	{
		return;
	}

	if (BoundInteractionComponent && BoundInteractionComponent != InteractionComponent)
	{
		BoundInteractionComponent->OnFocusedInteractableChanged.RemoveDynamic(this, &AChallengeProgressManager::HandleFocusedInteractableChanged);
	}

	BoundInteractionComponent = InteractionComponent;
	BoundInteractionComponent->OnFocusedInteractableChanged.AddUniqueDynamic(this, &AChallengeProgressManager::HandleFocusedInteractableChanged);
	HandleFocusedInteractableChanged(nullptr, BoundInteractionComponent->GetFocusedInteractable());
}

void AChallengeProgressManager::HandleFocusedInteractableChanged(AActor* PreviousInteractable, AActor* NewInteractable)
{
	if (!HUDWidget)
	{
		return;
	}

	if (!IsValid(NewInteractable) || !NewInteractable->Implements<UInteractableInterface>())
	{
		HUDWidget->ClearInteractionPrompt();
		return;
	}

	AActor* Interactor = BoundInteractionComponent ? BoundInteractionComponent->GetOwner() : nullptr;
	if (!IInteractableInterface::Execute_CanInteract(NewInteractable, Interactor))
	{
		HUDWidget->ClearInteractionPrompt();
		return;
	}

	HUDWidget->SetInteractionPrompt(IInteractableInterface::Execute_GetInteractionPrompt(NewInteractable));
}
