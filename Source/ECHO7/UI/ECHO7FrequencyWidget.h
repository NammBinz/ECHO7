// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7FrequencyWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UOverlay;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrequencyCalibrationTickSignature, bool, bIncreaseFrequency, float, DeltaSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyCalibrationCancelledSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyCalibrationSucceededSignature);

/** Native keyboard-driven UI for calibrating one AFrequencyGenerator. */
UCLASS()
class ECHO7_API UECHO7FrequencyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeCalibration(float InMinimumFrequency, float InMaximumFrequency, float InTargetFrequency, float InTargetTolerance, float InStartingFrequency, float InRequiredStableDuration);
	void ConfigureOpeningGlitch(float InDuration);
	void UpdateCalibrationState(float InCurrentFrequency, float InStableTime);
	void ShowSuccess();

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyCalibrationTickSignature OnCalibrationTick;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyCalibrationCancelledSignature OnCalibrationCancelled;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyCalibrationSucceededSignature OnCalibrationSucceeded;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void RefreshDisplay();
	void StartCalibrationTimer();
	void ArmInput();
	void StartOpeningGlitch();
	void ApplyOpeningGlitchPulse();
	void ScheduleNextOpeningGlitchPulse();
	void FinishOpeningGlitch();
	void UpdateCalibration(float DeltaSeconds);
	void FinishSuccessfulCalibration();
	void CancelCalibration();
	float GetFrequencyPercent() const;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CalibrationPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FrequencyText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> FrequencyBarContainer;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> FrequencyBarOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> FrequencyBar;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> FrequencyOverlayCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TargetRangeOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PostTargetFrequencyFill;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CurrentFrequencyMarker;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TargetRangeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IncreaseInstructionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DecreaseInstructionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StabilityText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> OpeningGlitchOverlay;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> OpeningGlitchStrips;

	FTimerHandle InputArmTimer;
	FTimerHandle SuccessTimer;
	FTimerHandle OpeningGlitchPulseTimer;
	FTimerHandle OpeningGlitchEndTimer;
	float MinimumFrequency = 0.0f;
	float MaximumFrequency = 100.0f;
	float TargetFrequency = 60.0f;
	float TargetTolerance = 8.0f;
	float CurrentFrequency = 25.0f;
	float StableTime = 0.0f;
	float RequiredStableDuration = 2.0f;
	float OpeningGlitchDuration = 0.20f;
	bool bInitialized = false;
	bool bInputArmed = false;
	bool bIncreaseHeld = false;
	bool bCompletionPending = false;
	bool bOpeningGlitchActive = false;
};
