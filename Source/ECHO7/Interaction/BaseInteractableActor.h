// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "BaseInteractableActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Basic native interactable actor used to test the interaction system. */
UCLASS(Blueprintable)
class ECHO7_API ABaseInteractableActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABaseInteractableActor();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

protected:
	/** Traceable collision volume for this interactable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComponent;

	/** Optional visible representation of this interactable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/** Text that can be displayed when this actor is focused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionPrompt;
};
