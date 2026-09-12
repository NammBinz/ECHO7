// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StationPowerController.generated.h"

class ALight;
class AMainReactor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPowerRestorationStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPowerRestoredSignature);

/** Restores station power after the assigned main reactor activates. */
UCLASS(Blueprintable)
class ECHO7_API AStationPowerController : public AActor
{
	GENERATED_BODY()

public:
	AStationPowerController();

	UFUNCTION(BlueprintPure, Category = "Station Power")
	bool IsPowerRestored() const { return bPowerRestored; }

	/** Broadcast once when the reactor begins the power restoration sequence. */
	UPROPERTY(BlueprintAssignable, Category = "Station Power")
	FPowerRestorationStartedSignature OnPowerRestorationStarted;

	/** Broadcast once when normal station power is fully online. */
	UPROPERTY(BlueprintAssignable, Category = "Station Power")
	FPowerRestoredSignature OnPowerRestored;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Reactor that initiates this controller's restoration sequence. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	TObjectPtr<AMainReactor> MainReactor;

	/** Lights active while the station is on emergency power. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	TArray<TObjectPtr<ALight>> EmergencyLights;

	/** Lights enabled after normal station power is restored. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	TArray<TObjectPtr<ALight>> PoweredLights;

	/** Actors revealed when normal station power is restored. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	TArray<TObjectPtr<AActor>> ActorsToEnableOnPower;

	/** Actors hidden when normal station power is restored. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	TArray<TObjectPtr<AActor>> ActorsToDisableOnPower;

	/** True once normal station power has been restored. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Station Power")
	bool bPowerRestored = false;

private:
	UFUNCTION()
	void HandleReactorActivated();
	void StartPowerRestoration();
	void SetEmergencyLightsOn();
	void SetEmergencyLightsOff();
	void EnablePoweredLights();
	void CompletePowerRestoration();
	void SetLightsEnabled(const TArray<TObjectPtr<ALight>>& Lights, bool bEnabled) const;

	bool bPowerRestorationInProgress = false;
	FTimerHandle FirstEmergencyOffTimer;
	FTimerHandle FirstEmergencyOnTimer;
	FTimerHandle SecondEmergencyOffTimer;
	FTimerHandle SecondEmergencyOnTimer;
	FTimerHandle FinalEmergencyOffTimer;
	FTimerHandle PoweredLightsOnTimer;
	FTimerHandle CompletionTimer;
};
