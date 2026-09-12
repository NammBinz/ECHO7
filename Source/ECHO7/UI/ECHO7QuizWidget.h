// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/QuizTerminal.h"
#include "ECHO7QuizWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuizCompletedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuizCancelledSignature);

/** Native multiple-choice security quiz UI used by AQuizTerminal. */
UCLASS()
class ECHO7_API UECHO7QuizWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Supplies this quiz's editor-configured questions and restarts at question one. */
	void InitializeQuiz(const TArray<FQuizQuestion>& InQuestions);

	/** Configures the native presentation values authored on the placed quiz terminal. */
	void ConfigurePresentation(
		float InEnergyChargeDuration,
		float InQuestionGlitchDuration,
		USoundBase* InQuestionAppearSound,
		USoundBase* InCorrectAnswerSound,
		USoundBase* InIncorrectAnswerSound,
		USoundBase* InEnergyChargeSound,
		USoundBase* InQuizGlitchSound);

	UPROPERTY(BlueprintAssignable, Category = "Quiz")
	FQuizCompletedSignature OnQuizCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Quiz")
	FQuizCancelledSignature OnQuizCancelled;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void RefreshQuestion();
	void BuildQuestionLineRows();
	void BuildEnergyCells();
	void BeginQuestionPresentation();
	void StartQuestionGlitch(bool bShowSuccessWhenFinished);
	void ApplyQuestionGlitchPulse();
	void ScheduleNextQuestionGlitchPulse();
	void FinishQuestionGlitch();
	void BeginEnergyCharge();
	void UpdateEnergyCharge();
	void FinishEnergyCharge();
	void SetEnergyCellProgress(int32 CellIndex, float Progress, bool bCharged);
	void SetAnswerButtonFeedback(EQuizAnswer SelectedAnswer, bool bCorrect);
	void ApplyAnswerButtonStyle(UButton* Button, const FLinearColor& BackgroundColor, const FLinearColor& BorderColor) const;
	void SetQuestionContentVisible(bool bVisible);
	void TriggerFeedbackFlash(const FLinearColor& Color, float Opacity);
	void ClearFeedbackFlash();
	void PlayOptionalSound(USoundBase* Sound) const;
	void HandleAnswerSelected(EQuizAnswer SelectedAnswer);
	void SetAnswerButtonsEnabled(bool bEnabled);
	void AdvanceToNextQuestion();
	void ResetAfterIncorrectAnswer();
	void FinishQuiz();
	void CompleteQuizAfterFeedback();
	void CancelQuiz();

	UFUNCTION()
	void HandleAnswerA();

	UFUNCTION()
	void HandleAnswerB();

	UFUNCTION()
	void HandleAnswerC();

	UFUNCTION()
	void HandleAnswerD();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DimBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> QuizPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CounterText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> QuestionBox;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> QuestionLinesContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> QuestionLineRows;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> EnergyCellsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> EnergyCellContainers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProgressBar>> EnergyCells;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> AnswerGrid;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AnswerAButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AnswerBButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AnswerCButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AnswerDButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> AnswerButtonContainers;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnswerAText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnswerBText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnswerCText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> AnswerDText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FooterText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> GlitchOverlay;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> GlitchStrips;

	TArray<FQuizQuestion> QuizQuestions;
	FTimerHandle FeedbackTimer;
	FTimerHandle EnergyChargeTimer;
	FTimerHandle QuestionGlitchPulseTimer;
	FTimerHandle QuestionGlitchEndTimer;
	FTimerHandle FeedbackFlashTimer;
	int32 CurrentQuestionIndex = 0;
	int32 ChargedCellCount = 0;
	int32 ChargingCellIndex = INDEX_NONE;
	float EnergyChargeStartTime = 0.0f;
	float EnergyChargeDuration = 0.45f;
	float QuestionGlitchDuration = 0.20f;
	TObjectPtr<USoundBase> QuestionAppearSound;
	TObjectPtr<USoundBase> CorrectAnswerSound;
	TObjectPtr<USoundBase> IncorrectAnswerSound;
	TObjectPtr<USoundBase> EnergyChargeSound;
	TObjectPtr<USoundBase> QuizGlitchSound;
	bool bQuizInitialized = false;
	bool bQuestionContentVisible = false;
	bool bQuestionGlitchActive = false;
	bool bShowSuccessAfterGlitch = false;
	bool bAwaitingResponse = false;
	bool bCompletionPending = false;
};
