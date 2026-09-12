// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

class AActor;

/** Interface implemented by actors that can be used by an interactor. */
UINTERFACE(BlueprintType)
class ECHO7_API UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class ECHO7_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/** Performs this actor's interaction behavior. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor) {}

	/** Returns the prompt shown while this actor is focused. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;
	virtual FText GetInteractionPrompt_Implementation() const
	{
		return NSLOCTEXT("Interaction", "DefaultInteractionPrompt", "Tương tác");
	}

	/** Returns whether this actor can currently be used by the supplied interactor. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const { return true; }
};
