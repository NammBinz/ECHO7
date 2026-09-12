// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GuidanceTrigger.generated.h"

class AChallengeProgressManager;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGuidanceTriggeredSignature);

/** An invisible volume that displays a contextual gameplay hint to the player. */
UCLASS(Blueprintable)
class ECHO7_API AGuidanceTrigger : public AActor
{
	GENERATED_BODY()

public:
	AGuidanceTrigger();

	/** Displays this trigger's guidance message if it has not already been consumed. */
	UFUNCTION(BlueprintCallable, Category = "Guidance")
	void ActivateGuidance();

	/** Broadcast when this trigger successfully displays its guidance message. */
	UPROPERTY(BlueprintAssignable, Category = "Guidance")
	FGuidanceTriggeredSignature OnGuidanceTriggered;

	/** Manager that owns the player-facing story-message HUD. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guidance")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Text displayed when the player enters this trigger. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guidance", meta = (MultiLine = "true"))
	FText GuidanceText;

	/** Optional manual guidance rows. These take precedence over the legacy GuidanceText value. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guidance", meta = (MultiLine = "true"))
	TArray<FText> GuidanceLines;

	/** Number of seconds that the guidance message remains visible. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guidance", meta = (ClampMin = "0.1", Units = "s"))
	float MessageDuration = 5.0f;

	/** When true, this trigger cannot display its message more than once per play session. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guidance")
	bool bShowOnlyOnce = true;

	/** Trigger volume, resizable directly in the editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** Whether this trigger has successfully displayed its message during this play session. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Guidance")
	bool bHasBeenTriggered = false;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
};
