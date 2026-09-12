// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "SciFiDoor.generated.h"

class AChallengeProgressManager;
class AFrequencyGenerator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDoorUnlockedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDoorOpenedSignature);

/** A simple interactable door that opens after being unlocked. */
UCLASS(Blueprintable)
class ECHO7_API ASciFiDoor : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	ASciFiDoor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Unlocks this door if it is currently locked. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void UnlockDoor();

	/** Starts the existing opening flow when this door is already unlocked. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void OpenDoor();

	/** Locks this door if it is not open. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void LockDoor();

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsLocked() const { return bIsLocked; }

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpen() const { return bIsOpen; }

	/** Broadcast when this door is unlocked. */
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FDoorUnlockedSignature OnDoorUnlocked;

	/** Broadcast when this door has finished opening. */
	UPROPERTY(BlueprintAssignable, Category = "Door")
	FDoorOpenedSignature OnDoorOpened;

	/** Optional manager used for HUD guidance and objective updates while this door is locked. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door Guidance")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Optional objective applied after the player checks this locked door. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door Guidance")
	FText ObjectiveWhenLocked;

	/** Optional manual locked-door guidance rows. Each entry is one HUD row. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door Guidance")
	TArray<FText> LockedInteractionLines;

	/** Semantic prompt shown while the door remains locked; the HUD supplies the [E] prefix. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door Guidance")
	FText LockedInteractionPrompt = FText::FromString(TEXT("Kiểm tra cửa"));

	/** Generator whose existing calibration sequence this locked door can launch. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door|Frequency Challenge")
	TObjectPtr<AFrequencyGenerator> LinkedFrequencyGenerator;

	/** Enables direct launch of LinkedFrequencyGenerator while this door is locked. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door|Frequency Challenge")
	bool bLaunchFrequencyChallengeWhenLocked = false;

	/** Semantic locked prompt used when the linked generator is available. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Locked Door|Frequency Challenge")
	FText FrequencyChallengePrompt = FText::FromString(TEXT("Khôi phục nguồn cửa"));

protected:
	/** Whether this door starts locked when play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bStartsLocked = true;

	/** Current locked state. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door")
	bool bIsLocked = true;

	/** True after the door has fully opened. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door")
	bool bIsOpen = false;

	/** Offset from the closed position to the open position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (Units = "cm"))
	FVector OpenOffset = FVector(0.0f, 0.0f, 250.0f);

	/** Opening speed in centimeters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0.0", Units = "cm/s"))
	float OpenSpeed = 200.0f;

private:
	void StartOpening();
	void FinishOpening();

	FVector ClosedLocation;
	bool bIsOpening = false;
};
