// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/FrequencySyncController.h"

#include "ECHO7.h"
#include "Components/SceneComponent.h"
#include "Game/ChallengeProgressManager.h"
#include "Game/FrequencyGenerator.h"

AFrequencySyncController::AFrequencySyncController()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	ObjectiveWhileActive = NSLOCTEXT("ECHO7", "FrequencySyncObjectiveWhileActive", "Ổn định các máy phát năng lượng.");
	ObjectiveAfterCompletion = NSLOCTEXT("ECHO7", "FrequencySyncObjectiveAfterCompletion", "Đi tới khu thử nghiệm chuyển động AI.");
	StageObjectives = {
		NSLOCTEXT("ECHO7", "FrequencyStageObjectiveA", "Ổn định trạm năng lượng A."),
		NSLOCTEXT("ECHO7", "FrequencyStageObjectiveB", "Tiếp tục tới trạm năng lượng B."),
		NSLOCTEXT("ECHO7", "FrequencyStageObjectiveC", "Ổn định trạm năng lượng C.")
	};
}

void AFrequencySyncController::BeginPlay()
{
	Super::BeginPlay();
	InitializeStageState();
}

void AFrequencySyncController::NotifyGeneratorStabilized(AFrequencyGenerator* Generator)
{
	if (bAllGeneratorsStabilized || !IsConfiguredGenerator(Generator))
	{
		return;
	}
	if (bRequireSequentialOrder && GetNextGeneratorToStabilize() != Generator)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencySyncController ignored an out-of-order generator stabilization."));
		return;
	}

	const TWeakObjectPtr<AFrequencyGenerator> WeakGenerator(Generator);
	if (StabilizedGenerators.Contains(WeakGenerator))
	{
		return;
	}

	StabilizedGenerators.Add(WeakGenerator);

	const int32 TotalGeneratorCount = GetConfiguredGeneratorCount();
	const int32 StabilizedGeneratorCount = GetStabilizedGeneratorCount();
	if (TotalGeneratorCount <= 0)
	{
		return;
	}

	OnGeneratorProgressChanged.Broadcast(StabilizedGeneratorCount, TotalGeneratorCount);
	ApplyStageEnabledState();
	UpdateStageObjective();

	if (StabilizedGeneratorCount < TotalGeneratorCount)
	{
		ShowStageCompletionMessage(Generator, StabilizedGeneratorCount, TotalGeneratorCount);
		return;
	}

	bAllGeneratorsStabilized = true;
	OnAllGeneratorsStabilized.Broadcast();
	CompleteConfiguredChallenge();
	ShowStageCompletionMessage(Generator, StabilizedGeneratorCount, TotalGeneratorCount);
}

AFrequencyGenerator* AFrequencySyncController::GetNextGeneratorToStabilize() const
{
	TSet<const AFrequencyGenerator*> SeenGenerators;
	for (const TObjectPtr<AFrequencyGenerator>& Generator : Generators)
	{
		AFrequencyGenerator* GeneratorPtr = Generator.Get();
		if (!IsValid(GeneratorPtr) || SeenGenerators.Contains(GeneratorPtr))
		{
			continue;
		}
		SeenGenerators.Add(GeneratorPtr);
		if (!StabilizedGenerators.Contains(TWeakObjectPtr<AFrequencyGenerator>(GeneratorPtr)))
		{
			return GeneratorPtr;
		}
	}

	return nullptr;
}

void AFrequencySyncController::InitializeStageState()
{
	StabilizedGenerators.Reset();
	TSet<const AFrequencyGenerator*> SeenGenerators;
	for (const TObjectPtr<AFrequencyGenerator>& Generator : Generators)
	{
		AFrequencyGenerator* GeneratorPtr = Generator.Get();
		if (!IsValid(GeneratorPtr) || SeenGenerators.Contains(GeneratorPtr))
		{
			continue;
		}
		SeenGenerators.Add(GeneratorPtr);
		if (GeneratorPtr->bStabilized)
		{
			StabilizedGenerators.Add(TWeakObjectPtr<AFrequencyGenerator>(GeneratorPtr));
		}
	}

	bAllGeneratorsStabilized = GetConfiguredGeneratorCount() > 0
		&& GetStabilizedGeneratorCount() >= GetConfiguredGeneratorCount();
	ApplyStageEnabledState();
	if (!bAllGeneratorsStabilized)
	{
		UpdateStageObjective();
	}
}

void AFrequencySyncController::ApplyStageEnabledState()
{
	AFrequencyGenerator* NextGenerator = bRequireSequentialOrder ? GetNextGeneratorToStabilize() : nullptr;
	TSet<AFrequencyGenerator*> SeenGenerators;
	for (const TObjectPtr<AFrequencyGenerator>& Generator : Generators)
	{
		AFrequencyGenerator* GeneratorPtr = Generator.Get();
		if (!IsValid(GeneratorPtr) || SeenGenerators.Contains(GeneratorPtr))
		{
			continue;
		}
		SeenGenerators.Add(GeneratorPtr);
		GeneratorPtr->SetStageEnabled(!GeneratorPtr->bStabilized && (!bRequireSequentialOrder || GeneratorPtr == NextGenerator));
	}
}

void AFrequencySyncController::UpdateStageObjective() const
{
	if (!ChallengeProgressManager)
	{
		return;
	}

	const int32 NextStageIndex = GetStabilizedGeneratorCount();
	if (StageObjectives.IsValidIndex(NextStageIndex) && !StageObjectives[NextStageIndex].IsEmpty())
	{
		ChallengeProgressManager->SetObjective(StageObjectives[NextStageIndex]);
	}
	else if (NextStageIndex == 0 && !ObjectiveWhileActive.IsEmpty())
	{
		ChallengeProgressManager->SetObjective(ObjectiveWhileActive);
	}
}

int32 AFrequencySyncController::GetConfiguredGeneratorCount() const
{
	TSet<const AFrequencyGenerator*> UniqueGenerators;

	for (const TObjectPtr<AFrequencyGenerator>& Generator : Generators)
	{
		if (IsValid(Generator))
		{
			UniqueGenerators.Add(Generator.Get());
		}
	}

	return UniqueGenerators.Num();
}

int32 AFrequencySyncController::GetStabilizedGeneratorCount() const
{
	TSet<const AFrequencyGenerator*> UniqueGenerators;
	int32 StabilizedGeneratorCount = 0;

	for (const TObjectPtr<AFrequencyGenerator>& Generator : Generators)
	{
		const AFrequencyGenerator* GeneratorPtr = Generator.Get();
		if (!IsValid(GeneratorPtr) || UniqueGenerators.Contains(GeneratorPtr))
		{
			continue;
		}

		UniqueGenerators.Add(GeneratorPtr);
		if (StabilizedGenerators.Contains(TWeakObjectPtr<AFrequencyGenerator>(Generator.Get())))
		{
			++StabilizedGeneratorCount;
		}
	}

	return StabilizedGeneratorCount;
}

bool AFrequencySyncController::IsConfiguredGenerator(const AFrequencyGenerator* Generator) const
{
	if (!IsValid(Generator))
	{
		return false;
	}

	for (const TObjectPtr<AFrequencyGenerator>& ConfiguredGenerator : Generators)
	{
		if (ConfiguredGenerator.Get() == Generator && IsValid(ConfiguredGenerator))
		{
			return true;
		}
	}

	return false;
}

void AFrequencySyncController::ShowStageCompletionMessage(const AFrequencyGenerator* Generator, int32 StabilizedGeneratorCount, int32 TotalGeneratorCount) const
{
	if (!ChallengeProgressManager)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencySyncController has no ChallengeProgressManager assigned."));
		return;
	}

	if (Generator && !Generator->StageCompletionLines.IsEmpty())
	{
		ChallengeProgressManager->ShowStoryMessageLines(Generator->StageCompletionLines, 3.0f);
		return;
	}

	const FText GeneratorName = Generator && !Generator->GeneratorDisplayName.IsEmpty()
		? Generator->GeneratorDisplayName
		: FText::Format(NSLOCTEXT("ECHO7", "FrequencyStationFallbackName", "TRẠM NĂNG LƯỢNG {0}"), FText::AsNumber(StabilizedGeneratorCount));
	const FText RouteStatus = StabilizedGeneratorCount >= TotalGeneratorCount
		? NSLOCTEXT("ECHO7", "FrequencyRouteCompleted", "TUYẾN NĂNG LƯỢNG ĐÃ KHÔI PHỤC HOÀN TOÀN")
		: FText::Format(
			NSLOCTEXT("ECHO7", "FrequencyRouteProgress", "TUYẾN TRUY CẬP {0}/{1} ĐÃ KHÔI PHỤC"),
			FText::AsNumber(StabilizedGeneratorCount),
			FText::AsNumber(TotalGeneratorCount));
	ChallengeProgressManager->ShowStoryMessageLines({
		FText::Format(NSLOCTEXT("ECHO7", "FrequencyStationStable", "{0} ĐÃ ỔN ĐỊNH"), GeneratorName),
		RouteStatus
	}, 3.0f);
}

void AFrequencySyncController::CompleteConfiguredChallenge()
{
	if (bChallengeCompletionAttempted)
	{
		return;
	}

	bChallengeCompletionAttempted = true;

	if (!ChallengeProgressManager)
	{
		UE_LOG(LogECHO7, Warning, TEXT("FrequencySyncController completed without a ChallengeProgressManager."));
		return;
	}

	const bool bChallengeCompleted = ChallengeProgressManager->CompleteChallenge(ChallengeIndex);
	if (bChallengeCompleted && !ObjectiveAfterCompletion.IsEmpty())
	{
		ChallengeProgressManager->SetObjective(ObjectiveAfterCompletion);
	}

}
