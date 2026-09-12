// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/ECHO7Typewriter.h"
#include "GameFramework/Actor.h"
#include "AIMovementTestController.generated.h"

class AChallengeProgressManager;
class ALight;
class APointLight;
class APawn;
class APlayerController;
class UBoxComponent;
class ULightComponent;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;
class UECHO7StoryOverlayWidget;

/** The active phase of the AI movement test. */
UENUM(BlueprintType)
enum class EAIMovementTestState : uint8
{
	Idle,
	Starting,
	Move,
	Scanning,
	Failed,
	Completed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementTestStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementTestMovePhaseStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementTestScanPhaseStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementTestFailedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementTestCompletedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAIMovementScanCorridorCompletedSignature);

struct FECHO7DefaultRoomLightState
{
	TWeakObjectPtr<ULightComponent> LightComponent;
	float Intensity = 0.0f;
	FLinearColor Color = FLinearColor::White;
	bool bVisible = true;
};

/**
 * A placeable red-light/green-light movement challenge.
 * The player may move during Move and must remain within the configured XY tolerance during Scanning.
 */
UCLASS(Blueprintable)
class ECHO7_API AAIMovementTestController : public AActor
{
	GENERATED_BODY()

public:
	AAIMovementTestController();

	/** Starts the test for the supplied player when it is currently idle. */
	UFUNCTION(BlueprintCallable, Category = "AI Movement Test")
	void StartTest(APawn* PlayerPawn);

	/** Causes the active scanning phase to fail and schedule its normal retry. */
	UFUNCTION(BlueprintCallable, Category = "AI Movement Test")
	void FailTest();

	UFUNCTION(BlueprintPure, Category = "AI Movement Test")
	EAIMovementTestState GetCurrentState() const { return CurrentState; }

	/** True once FinishZone has disabled this run's movement-scan corridor. */
	UFUNCTION(BlueprintPure, Category = "AI Movement Test")
	bool HasClearedScanCorridor() const { return bScanCorridorCleared; }

	/** Fired when the player initially starts this controller's test. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementTestStartedSignature OnTestStarted;

	/** Fired each time the movement-permitted phase starts. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementTestMovePhaseStartedSignature OnMovePhaseStarted;

	/** Fired each time the no-movement scanning phase starts. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementTestScanPhaseStartedSignature OnScanPhaseStarted;

	/** Fired after movement is detected during scanning. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementTestFailedSignature OnTestFailed;

	/** Fired once after the player validly reaches the finish zone. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementTestCompletedSignature OnTestCompleted;

	/** Fired when FinishZone stops the scan corridor without completing Challenge 3. */
	UPROPERTY(BlueprintAssignable, Category = "AI Movement Test")
	FAIMovementScanCorridorCompletedSignature OnScanCorridorCompleted;

	/** Manager that owns the global three-challenge recovery progression. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Challenge index submitted once the finish zone is reached. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (ClampMin = "1", ClampMax = "3"))
	int32 ChallengeIndex = 3;

	/** Objective shown while the movement test is active. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveWhileTesting;

	/** Objective shown after the movement test completes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveAfterCompletion;

	/** Objective shown after FinishZone disables the scan corridor. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveAfterScanCorridor;

	/** Legacy route guard. Leave false for the fault-repair Challenge 3 flow. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge|Legacy")
	bool bCompleteChallengeOnFinishZone = false;

	/** Tutorial rows shown cumulatively before the first test attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial")
	TArray<FText> TutorialLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "0.0", Units = "s"))
	float TutorialLineInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "0.0", Units = "s"))
	float TutorialEndDelay = 5.0f;

	/** Font size used for the centered tutorial rows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "12.0"))
	int32 TutorialFontSize = 52;

	/** Font size used for the centered 3 / 2 / 1 countdown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "12.0"))
	int32 CountdownFontSize = 100;

	/** Unicode grapheme clusters revealed each second for every tutorial row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "1.0"))
	float TutorialCharactersPerSecond = 28.0f;

	/** Pause after a tutorial row completes typing before the next row begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "0.0", Units = "s"))
	float TutorialLinePause = 0.35f;

	/** Optional sound played once whenever a tutorial row appears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial")
	TObjectPtr<USoundBase> TutorialLineSound;

	/** Optional throttled typing sound played while a tutorial row reveals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial")
	TObjectPtr<USoundBase> TutorialTypeSound;

	/** Short transition pulse shown before story rows and before the countdown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "0.01", Units = "s"))
	float TutorialGlitchDuration = 0.30f;

	/** Seconds each centered countdown value remains on screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Tutorial", meta = (ClampMin = "0.05", Units = "s"))
	float CountdownStepDuration = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float InitialDelay = 2.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float MinMoveDuration = 2.5f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float MaxMoveDuration = 5.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float MinScanDuration = 1.5f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float MaxScanDuration = 3.5f;

	/** Player reaction window before strict movement detection begins during each scanning phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Movement Test|Scanning", meta = (ClampMin = "0.0", Units = "s", DisplayName = "Scan Movement Grace Period"))
	float ScanGracePeriod = 0.5f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float FailureRestartDelay = 1.5f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float CompletionMessageDuration = 2.0f;

	/** Maximum permitted horizontal displacement from the scanning reference location, in cm. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.0", Units = "cm"))
	float MovementTolerance = 8.0f;

	/** Frequency of horizontal movement checks while scanning. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Detection", meta = (ClampMin = "0.05", ClampMax = "0.10", Units = "s"))
	float MovementCheckInterval = 0.075f;

	/** Optional actor used as the retry destination. The first StartZone entry is used when it is not assigned. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Respawn")
	TObjectPtr<AActor> RespawnPoint;

	/** Existing room lights enabled during Move. Empty arrays are supported. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Visual Feedback")
	TArray<TObjectPtr<ALight>> MovePhaseLights;

	/** Existing room lights enabled during Scanning. Empty arrays are supported. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Visual Feedback")
	TArray<TObjectPtr<ALight>> ScanPhaseLights;

	/** Preferred list: any placed actor containing one or more alarm ULightComponent-derived components. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights")
	TArray<TObjectPtr<AActor>> WarningLightActors;

	/** Compatibility list for PointLights assigned before WarningLightActors was introduced. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights|Legacy", meta = (DisplayName = "Warning Point Lights (Legacy)"))
	TArray<TObjectPtr<APointLight>> WarningLights;

	/** Preferred list: any placed actor containing one or more ULightComponent-derived components. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights")
	TArray<TObjectPtr<AActor>> DefaultRoomLightActors;

	/** Compatibility list for PointLights assigned before DefaultRoomLightActors was introduced. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights|Legacy", meta = (DisplayName = "Default Room Point Lights (Legacy)"))
	TArray<TObjectPtr<APointLight>> DefaultRoomLights;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights")
	bool bEnableWarningLights = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights")
	FLinearColor WarningLightColor = FLinearColor::Red;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights", meta = (ClampMin = "0.0"))
	float WarningMaxIntensity = 5000.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights", meta = (ClampMin = "0.01", Units = "s"))
	float WarningBlinkInterval = 0.18f;

	/** Multiplier applied to each cached normal-light intensity only while Scanning. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge 3|Warning Lights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DefaultLightScanMultiplier = 0.10f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> MoveSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ScanSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FailureSound;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> CompletionSound;

	/** Resizable player-entry trigger, visible in the editor viewport. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> StartZone;

	/** Resizable valid-completion trigger, visible in the editor viewport. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> FinishZone;

	/** Current state, exposed for PIE debugging. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	EAIMovementTestState CurrentState = EAIMovementTestState::Idle;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleStartZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleFinishZoneBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void BeginMovePhase();
	void BeginScanningPhase();
	void BeginTutorialOrStartTest(APawn* PlayerPawn);
	void DisplayNextTutorialLine();
	void UpdateTutorialTypewriter();
	void CompleteCurrentTutorialLine();
	void RefreshTutorialDisplay();
	void FinishTutorial();
	void BeginCountdown();
	void AdvanceCountdown();
	void StartTestAfterCountdown();
	void ClearTutorialTimers();
	bool PrepareTutorialOverlay();
	void RemoveTutorialOverlay();
	void LockTutorialInput();
	void UnlockTutorialInput();
	void BeginMovementDetection();
	void CheckPlayerMovement();
	void ShowFailureMessage();
	void RestartAfterFailure();
	void CompleteTest();
	void CompleteScanCorridor();
	void ClearChallengeTimers();
	void ResetScanLightsImmediately();
	void StartWarningLights();
	void StopWarningLights();
	void HandleWarningLightBlink();
	void SetWarningLightIntensity(float Intensity) const;
	void CacheWarningLightComponents();
	void CacheDefaultRoomLights();
	void DimDefaultRoomLightsForScan();
	void RestoreDefaultRoomLights();
	void UpdatePhaseLights();
	void SetLightsVisible(const TArray<TObjectPtr<ALight>>& Lights, bool bVisible) const;
	void PlayOptionalSound(USoundBase* Sound) const;
	void ShowMessage(const FText& Message, float Duration, bool bWarning = false) const;
	void ShowMessageLines(const TArray<FText>& Lines, float Duration, bool bWarning = false) const;
	void StopPlayerMovement() const;

	APawn* GetPlayerPawnFromOverlap(AActor* OtherActor) const;
	bool IsTestActive() const;
	bool HasPlayerMovedBeyondTolerance() const;
	float GetRandomDuration(float MinimumDuration, float MaximumDuration) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ActivePlayer;

	FTransform CachedStartTransform;
	FVector ScanReferenceLocation = FVector::ZeroVector;
	FTimerHandle InitialDelayTimer;
	FTimerHandle MovePhaseTimer;
	FTimerHandle ScanPhaseTimer;
	FTimerHandle ScanGraceTimer;
	FTimerHandle MovementDetectionTimer;
	FTimerHandle FailureMessageTimer;
	FTimerHandle FailureRestartTimer;
	FTimerHandle WarningLightBlinkTimer;
	FTimerHandle TutorialTypewriterTimer;
	FTimerHandle TutorialLinePauseTimer;
	FTimerHandle TutorialEndTimer;
	FTimerHandle TutorialTransitionTimer;
	FTimerHandle CountdownTimer;
	TArray<FText> VisibleTutorialLines;
	FECHO7TypewriterState TutorialTypewriter;
	int32 NextTutorialLineIndex = 0;
	float TutorialLineStartTime = 0.0f;
	int32 CountdownValue = 3;
	bool bHasCachedStartTransform = false;
	bool bChallengeCompletionAttempted = false;
	bool bMovementDetectionActive = false;
	bool bTutorialActive = false;
	bool bTutorialCompleted = false;
	bool bScanCorridorCleared = false;
	bool bTutorialInputLocked = false;
	bool bWarningLightsLit = false;
	bool bWarningLightComponentsCached = false;
	bool bDefaultRoomLightsCached = false;

	TArray<TWeakObjectPtr<ULightComponent>> CachedWarningLightComponents;
	TArray<FECHO7DefaultRoomLightState> CachedDefaultRoomLightStates;

	UPROPERTY(Transient)
	TObjectPtr<UECHO7StoryOverlayWidget> TutorialOverlayWidget;

	TWeakObjectPtr<APlayerController> TutorialPlayerController;
};
