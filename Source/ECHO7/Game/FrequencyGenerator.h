// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "FrequencyGenerator.generated.h"

class AFrequencySyncController;
class APlayerController;
class ASciFiDoor;
class UECHO7FrequencyWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyCalibrationStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyTargetStateSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyGeneratorStabilizedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyGeneratorStageActionSignature);

/** An interactable generator that must be held within its configured frequency range. */
UCLASS(Blueprintable)
class ECHO7_API AFrequencyGenerator : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AFrequencyGenerator();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Enables this station for its controller-defined route stage. Stabilized stations remain disabled. */
	UFUNCTION(BlueprintCallable, Category = "Frequency|Progression")
	void SetStageEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Frequency|Progression")
	bool IsStageEnabled() const { return bStageEnabled; }

	/** Starts the same calibration flow used by normal interaction, after validating player and stage state. */
	UFUNCTION(BlueprintCallable, Category = "Frequency")
	bool StartCalibration(AActor* Interactor);

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	float StartingFrequency = 25.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	float MinFrequency = 0.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	float MaxFrequency = 100.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	float TargetFrequency = 60.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency", meta = (ClampMin = "0.0"))
	float TargetTolerance = 8.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency", meta = (ClampMin = "0.0"))
	float IncreaseRate = 30.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency", meta = (ClampMin = "0.0"))
	float DecreaseRate = 20.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency", meta = (ClampMin = "0.01", Units = "s"))
	float RequiredStableDuration = 2.0f;

	/** Short native UI transition played before calibration input becomes available. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float CalibrationOpenGlitchDuration = 0.20f;

	/** Controller that aggregates this generator with the other Challenge 2 generators. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AFrequencySyncController> FrequencySyncController;

	/** Player-facing station name used by the route-progress fallback message. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion")
	FText GeneratorDisplayName;

	/** Optional manual completion rows. Each entry maps to one temporary HUD row. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion", meta = (MultiLine = "true"))
	TArray<FText> StageCompletionLines;

	/** Doors unlocked after this station stabilizes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion")
	TArray<TObjectPtr<ASciFiDoor>> DoorsToUnlock;

	/** Existing actors hidden and made non-colliding after this station stabilizes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion")
	TArray<TObjectPtr<AActor>> ActorsToDisable;

	/** Existing actors revealed and made collidable after this station stabilizes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion")
	TArray<TObjectPtr<AActor>> ActorsToEnable;

	/** Opens assigned doors after unlocking them when the existing door flow permits it. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Stage Completion")
	bool bOpenUnlockedDoorsImmediately = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	bool bStabilized = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Frequency")
	bool bCalibrationOpen = false;

	/** Whether this station is currently unlocked by the route controller. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Frequency|Progression")
	bool bStageEnabled = true;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyCalibrationStartedSignature OnCalibrationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyTargetStateSignature OnFrequencyEnteredTarget;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyTargetStateSignature OnFrequencyLeftTarget;

	UPROPERTY(BlueprintAssignable, Category = "Frequency")
	FFrequencyGeneratorStabilizedSignature OnGeneratorStabilized;

	UPROPERTY(BlueprintAssignable, Category = "Stage Completion")
	FFrequencyGeneratorStageActionSignature OnStageUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Stage Completion")
	FFrequencyGeneratorStageActionSignature OnStageObstacleDisabled;

	UPROPERTY(BlueprintAssignable, Category = "Stage Completion")
	FFrequencyGeneratorStageActionSignature OnStageCompleted;

protected:
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void OpenCalibration(APlayerController* PlayerController);
	void CloseCalibration();
	void RestorePlayerInput();
	void StabilizeGenerator();
	void ApplyStageCompletionActions();
	void UpdateFrequencyWidget() const;
	float GetMinimumFrequency() const;
	float GetMaximumFrequency() const;
	float GetClampedTargetFrequency() const;
	float GetClampedTolerance() const;
	float GetRequiredDuration() const;
	bool IsFrequencyInTargetRange() const;

	UFUNCTION()
	void HandleCalibrationTick(bool bIncreaseFrequency, float DeltaSeconds);

	UFUNCTION()
	void HandleCalibrationCancelled();

	UFUNCTION()
	void HandleCalibrationSucceeded();

	UPROPERTY(Transient)
	TObjectPtr<UECHO7FrequencyWidget> ActiveFrequencyWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ActivePlayerController;

	float CurrentFrequency = 0.0f;
	float StableTime = 0.0f;
	bool bWasInTargetRange = false;
	bool bInputCaptured = false;
};
