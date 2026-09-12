// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ECHO7PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class AECHO7EndingController;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class ECHO7_API AECHO7PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AECHO7PlayerController();

	/** Enables hidden ending-restart input routing for the active final ending only. */
	void SetActiveEndingRestartController(AECHO7EndingController* EndingController);

	/** Removes ending-restart routing without disturbing other controller input. */
	void ClearActiveEndingRestartController(const AECHO7EndingController* EndingController);

	/** Forwards the hidden final-screen restart key to the active ending. */
	void HandleEndingRestartKey();

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Fade from a fully black frame when a fresh gameplay world starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Startup", meta = (ClampMin = "0.01", Units = "s"))
	float StartupFadeDuration = 1.0f;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void TryStartStartupFade();

	FTimerHandle StartupFadeRetryTimer;
	TWeakObjectPtr<AECHO7EndingController> ActiveEndingRestartController;
	int32 StartupFadeAttemptCount = 0;
	bool bStartupFadeStarted = false;
};
