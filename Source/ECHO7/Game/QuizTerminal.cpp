// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/QuizTerminal.h"

#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Engine/Engine.h"
#include "Environment/SciFiDoor.h"
#include "Game/ChallengeProgressManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/ECHO7QuizWidget.h"

AQuizTerminal::AQuizTerminal()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "QuizTerminalInteractionPrompt", "Thực hiện xác minh bảo mật");
	ObjectiveAfterCompletion = NSLOCTEXT("ECHO7", "QuizTerminalObjectiveAfterCompletion", "Đi tới khu thử nghiệm AI.");
}

void AQuizTerminal::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	if (Questions.IsEmpty())
	{
		ShowMissingQuestionDataMessage();
		return;
	}

	const AECHO7Character* Character = Cast<AECHO7Character>(Interactor);
	APlayerController* PlayerController = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		UE_LOG(LogECHO7, Warning, TEXT("QuizTerminal could not find a local PlayerController."));
		return;
	}

	OpenQuiz(PlayerController);
}

bool AQuizTerminal::CanInteract_Implementation(AActor* Interactor) const
{
	return IsValid(Interactor) && !bCompleted && !bQuizOpen && Cast<AECHO7Character>(Interactor) != nullptr;
}

void AQuizTerminal::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	CloseQuiz();
	Super::EndPlay(EndPlayReason);
}

void AQuizTerminal::OpenQuiz(APlayerController* PlayerController)
{
	if (!PlayerController || bQuizOpen || ActiveQuizWidget)
	{
		return;
	}

	ActiveQuizWidget = CreateWidget<UECHO7QuizWidget>(PlayerController, UECHO7QuizWidget::StaticClass());
	if (!ActiveQuizWidget)
	{
		UE_LOG(LogECHO7, Warning, TEXT("QuizTerminal failed to create its quiz widget."));
		return;
	}

	ActivePlayerController = PlayerController;
	ActiveQuizWidget->OnQuizCompleted.AddDynamic(this, &AQuizTerminal::HandleQuizCompleted);
	ActiveQuizWidget->OnQuizCancelled.AddDynamic(this, &AQuizTerminal::HandleQuizCancelled);
	ActiveQuizWidget->ConfigurePresentation(
		EnergyChargeDuration,
		QuestionGlitchDuration,
		QuestionAppearSound.Get(),
		CorrectAnswerSound.Get(),
		IncorrectAnswerSound.Get(),
		EnergyChargeSound.Get(),
		QuizGlitchSound.Get());
	ActiveQuizWidget->InitializeQuiz(Questions);

	if (!ActiveQuizWidget->AddToPlayerScreen(20))
	{
		UE_LOG(LogECHO7, Warning, TEXT("QuizTerminal failed to add its quiz widget to the player screen."));
		ActiveQuizWidget->OnQuizCompleted.RemoveDynamic(this, &AQuizTerminal::HandleQuizCompleted);
		ActiveQuizWidget->OnQuizCancelled.RemoveDynamic(this, &AQuizTerminal::HandleQuizCancelled);
		ActiveQuizWidget = nullptr;
		ActivePlayerController = nullptr;
		return;
	}

	bQuizOpen = true;
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
	bQuizInputCaptured = true;

	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	ActiveQuizWidget->SetKeyboardFocus();
}

void AQuizTerminal::CloseQuiz()
{
	if (ActiveQuizWidget)
	{
		ActiveQuizWidget->OnQuizCompleted.RemoveDynamic(this, &AQuizTerminal::HandleQuizCompleted);
		ActiveQuizWidget->OnQuizCancelled.RemoveDynamic(this, &AQuizTerminal::HandleQuizCancelled);
		ActiveQuizWidget->RemoveFromParent();
		ActiveQuizWidget = nullptr;
	}

	RestorePlayerInput();
	bQuizOpen = false;
}

void AQuizTerminal::RestorePlayerInput()
{
	if (bQuizInputCaptured && IsValid(ActivePlayerController))
	{
		ActivePlayerController->SetIgnoreMoveInput(false);
		ActivePlayerController->SetIgnoreLookInput(false);
		ActivePlayerController->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		ActivePlayerController->SetInputMode(InputMode);
	}

	bQuizInputCaptured = false;
	ActivePlayerController = nullptr;
}

void AQuizTerminal::ShowMissingQuestionDataMessage() const
{
	const FText Message = NSLOCTEXT("ECHO7", "QuizTerminalMissingQuestionData", "KHÔNG TÌM THẤY DỮ LIỆU XÁC MINH");
	UE_LOG(LogECHO7, Warning, TEXT("QuizTerminal has no configured questions."));

	if (ChallengeProgressManager)
	{
		ChallengeProgressManager->ShowWarning(Message, 3.0f);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message.ToString());
	}
}

void AQuizTerminal::HandleQuizCompleted()
{
	bCompleted = true;
	CloseQuiz();

	if (ChallengeProgressManager)
	{
		if (ChallengeProgressManager->CompleteChallenge(ChallengeIndex) && !ObjectiveAfterCompletion.IsEmpty())
		{
			ChallengeProgressManager->SetObjective(ObjectiveAfterCompletion);
		}
	}
	else
	{
		UE_LOG(LogECHO7, Warning, TEXT("QuizTerminal completed without a ChallengeProgressManager."));
	}

	for (ASciFiDoor* Door : DoorsToUnlock)
	{
		if (IsValid(Door))
		{
			Door->UnlockDoor();
		}
	}
}

void AQuizTerminal::HandleQuizCancelled()
{
	CloseQuiz();
}
