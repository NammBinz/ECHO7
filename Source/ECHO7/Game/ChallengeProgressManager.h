// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChallengeProgressManager.generated.h"

class AMainReactor;
class UECHO7HUDWidget;
class UInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FChallengeCompletedSignature, int32, ChallengeIndex, int32, CurrentProgress, int32, TotalProgress);

/** Coordinates the three challenge progression steps and the native story HUD. */
UCLASS(Blueprintable)
class ECHO7_API AChallengeProgressManager : public AActor
{
	GENERATED_BODY()

public:
	AChallengeProgressManager();

	/** Completes one unique challenge and registers it with the main reactor. */
	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	bool CompleteChallenge(int32 ChallengeIndex);

	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void SetObjective(const FText& NewObjective);

	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ShowStoryMessage(const FText& Message, float Duration);

	/** Displays author-configured story rows; each entry maps to one HUD row. */
	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ShowStoryMessageLines(const TArray<FText>& Lines, float Duration);

	/** Clears the temporary story/warning area without changing objective or recovery HUD. */
	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ClearStoryMessage();

	/** Displays a large centered tutorial text group through the native HUD. */
	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ShowTutorialLines(const TArray<FText>& Lines, float Duration, int32 FontSize);

	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ShowWarning(const FText& Message, float Duration);

	/** Displays author-configured warning rows; each entry maps to one HUD row. */
	UFUNCTION(BlueprintCallable, Category = "Challenge Progress")
	void ShowWarningLines(const TArray<FText>& Lines, float Duration);

	/** Starts the current level's completion timer once. */
	UFUNCTION(BlueprintCallable, Category = "Run Timer")
	void StartRunTimer();

	/** Freezes the completion timer once and preserves the final elapsed value. */
	UFUNCTION(BlueprintCallable, Category = "Run Timer")
	void StopRunTimer();

	/** Returns the live or frozen elapsed run duration in seconds. */
	UFUNCTION(BlueprintPure, Category = "Run Timer")
	double GetElapsedRunTime() const;

	/** Returns the live or frozen duration as MM:SS, or HH:MM:SS after one hour. */
	UFUNCTION(BlueprintPure, Category = "Run Timer")
	FText GetFormattedRunTime() const;

	UFUNCTION(BlueprintPure, Category = "Run Timer")
	bool IsRunTimerCompleted() const { return bRunTimerCompleted; }

	/** Broadcast after a unique challenge has been registered successfully. */
	UPROPERTY(BlueprintAssignable, Category = "Challenge Progress")
	FChallengeCompletedSignature OnChallengeCompleted;

	/** Main Reactor used as the three-step progression counter. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	TObjectPtr<AMainReactor> MainReactor;

	/** Current player-facing objective. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	FText CurrentObjective;

	/** Completed challenge count mirrored from the main reactor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	int32 CurrentCompletedChallengeCount = 0;

	/** Number of sequential challenges in this game. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Challenge Progress")
	int32 TotalChallengeCount = 3;

	/** The native HUD created for the local player. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	TObjectPtr<UECHO7HUDWidget> HUDWidget;

	/** World real time at which this run began. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Run Timer")
	double RunStartTime = 0.0;

	/** Frozen elapsed duration captured when the final puzzle was solved. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Run Timer")
	double FinalCompletionTime = 0.0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Run Timer")
	bool bRunTimerRunning = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Run Timer")
	bool bRunTimerCompleted = false;

	/** Intro messages shown sequentially when play begins. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Story")
	TArray<FText> StartingStoryMessages;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Story", meta = (ClampMin = "0.1", Units = "s"))
	float StartingMessageDuration = 2.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Story", meta = (ClampMin = "0.0", Units = "s"))
	float StartingMessageGap = 0.25f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateHUD();
	void UpdateHUD();
	void RefreshRunTimerHUD();
	void DisplayNextStartingMessage();
	void BindInteractionPrompt();

	UFUNCTION()
	void HandleFocusedInteractableChanged(AActor* PreviousInteractable, AActor* NewInteractable);

	TSet<int32> CompletedChallengeIndices;
	FTimerHandle StartingStoryTimer;
	FTimerHandle RunTimerRefreshTimer;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionComponent> BoundInteractionComponent;

	int32 StartingStoryMessageIndex = 0;
	bool bMissingReactorWarningLogged = false;
};
