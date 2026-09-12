// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AccessTerminal.h"

#include "ECHO7.h"
#include "Engine/Engine.h"
#include "Environment/SciFiDoor.h"
#include "Game/ChallengeProgressManager.h"

AAccessTerminal::AAccessTerminal()
{
	InteractionPrompt = NSLOCTEXT("ECHO7", "AccessTerminalInteractionPrompt", "Khôi phục hệ thống truy cập");
	ActivationMessage = NSLOCTEXT("ECHO7", "AccessTerminalActivationMessage", "QUYỀN TRUY CẬP ĐÃ ĐƯỢC CẤP");
}

void AAccessTerminal::Interact_Implementation(AActor* Interactor)
{
	if (bUsed)
	{
		return;
	}

	bUsed = true;
	for (ASciFiDoor* Door : DoorsToUnlock)
	{
		if (IsValid(Door))
		{
			Door->UnlockDoor();
		}
	}

	if (ChallengeProgressManager)
	{
		if (bCompletesChallenge)
		{
			if (ChallengeIndex > 0
				&& ChallengeProgressManager->CompleteChallenge(ChallengeIndex)
				&& !ObjectiveAfterActivation.IsEmpty())
			{
				ChallengeProgressManager->SetObjective(ObjectiveAfterActivation);
			}
		}
		else if (!ObjectiveAfterActivation.IsEmpty())
		{
			ChallengeProgressManager->SetObjective(ObjectiveAfterActivation);
		}
	}

	OnTerminalActivated.Broadcast();

	if (!ActivationLines.IsEmpty() || !ActivationMessage.IsEmpty())
	{
		const FText& LogMessage = ActivationLines.IsEmpty() ? ActivationMessage : ActivationLines[0];
		UE_LOG(LogECHO7, Display, TEXT("%s"), *LogMessage.ToString());
		if (!bCompletesChallenge && ChallengeProgressManager)
		{
			if (!ActivationLines.IsEmpty())
			{
				ChallengeProgressManager->ShowStoryMessageLines(ActivationLines, 3.0f);
			}
			else
			{
				ChallengeProgressManager->ShowStoryMessage(ActivationMessage, 3.0f);
			}
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, LogMessage.ToString());
		}
	}
}

bool AAccessTerminal::CanInteract_Implementation(AActor* Interactor) const
{
	return !bUsed;
}
