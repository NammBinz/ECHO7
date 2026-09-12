// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FrequencySyncController.generated.h"

class AChallengeProgressManager;
class AFrequencyGenerator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrequencyGeneratorProgressChangedSignature, int32, StabilizedGeneratorCount, int32, TotalGeneratorCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrequencyAllGeneratorsStabilizedSignature);

/** Coordinates the configured frequency generators that comprise Challenge 2. */
UCLASS(Blueprintable)
class ECHO7_API AFrequencySyncController : public AActor
{
	GENERATED_BODY()

public:
	AFrequencySyncController();

	/** Registers a configured generator after it has been stabilized. Duplicate and unconfigured generators are ignored. */
	UFUNCTION(BlueprintCallable, Category = "Frequency Sync")
	void NotifyGeneratorStabilized(AFrequencyGenerator* Generator);

	UFUNCTION(BlueprintPure, Category = "Frequency Sync")
	int32 GetConfiguredGeneratorCount() const;

	UFUNCTION(BlueprintPure, Category = "Frequency Sync")
	int32 GetStabilizedGeneratorCount() const;

	/** Broadcast whenever a unique configured generator becomes stabilized. */
	UPROPERTY(BlueprintAssignable, Category = "Frequency Sync")
	FFrequencyGeneratorProgressChangedSignature OnGeneratorProgressChanged;

	/** Broadcast once after every valid, unique configured generator is stabilized. */
	UPROPERTY(BlueprintAssignable, Category = "Frequency Sync")
	FFrequencyAllGeneratorsStabilizedSignature OnAllGeneratorsStabilized;

	/** Generator stations participating in this challenge. Null and duplicate entries are ignored. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Frequency Sync")
	TArray<TObjectPtr<AFrequencyGenerator>> Generators;

	/** When enabled, the order of Generators defines the required station sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frequency Sync|Progression")
	bool bRequireSequentialOrder = true;

	/** Manager that owns the global three-challenge recovery progression. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge")
	TObjectPtr<AChallengeProgressManager> ChallengeProgressManager;

	/** Challenge index reported to the progression manager once synchronization completes. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (ClampMin = "1", ClampMax = "3"))
	int32 ChallengeIndex = 2;

	/** Objective used while the player is stabilizing the configured generators. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveWhileActive;

	/** Optional objectives for each configured stage. Missing entries fall back safely. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	TArray<FText> StageObjectives;

	/** Objective set after all configured generators have been stabilized. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Challenge", meta = (MultiLine = "true"))
	FText ObjectiveAfterCompletion;

	/** True after all valid, unique configured generators have been stabilized. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Frequency Sync")
	bool bAllGeneratorsStabilized = false;

protected:
	virtual void BeginPlay() override;

private:
	bool IsConfiguredGenerator(const AFrequencyGenerator* Generator) const;
	AFrequencyGenerator* GetNextGeneratorToStabilize() const;
	void InitializeStageState();
	void ApplyStageEnabledState();
	void UpdateStageObjective() const;
	void ShowStageCompletionMessage(const AFrequencyGenerator* Generator, int32 StabilizedGeneratorCount, int32 TotalGeneratorCount) const;
	void CompleteConfiguredChallenge();

	TSet<TWeakObjectPtr<AFrequencyGenerator>> StabilizedGenerators;
	bool bChallengeCompletionAttempted = false;
};
