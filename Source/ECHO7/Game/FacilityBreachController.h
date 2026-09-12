// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FacilityBreachController.generated.h"

class AChallengeProgressManager;
class ALight;
class ASciFiDoor;
class USceneComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFacilityBreachStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFacilityBreachSequenceCompletedSignature);

/**
 * Starts the one-time facility breach sequence once all global challenges are complete.
 * Existing level lights, doors, and sounds are configured per placed instance.
 */
UCLASS(Blueprintable)
class ECHO7_API AFacilityBreachController : public AActor
{
	GENERATED_BODY()

public:
	AFacilityBreachController();

	/** True after the calm delay has elapsed and the emergency breach is active. */
	UFUNCTION(BlueprintPure, Category = "Facility Breach")
	bool IsBreachActive() const { return bBreachActive; }

	/** Broadcast once when the emergency breach begins. */
	UPROPERTY(BlueprintAssignable, Category = "Facility Breach")
	FFacilityBreachStartedSignature OnBreachStarted;

	/** Broadcast after the final breach message and evacuation objective are shown. */
	UPROPERTY(BlueprintAssignable, Category = "Facility Breach")
	FFacilityBreachSequenceCompletedSignature OnBreachSequenceCompleted;

	/** Global challenge owner that triggers this controller at 3/3. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Disable on maps that use the Challenge 3 repair-and-return ending route instead of automatic breach evacuation. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge|Legacy Route")
	bool bReactToFinalChallenge = true;

	/** Delay after challenge 3 completes before the emergency sequence begins. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float CalmDelay = 3.0f;

	/** Time the warning message is displayed before the breach message. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float WarningDuration = 1.5f;

	/** Time the breach message is displayed before the subject-status message. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float BreachMessageDuration = 2.0f;

	/** Time the subject-status message is displayed. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float SubjectMessageDuration = 2.0f;

	/** Existing normal lights disabled when the breach begins. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Emergency Lights")
	TArray<TObjectPtr<ALight>> NormalLights;

	/** Existing emergency lights enabled when the breach begins. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Emergency Lights")
	TArray<TObjectPtr<ALight>> EmergencyLights;

	/** Enables a short timer-driven emergency-light flicker before the lights settle on. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Emergency Lights")
	bool bUseEmergencyFlicker = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Emergency Lights", meta = (ClampMin = "0.0", Units = "s"))
	float FlickerDuration = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Emergency Lights", meta = (ClampMin = "0.01", Units = "s"))
	float FlickerInterval = 0.12f;

	/** Escape-route doors unlocked at the start of the breach. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Doors")
	TArray<TObjectPtr<ASciFiDoor>> DoorsToUnlock;

	/** Doors locked when their existing safe door implementation permits it. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Doors")
	TArray<TObjectPtr<ASciFiDoor>> DoorsToCloseOrLock;

	/** Optional alarm played when the emergency breach begins. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> AlarmSound;

	/** Optional breach sound played with the isolation-breach message. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> BreachSound;

	/** Visible in PIE to confirm this controller has reacted to the completed challenge. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	bool bBreachStarted = false;

	/** Visible in PIE to confirm the emergency portion of the sequence is active. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	bool bBreachActive = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleChallengeCompleted(int32 ChallengeIndex, int32 CurrentProgress, int32 TotalProgress);

	void StartBreachSequence();
	void BeginEmergencyBreach();
	void ShowBreachMessage();
	void ShowSubjectMessage();
	void FinishBreachSequence();
	void ToggleEmergencyLights();
	void FinishEmergencyFlicker();
	void SetLightsEnabled(const TArray<TObjectPtr<ALight>>& Lights, bool bEnabled) const;
	void UnlockEscapeRouteDoors();
	void LockConfiguredDoors();
	void PlayOptionalSound(USoundBase* Sound) const;
	void ClearBreachTimers();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	FTimerHandle CalmDelayTimer;
	FTimerHandle WarningTimer;
	FTimerHandle BreachMessageTimer;
	FTimerHandle SubjectMessageTimer;
	FTimerHandle FlickerTimer;
	FTimerHandle FlickerEndTimer;
	bool bEmergencyLightsVisible = false;
	bool bBreachSequenceCompleted = false;
};
