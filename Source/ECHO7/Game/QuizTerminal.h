// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "QuizTerminal.generated.h"

class AChallengeProgressManager;
class APlayerController;
class ASciFiDoor;
class UECHO7QuizWidget;
class USoundBase;

/** Identifies the correct answer for an editor-configured quiz question. */
UENUM(BlueprintType)
enum class EQuizAnswer : uint8
{
	A UMETA(DisplayName = "A"),
	B UMETA(DisplayName = "B"),
	C UMETA(DisplayName = "C"),
	D UMETA(DisplayName = "D")
};

/** One editor-configurable multiple-choice security question. */
USTRUCT(BlueprintType)
struct ECHO7_API FQuizQuestion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	FText QuestionText;

	/** Optional manual display rows. When set, these take precedence over the legacy QuestionText row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz", meta = (MultiLine = "true"))
	TArray<FText> QuestionLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	FText AnswerA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	FText AnswerB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	FText AnswerC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	FText AnswerD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	EQuizAnswer CorrectAnswer = EQuizAnswer::A;
};

/** An interactable terminal that completes a challenge after a configured security quiz. */
UCLASS(Blueprintable)
class ECHO7_API AQuizTerminal : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AQuizTerminal();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Questions configured per placed terminal. No question content is defined in C++. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Quiz", meta = (TitleProperty = "QuestionText"))
	TArray<FQuizQuestion> Questions;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Presentation", meta = (ClampMin = "0.05", Units = "s"))
	float EnergyChargeDuration = 0.45f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Presentation", meta = (ClampMin = "0.05", Units = "s"))
	float QuestionGlitchDuration = 0.20f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Audio")
	TObjectPtr<USoundBase> QuestionAppearSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Audio")
	TObjectPtr<USoundBase> CorrectAnswerSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Audio")
	TObjectPtr<USoundBase> IncorrectAnswerSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Audio")
	TObjectPtr<USoundBase> EnergyChargeSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quiz|Audio")
	TObjectPtr<USoundBase> QuizGlitchSound;

	/** Progress manager notified after the quiz is completed. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Challenge index registered after successful quiz completion. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (ClampMin = "1", ClampMax = "3"))
	int32 ChallengeIndex = 1;

	/** Objective displayed after successfully completing the assigned challenge. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveAfterCompletion;

	/** Optional doors unlocked after successfully completing this quiz. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Door")
	TArray<TObjectPtr<ASciFiDoor>> DoorsToUnlock;

	/** True once this terminal has completed its quiz during the current play session. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Quiz")
	bool bCompleted = false;

	/** True while this terminal owns an active quiz widget. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Quiz")
	bool bQuizOpen = false;

protected:
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void OpenQuiz(APlayerController* PlayerController);
	void CloseQuiz();
	void RestorePlayerInput();
	void ShowMissingQuestionDataMessage() const;

	UFUNCTION()
	void HandleQuizCompleted();

	UFUNCTION()
	void HandleQuizCancelled();

	UPROPERTY(Transient)
	TObjectPtr<UECHO7QuizWidget> ActiveQuizWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ActivePlayerController;

	bool bQuizInputCaptured = false;
};
