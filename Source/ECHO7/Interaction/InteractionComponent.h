// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FFocusedInteractableChangedSignature,
	AActor*, PreviousInteractable,
	AActor*, NewInteractable);

/** Finds and activates interactable actors in front of its owning player. */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class ECHO7_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** Attempts to interact with the currently focused actor. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact();

	/** Returns the interactable actor currently under the player's crosshair. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedInteractable() const { return FocusedInteractable; }

	/** Broadcast whenever the focused interactable changes. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FFocusedInteractableChangedSignature OnFocusedInteractableChanged;

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Maximum distance, in centimeters, at which an actor can be interacted with. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float InteractionDistance = 300.0f;

	/** Interactable actor currently under the player's crosshair. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> FocusedInteractable;

private:
	void UpdateFocusedInteractable();
	void SetFocusedInteractable(AActor* NewFocusedInteractable);
	bool GetInteractionViewPoint(FVector& ViewLocation, FRotator& ViewRotation) const;
};
