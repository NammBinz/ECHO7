// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "EnergyCore.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnergyCorePickedUpSignature);

/** An interactable Energy Core that can be carried by a player. */
UCLASS(Blueprintable)
class ECHO7_API AEnergyCore : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AEnergyCore();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Broadcast when this core is successfully picked up. */
	UPROPERTY(BlueprintAssignable, Category = "Energy Core")
	FEnergyCorePickedUpSignature OnCorePickedUp;

	/** True after this core has been picked up. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Energy Core")
	bool bIsPickedUp = false;
};
