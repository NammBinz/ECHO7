// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/SciFiDoor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ECHO7.h"
#include "Engine/Engine.h"
#include "Game/ChallengeProgressManager.h"
#include "Game/FrequencyGenerator.h"

namespace SciFiDoor
{
	const FText LockedDoorMessage = NSLOCTEXT("ECHO7", "SciFiDoorLockedMessage", "CỬA ĐANG BỊ KHÓA");
}

ASciFiDoor::ASciFiDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	CollisionComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	InteractionPrompt = NSLOCTEXT("ECHO7", "SciFiDoorInteractionPrompt", "Mở cửa");
}

void ASciFiDoor::BeginPlay()
{
	Super::BeginPlay();

	ClosedLocation = GetActorLocation();
	bIsLocked = bStartsLocked;
	SetActorTickEnabled(false);
}

void ASciFiDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsOpening)
	{
		SetActorTickEnabled(false);
		return;
	}

	const FVector OpenLocation = ClosedLocation + OpenOffset;
	const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), OpenLocation, DeltaTime, OpenSpeed);
	SetActorLocation(NewLocation);

	if (NewLocation.Equals(OpenLocation, 1.0f))
	{
		SetActorLocation(OpenLocation);
		FinishOpening();
	}
}

void ASciFiDoor::Interact_Implementation(AActor* Interactor)
{
	if (bIsOpen || bIsOpening)
	{
		return;
	}

	if (bIsLocked)
	{
		if (bLaunchFrequencyChallengeWhenLocked
			&& IsValid(LinkedFrequencyGenerator)
			&& !LinkedFrequencyGenerator->bStabilized
			&& LinkedFrequencyGenerator->IsStageEnabled()
			&& LinkedFrequencyGenerator->StartCalibration(Interactor))
		{
			return;
		}

		if (ChallengeProgressManager)
		{
			if (!LockedInteractionLines.IsEmpty())
			{
				ChallengeProgressManager->ShowWarningLines(LockedInteractionLines, 3.0f);
			}
			else
			{
				ChallengeProgressManager->ShowWarning(SciFiDoor::LockedDoorMessage, 3.0f);
			}

			if (!ObjectiveWhenLocked.IsEmpty())
			{
				ChallengeProgressManager->SetObjective(ObjectiveWhenLocked);
			}
		}

		FString DebugMessage = SciFiDoor::LockedDoorMessage.ToString();
		if (!LockedInteractionLines.IsEmpty())
		{
			DebugMessage.Reset();
			for (const FText& Line : LockedInteractionLines)
			{
				if (!DebugMessage.IsEmpty())
				{
					DebugMessage.AppendChar(TEXT('\n'));
				}
				DebugMessage.Append(Line.ToString());
			}
		}
		UE_LOG(LogECHO7, Display, TEXT("%s"), *DebugMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, DebugMessage);
		}
		return;
	}

	StartOpening();
}

FText ASciFiDoor::GetInteractionPrompt_Implementation() const
{
	if (bIsLocked
		&& bLaunchFrequencyChallengeWhenLocked
		&& IsValid(LinkedFrequencyGenerator)
		&& !LinkedFrequencyGenerator->bStabilized
		&& !LinkedFrequencyGenerator->bCalibrationOpen
		&& LinkedFrequencyGenerator->IsStageEnabled()
		&& !FrequencyChallengePrompt.IsEmpty())
	{
		return FrequencyChallengePrompt;
	}

	return bIsLocked && !LockedInteractionPrompt.IsEmpty()
		? LockedInteractionPrompt
		: Super::GetInteractionPrompt_Implementation();
}

bool ASciFiDoor::CanInteract_Implementation(AActor* Interactor) const
{
	return !bIsOpen && !bIsOpening;
}

void ASciFiDoor::UnlockDoor()
{
	if (!bIsLocked)
	{
		return;
	}

	bIsLocked = false;
	OnDoorUnlocked.Broadcast();
}

void ASciFiDoor::OpenDoor()
{
	if (!bIsLocked && !bIsOpen && !bIsOpening)
	{
		StartOpening();
	}
}

void ASciFiDoor::LockDoor()
{
	if (!bIsOpen)
	{
		bIsLocked = true;
	}
}

void ASciFiDoor::StartOpening()
{
	bIsOpening = true;
	SetActorTickEnabled(true);
}

void ASciFiDoor::FinishOpening()
{
	bIsOpening = false;
	bIsOpen = true;
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	OnDoorOpened.Broadcast();

	UE_LOG(LogECHO7, Display, TEXT("CỬA ĐÃ MỞ"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("CỬA ĐÃ MỞ"));
	}
}
