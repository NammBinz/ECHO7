// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "ReactorCoreSocket.generated.h"

class AMainReactor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReactorCoreInsertedSignature);

/** An interactable socket that accepts one Energy Core. */
UCLASS(Blueprintable)
class ECHO7_API AReactorCoreSocket : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AReactorCoreSocket();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Main reactor notified after a successful core insertion. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Reactor")
	TObjectPtr<AMainReactor> MainReactor;

	/** Designer-facing identifier for this socket. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Reactor")
	int32 SocketIndex = 0;

	/** Broadcast when an Energy Core is installed in this socket. */
	UPROPERTY(BlueprintAssignable, Category = "Reactor")
	FReactorCoreInsertedSignature OnCoreInserted;

	/** True once an Energy Core has been installed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reactor")
	bool bIsOccupied = false;
};
