// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/GuidanceTrigger.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "ECHO7.h"
#include "ECHO7Character.h"
#include "Game/ChallengeProgressManager.h"

AGuidanceTrigger::AGuidanceTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger Volume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetBoxExtent(FVector(150.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetHiddenInGame(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AGuidanceTrigger::HandleTriggerBeginOverlap);

	GuidanceText = NSLOCTEXT("ECHO7", "DefaultGuidanceText", "Hãy kiểm tra bảng điều khiển trước mặt. Nhìn vào bảng điều khiển và nhấn [E] để tương tác.");
}

void AGuidanceTrigger::ActivateGuidance()
{
	if ((bShowOnlyOnce && bHasBeenTriggered) || !ChallengeProgressManager || (GuidanceLines.IsEmpty() && GuidanceText.IsEmpty()))
	{
		return;
	}

	if (!GuidanceLines.IsEmpty())
	{
		ChallengeProgressManager->ShowStoryMessageLines(GuidanceLines, MessageDuration);
	}
	else
	{
		ChallengeProgressManager->ShowStoryMessage(GuidanceText, MessageDuration);
	}
	bHasBeenTriggered = true;
	OnGuidanceTriggered.Broadcast();
}

void AGuidanceTrigger::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!Cast<AECHO7Character>(OtherActor))
	{
		return;
	}

	ActivateGuidance();
}
