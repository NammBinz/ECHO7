// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EvacuationZone.generated.h"

class AChallengeProgressManager;
class AFacilityBreachController;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEvacuationCompletedSignature);

/**
 * An invisible destination volume that completes the evacuation portion of an
 * active facility-breach sequence.
 */
UCLASS(Blueprintable)
class ECHO7_API AEvacuationZone : public AActor
{
	GENERATED_BODY()

public:
	AEvacuationZone();

	/** Broadcast once when the active test player reaches this zone after the breach begins. */
	UPROPERTY(BlueprintAssignable, Category = "Evacuation")
	FEvacuationCompletedSignature OnEvacuationCompleted;

	/** Controller whose active breach state permits this zone to complete. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Evacuation")
	TObjectPtr<AFacilityBreachController> FacilityBreachController;

	/** Manager used to display the player-facing evacuation arrival message. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Evacuation")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Number of seconds that the evacuation arrival message remains visible. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Evacuation", meta = (ClampMin = "0.1", Units = "s"))
	float MessageDuration = 3.0f;

	/** Keep enabled for legacy breach evacuation zones. Disable for the Challenge 3 control-room return zone. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Evacuation|Requirements")
	bool bRequireActiveBreach = true;

	/** Optional guard for a return zone that must activate only after all recovery challenges are complete. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Evacuation|Requirements")
	bool bRequireAllChallengesComplete = false;

	/** Trigger volume, visible and resizable in the editor but hidden during play. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EvacuationVolume;

	/** Whether evacuation has already completed during this play session. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Evacuation")
	bool bEvacuationCompleted = false;

private:
	UFUNCTION()
	void HandleEvacuationVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void CompleteEvacuation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
};
