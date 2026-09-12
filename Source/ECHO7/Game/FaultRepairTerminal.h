// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Game/ECHO7Typewriter.h"
#include "Interaction/BaseInteractableActor.h"
#include "FaultRepairTerminal.generated.h"

class AChallengeProgressManager;
class AEvacuationZone;
class AAIMovementTestController;
class APlayerController;
class UECHO7FaultPuzzleWidget;
class UECHO7StoryOverlayWidget;
class UTexture2D;

/**
 * Fault-area terminal that gates Challenge 3 completion behind a native 3x3
 * data-reconstruction puzzle.
 */
UCLASS(Blueprintable)
class ECHO7_API AFaultRepairTerminal : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AFaultRepairTerminal();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Global progression owner notified only after the image puzzle is solved. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** The scan corridor that must be safely cleared before this terminal can be used. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AAIMovementTestController> MovementTestController;

	/** Existing evacuation/return-zone actor used by the configured ending controller after repair. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Ending")
	TObjectPtr<AEvacuationZone> ReturnToControlRoomZone;

	/** One texture split into a native 3x3 UV-cropped image puzzle. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	TObjectPtr<UTexture2D> PuzzleSourceTexture;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle", meta = (ClampMin = "0.0"))
	float TileGap = 4.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	bool bShuffleOnOpen = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Puzzle")
	bool bPreserveProgressWhenClosed = true;

	/** Manual story rows displayed after the puzzle completes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story")
	TArray<FText> PostRepairStoryLines;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story", meta = (ClampMin = "12"))
	int32 PostRepairStoryFontSize = 52;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story", meta = (ClampMin = "1.0"))
	float PostRepairCharactersPerSecond = 32.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story", meta = (ClampMin = "0.0", Units = "s"))
	float PostRepairLinePause = 0.35f;

	/** Read time after the full post-repair story becomes visible. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story", meta = (ClampMin = "0.0", Units = "s"))
	float PostRepairReadDuration = 5.0f;

	/** Objective shown once the post-repair transition returns control to the player. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Post Repair Story", meta = (MultiLine = "true"))
	FText ObjectiveAfterRepair;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.01", Units = "s"))
	float GlitchDuration = 0.30f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	bool bSolved = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	bool bPuzzleOpen = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void BeginPuzzleOpenTransition(APlayerController* PlayerController);
	void OpenPuzzleAfterGlitch();
	void BeginPuzzleCloseTransition();
	void FinishPuzzleCloseTransition();
	void ClosePuzzleWidget(bool bPreserveProgress);
	void BeginPostRepairStory();
	void BeginNextPostRepairLine();
	void UpdatePostRepairTypewriter();
	void CompleteCurrentPostRepairLine();
	void RefreshPostRepairStory();
	void BeginReturnTransition();
	void FinishPostRepairStory();
	bool PrepareStoryOverlay(APlayerController* PlayerController);
	void CapturePlayerInput(APlayerController* PlayerController, bool bForPuzzle);
	/** Restores the single gameplay-input lock owned by this terminal. Safe to call repeatedly. */
	void RestoreGameplayInput();
	APlayerController* GetLocalPlayerController() const;
	void ClearTerminalTimers();
	void ShowWarning(const FText& Message) const;

	UFUNCTION()
	void HandlePuzzleSolved();

	UFUNCTION()
	void HandlePuzzleCancelled();

	UFUNCTION()
	void HandleScanCorridorCompleted();

	UPROPERTY(Transient)
	TObjectPtr<UECHO7FaultPuzzleWidget> ActivePuzzleWidget;

	UPROPERTY(Transient)
	TObjectPtr<UECHO7StoryOverlayWidget> StoryOverlayWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ActivePlayerController;

	FTimerHandle PuzzleTransitionTimer;
	FTimerHandle StoryTransitionTimer;
	FTimerHandle StoryTypewriterTimer;
	FTimerHandle StoryLinePauseTimer;
	FTimerHandle StoryReadTimer;
	TArray<int32> SavedTileOrder;
	TArray<FText> VisiblePostRepairLines;
	FECHO7TypewriterState PostRepairTypewriter;
	int32 NextPostRepairLineIndex = 0;
	float PostRepairLineStartTime = 0.0f;
	bool bInputCaptured = false;
	bool bPostRepairStoryActive = false;
};
