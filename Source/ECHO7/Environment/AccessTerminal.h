// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/BaseInteractableActor.h"
#include "AccessTerminal.generated.h"

class ASciFiDoor;
class AChallengeProgressManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTerminalActivatedSignature);

/** An interactable terminal that unlocks its assigned doors once. */
UCLASS(Blueprintable)
class ECHO7_API AAccessTerminal : public ABaseInteractableActor
{
	GENERATED_BODY()

public:
	AAccessTerminal();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** Doors unlocked when this terminal is activated. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Terminal")
	TArray<TObjectPtr<ASciFiDoor>> DoorsToUnlock;

	/** Optional challenge manager notified by this terminal's first activation. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Whether this terminal completes its configured challenge when activated. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress")
	bool bCompletesChallenge = true;

	/** Optional challenge index completed when this terminal is first used. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ChallengeIndex = 0;

	/** Optional player-facing message shown when this terminal is activated. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Terminal", meta = (MultiLine = "true"))
	FText ActivationMessage;

	/** Optional manual activation rows. These take precedence over ActivationMessage. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Terminal", meta = (MultiLine = "true"))
	TArray<FText> ActivationLines;

	/** Optional objective set after successfully completing the assigned challenge. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge Progress", meta = (MultiLine = "true"))
	FText ObjectiveAfterActivation;

	/** Broadcast when this terminal is activated for the first time. */
	UPROPERTY(BlueprintAssignable, Category = "Terminal")
	FTerminalActivatedSignature OnTerminalActivated;

	/** True after this terminal has been activated. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Terminal")
	bool bUsed = false;
};
