// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/ECHO7Typewriter.h"
#include "GameFramework/Actor.h"
#include "ECHO7EndingController.generated.h"

class AEvacuationZone;
class AChallengeProgressManager;
class APlayerController;
class APawn;
class UECHO7EndingWidget;
class USceneComponent;
class USoundBase;

/** One editor-authored line in the post-evacuation ending story. */
USTRUCT(BlueprintType)
struct FECHO7EndingLine
{
	GENERATED_BODY()

	/** Player-facing story text. Vietnamese and other Unicode text are supported. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Text", meta = (MultiLine = "true"))
	FText Text;

	/** Pause after this line finishes typing before the next line begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Text", meta = (ClampMin = "0.0", Units = "s"))
	float DelayAfterLine = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEndingTextCompletedSignature);

/** Plays the one-time post-evacuation teleport, fade, and typewriter ending sequence. */
UCLASS(Blueprintable)
class ECHO7_API AECHO7EndingController : public AActor
{
	GENERATED_BODY()

public:
	AECHO7EndingController();

	/** Receives the hidden final-screen restart key from the local PlayerController. */
	void HandleRestartInput();

	/** Existing evacuation volume whose completion event starts this ending. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<AEvacuationZone> EvacuationZone;

	/** Destination used once the fade has fully hidden the player transition. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Teleport")
	TObjectPtr<AActor> EndingTeleportPoint;

	/** When true, both the pawn and first-person control rotation match the destination. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Teleport")
	bool bMatchTeleportPointRotation = true;

	/** Total duration of the irregular fullscreen glitch that obscures the teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.0", Units = "s"))
	float GlitchDuration = 0.65f;

	/** Seconds after the glitch begins at which the player is moved to EndingTeleportPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.0", Units = "s"))
	float TeleportTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.005", Units = "s"))
	float MinGlitchInterval = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.005", Units = "s"))
	float MaxGlitchInterval = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxDarkFlashOpacity = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrightFlashChance = 0.20f;

	/** Applies a small temporary control-rotation offset during the glitch only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch")
	bool bUseCameraJitter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Glitch", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CameraJitterStrength = 0.8f;

	/** Time that the destination scene remains fully visible after the glitch ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float EndingSceneVisibleDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float FinalBlackFadeDuration = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float TextDelayAfterBlack = 0.20f;

	/** Any number of story lines may be authored and reordered directly in Details. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Text")
	TArray<FECHO7EndingLine> EndingLines;

	/** Grapheme clusters revealed per second by the typewriter. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Text", meta = (ClampMin = "1.0"))
	float CharactersPerSecond = 28.0f;

	/** Optional short sound emitted periodically while non-whitespace graphemes are revealed. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Audio")
	TObjectPtr<USoundBase> TypingSound;

	/** Number of non-whitespace graphemes between short typing sound emissions. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Audio", meta = (ClampMin = "1"))
	int32 TypingSoundEveryNCharacters = 2;

	/** Optional sound played once whenever a non-empty story line begins typing. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending|Audio")
	TObjectPtr<USoundBase> LineStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Debug")
	bool bTestEndingOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ending|Debug", meta = (EditCondition = "bTestEndingOnBeginPlay", ClampMin = "0.0"))
	float TestEndingDelay = 1.0f;

	/** Broadcast once after every configured line has finished typing. */
	UPROPERTY(BlueprintAssignable, Category = "Ending")
	FEndingTextCompletedSignature OnEndingTextCompleted;

	/** True after this controller has accepted evacuation completion and locked the ending state. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "State")
	bool bEndingStarted = false;

	/** True after the transparent ending story widget is visible. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "State")
	bool bEndingTextVisible = false;

	/** True once all configured story lines have completed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "State")
	bool bEndingTextCompleted = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleEvacuationCompleted();

	void StartEndingSequence();
	void BeginGlitchSequence();
	void ApplyGlitchPulse();
	void ScheduleNextGlitchPulse();
	void TeleportPlayerDuringGlitch();
	void EndGlitchSequence();
	void StartFinalBlackFade();
	void UpdateFinalBlackFade();
	void CompleteFinalBlackFade();
	void BeginEndingText();
	void BeginNextEndingLine();
	void UpdateTypewriter();
	void CompleteCurrentEndingLine();
	void CompleteEndingText();
	void CaptureCompletionTime();
	void EnableEndingRestartInput();
	void DisableEndingRestartInput();
	void RestartCurrentLevel();
	void PlayOptionalSound(USoundBase* Sound) const;
	bool PrepareEndingWidget();
	bool LockLocalPlayerInput();
	void UnlockLocalPlayerInput();
	APlayerController* GetLocalPlayerController() const;
	APawn* GetEndingPlayerPawn() const;
	void StopPlayerMovement(APawn* PlayerPawn) const;
	void ClearEndingTimers();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TObjectPtr<UECHO7EndingWidget> EndingWidget;

	FTimerHandle GlitchPulseTimer;
	FTimerHandle TeleportTimer;
	FTimerHandle GlitchEndTimer;
	FTimerHandle EndingSceneVisibleTimer;
	FTimerHandle FinalBlackFadeTimer;
	FTimerHandle TextStartTimer;
	FTimerHandle TypewriterTimer;
	FTimerHandle LineDelayTimer;
	FTimerHandle TestEndingTimer;

	TWeakObjectPtr<APlayerController> LockedPlayerController;
	TWeakObjectPtr<APawn> LockedPlayerPawn;
	TArray<FText> CompletedEndingLines;
	FECHO7TypewriterState ActiveLineTypewriter;

	UPROPERTY(Transient)
	FText FrozenCompletionTimeText;

	int32 CurrentEndingLineIndex = 0;
	float ActiveLineStartTime = 0.0f;
	float FinalBlackFadeStartTime = 0.0f;
	FRotator StableControlRotation = FRotator::ZeroRotator;
	bool bInputLocked = false;
	bool bGlitchActive = false;
	bool bTeleportPerformed = false;
	bool bHasStableControlRotation = false;
	bool bRestartInputEnabled = false;
	bool bRestartRequested = false;
};
