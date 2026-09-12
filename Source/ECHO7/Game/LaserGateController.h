// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "LaserGateController.generated.h"

class AFrequencyGenerator;
class UBoxComponent;
class UECHO7LaserGateGlitchWidget;

/** One placeable Stage 2 security gate: interaction, collision, success glitch, and visual retraction. */
UCLASS()
class ECHO7_API ALaserGateController : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	ALaserGateController();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Starts the one-time success transition. Safe to invoke repeatedly. */
	UFUNCTION(BlueprintCallable, Category = "Laser Gate")
	void OpenGate();

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Laser Gate")
	TObjectPtr<AFrequencyGenerator> LinkedFrequencyGenerator;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Laser Gate")
	TObjectPtr<AActor> LaserVisualActor;

	/** Actual physical wall, independent of the LED/laser visual actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BlockingBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate")
	FVector OpenMoveOffset = FVector(0.0f, 0.0f, -400.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate", meta = (ClampMin = "0.01", Units = "s"))
	float MoveDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate")
	bool bDisableCollisionWhenOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate")
	bool bHideVisualWhenOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate|Glitch", meta = (ClampMin = "0.01", Units = "s"))
	float SuccessGlitchDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate|Glitch", meta = (ClampMin = "0.0", Units = "s"))
	float WallMoveStartDelay = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate|Debug")
	bool bTestOpenOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Laser Gate|Debug", meta = (EditCondition = "bTestOpenOnBeginPlay", ClampMin = "0.0", Units = "s"))
	float TestOpenDelay = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Laser Gate")
	bool bOpened = false;

private:
	UFUNCTION()
	void HandleGeneratorStabilized();

	void BeginVisualMovement();
	void FinishOpening();
	void PlaySuccessGlitch();
	void PrepareVisualForMovement();
	void LogCanInteractStateIfChanged(bool bCanInteract, bool bGeneratorValid, bool bStageEnabled, bool bGeneratorStabilized, bool bCalibrationActive) const;

	UPROPERTY(Transient)
	TObjectPtr<UECHO7LaserGateGlitchWidget> ActiveGlitchWidget;

	FTimerHandle TestOpenTimer;
	FTimerHandle WallMoveStartTimer;
	FVector VisualStartLocation = FVector::ZeroVector;
	FVector VisualTargetLocation = FVector::ZeroVector;
	float MovementElapsedTime = 0.0f;
	bool bOpening = false;
	bool bVisualMoving = false;
	mutable bool bHasLoggedCanInteractState = false;
	mutable bool bLastCanInteractState = false;
};
