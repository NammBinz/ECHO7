// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/EvacuationZone.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "ECHO7Character.h"
#include "Game/ChallengeProgressManager.h"
#include "Game/FacilityBreachController.h"

namespace EvacuationZone
{
	const FText EvacuationArrivalMessage = NSLOCTEXT("ECHO7", "EvacuationArrivalMessage", "ĐÃ ĐẾN KHU VỰC SƠ TÁN");
}

AEvacuationZone::AEvacuationZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	EvacuationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Evacuation Volume"));
	EvacuationVolume->SetupAttachment(SceneRoot);
	EvacuationVolume->SetBoxExtent(FVector(150.0f));
	EvacuationVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EvacuationVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	EvacuationVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EvacuationVolume->SetGenerateOverlapEvents(true);
	EvacuationVolume->SetHiddenInGame(true);
	EvacuationVolume->OnComponentBeginOverlap.AddDynamic(this, &AEvacuationZone::HandleEvacuationVolumeBeginOverlap);
}

void AEvacuationZone::HandleEvacuationVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AECHO7Character* PlayerCharacter = Cast<AECHO7Character>(OtherActor);
	AChallengeProgressManager* ProgressManager = ChallengeProgressManager;
	if (!IsValid(ProgressManager) && IsValid(FacilityBreachController))
	{
		ProgressManager = FacilityBreachController->ChallengeProgressManager;
	}
	const bool bChallengesComplete = IsValid(ProgressManager)
		&& ProgressManager->TotalChallengeCount > 0
		&& ProgressManager->CurrentCompletedChallengeCount >= ProgressManager->TotalChallengeCount;
	if (!PlayerCharacter
		|| !PlayerCharacter->IsPlayerControlled()
		|| bEvacuationCompleted
		|| (bRequireActiveBreach && (!IsValid(FacilityBreachController) || !FacilityBreachController->IsBreachActive()))
		|| (bRequireAllChallengesComplete && !bChallengesComplete))
	{
		return;
	}

	CompleteEvacuation();
}

void AEvacuationZone::CompleteEvacuation()
{
	if (bEvacuationCompleted)
	{
		return;
	}

	bEvacuationCompleted = true;

	AChallengeProgressManager* StoryManager = ChallengeProgressManager;
	if (!IsValid(StoryManager) && IsValid(FacilityBreachController))
	{
		StoryManager = FacilityBreachController->ChallengeProgressManager;
	}

	if (IsValid(StoryManager))
	{
		StoryManager->ShowStoryMessage(EvacuationZone::EvacuationArrivalMessage, MessageDuration);
	}

	OnEvacuationCompleted.Broadcast();
}
