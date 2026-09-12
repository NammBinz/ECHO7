// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/LaserGateController.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ECHO7.h"
#include "Game/FrequencyGenerator.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/ECHO7LaserGateGlitchWidget.h"

ALaserGateController::ALaserGateController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	InteractionPrompt = NSLOCTEXT("ECHO7", "LaserGateInteractionPrompt", "Khôi phục lưới an ninh");

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	StaticMeshComponent->SetVisibility(false);

	BlockingBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Blocking Box"));
	BlockingBox->SetupAttachment(CollisionComponent);
	BlockingBox->InitBoxExtent(FVector(120.0f, 20.0f, 180.0f));
	BlockingBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ALaserGateController::BeginPlay()
{
	Super::BeginPlay();
	if (LinkedFrequencyGenerator)
	{
		LinkedFrequencyGenerator->OnGeneratorStabilized.AddUniqueDynamic(this, &ALaserGateController::HandleGeneratorStabilized);
	}
	if (bTestOpenOnBeginPlay)
	{
		GetWorldTimerManager().SetTimer(TestOpenTimer, this, &ALaserGateController::OpenGate, FMath::Max(0.0f, TestOpenDelay), false);
	}
}

void ALaserGateController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TestOpenTimer);
	GetWorldTimerManager().ClearTimer(WallMoveStartTimer);
	if (LinkedFrequencyGenerator)
	{
		LinkedFrequencyGenerator->OnGeneratorStabilized.RemoveDynamic(this, &ALaserGateController::HandleGeneratorStabilized);
	}
	if (ActiveGlitchWidget)
	{
		ActiveGlitchWidget->RemoveFromParent();
		ActiveGlitchWidget = nullptr;
	}
	SetActorTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

void ALaserGateController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bVisualMoving)
	{
		SetActorTickEnabled(false);
		return;
	}
	if (!IsValid(LaserVisualActor))
	{
		FinishOpening();
		return;
	}

	MovementElapsedTime += FMath::Max(0.0f, DeltaSeconds);
	const float LinearAlpha = FMath::Clamp(MovementElapsedTime / FMath::Max(0.01f, MoveDuration), 0.0f, 1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - 2.0f * LinearAlpha);
	LaserVisualActor->SetActorLocation(FMath::Lerp(VisualStartLocation, VisualTargetLocation, SmoothAlpha));
	if (LinearAlpha >= 1.0f)
	{
		LaserVisualActor->SetActorLocation(VisualTargetLocation);
		FinishOpening();
	}
}

void ALaserGateController::Interact_Implementation(AActor* Interactor)
{
	const bool bGeneratorValid = IsValid(LinkedFrequencyGenerator);
	const bool bStageEnabled = bGeneratorValid && LinkedFrequencyGenerator->IsStageEnabled();
	const bool bGeneratorStabilized = bGeneratorValid && LinkedFrequencyGenerator->bStabilized;
	const bool bCalibrationActive = bGeneratorValid && LinkedFrequencyGenerator->bCalibrationOpen;
	const bool bCanInteract = CanInteract_Implementation(Interactor);
	UE_LOG(LogECHO7, Display, TEXT("LASER_GATE_DIAG Interact attempt: CanInteract=%s LinkedGenerator=%s StageEnabled=%s Stabilized=%s CalibrationActive=%s"),
		bCanInteract ? TEXT("true") : TEXT("false"),
		bGeneratorValid ? TEXT("true") : TEXT("false"),
		bStageEnabled ? TEXT("true") : TEXT("false"),
		bGeneratorStabilized ? TEXT("true") : TEXT("false"),
		bCalibrationActive ? TEXT("true") : TEXT("false"));

	if (!bCanInteract)
	{
		return;
	}

	if (bGeneratorValid)
	{
		const bool bStartedCalibration = LinkedFrequencyGenerator->StartCalibration(Interactor);
		UE_LOG(LogECHO7, Display, TEXT("LASER_GATE_DIAG StartCalibration result: %s"), bStartedCalibration ? TEXT("true") : TEXT("false"));
	}
}

bool ALaserGateController::CanInteract_Implementation(AActor* Interactor) const
{
	const bool bGeneratorValid = IsValid(LinkedFrequencyGenerator);
	const bool bStageEnabled = bGeneratorValid && LinkedFrequencyGenerator->IsStageEnabled();
	const bool bGeneratorStabilized = bGeneratorValid && LinkedFrequencyGenerator->bStabilized;
	const bool bCalibrationActive = bGeneratorValid && LinkedFrequencyGenerator->bCalibrationOpen;
	const bool bCanInteract = !bOpening && !bOpened && bGeneratorValid && bStageEnabled
		&& !bGeneratorStabilized && !bCalibrationActive
		&& LinkedFrequencyGenerator->CanInteract_Implementation(Interactor);
	LogCanInteractStateIfChanged(bCanInteract, bGeneratorValid, bStageEnabled, bGeneratorStabilized, bCalibrationActive);
	return bCanInteract;
}

void ALaserGateController::OpenGate()
{
	if (bOpening || bOpened)
	{
		return;
	}
	bOpening = true;
	PlaySuccessGlitch();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WallMoveStartTimer, this, &ALaserGateController::BeginVisualMovement, FMath::Max(0.0f, WallMoveStartDelay), false);
	}
	else
	{
		BeginVisualMovement();
	}
}

void ALaserGateController::HandleGeneratorStabilized()
{
	OpenGate();
}

void ALaserGateController::BeginVisualMovement()
{
	if (!bOpening || bOpened)
	{
		return;
	}
	if (!IsValid(LaserVisualActor))
	{
		FinishOpening();
		return;
	}

	PrepareVisualForMovement();
	VisualStartLocation = LaserVisualActor->GetActorLocation();
	VisualTargetLocation = VisualStartLocation + OpenMoveOffset;
	MovementElapsedTime = 0.0f;
	bVisualMoving = true;
	SetActorTickEnabled(true);
}

void ALaserGateController::FinishOpening()
{
	bVisualMoving = false;
	bOpening = false;
	bOpened = true;
	SetActorTickEnabled(false);
	if (bDisableCollisionWhenOpen)
	{
		BlockingBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (IsValid(LaserVisualActor))
		{
			LaserVisualActor->SetActorEnableCollision(false);
		}
	}
	if (bHideVisualWhenOpen && IsValid(LaserVisualActor))
	{
		LaserVisualActor->SetActorHiddenInGame(true);
	}
}

void ALaserGateController::PlaySuccessGlitch()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}
	ActiveGlitchWidget = CreateWidget<UECHO7LaserGateGlitchWidget>(PlayerController, UECHO7LaserGateGlitchWidget::StaticClass());
	if (ActiveGlitchWidget && ActiveGlitchWidget->AddToPlayerScreen(100))
	{
		ActiveGlitchWidget->PlayGlitch(SuccessGlitchDuration);
	}
	else
	{
		ActiveGlitchWidget = nullptr;
	}
}

void ALaserGateController::PrepareVisualForMovement()
{
	if (!IsValid(LaserVisualActor))
	{
		return;
	}
	if (USceneComponent* VisualRootComponent = LaserVisualActor->GetRootComponent())
	{
		if (VisualRootComponent->Mobility != EComponentMobility::Movable)
		{
			VisualRootComponent->SetMobility(EComponentMobility::Movable);
		}
	}
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(LaserVisualActor);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent && PrimitiveComponent->Mobility != EComponentMobility::Movable)
		{
			PrimitiveComponent->SetMobility(EComponentMobility::Movable);
		}
	}
}

void ALaserGateController::LogCanInteractStateIfChanged(
	bool bCanInteract,
	bool bGeneratorValid,
	bool bStageEnabled,
	bool bGeneratorStabilized,
	bool bCalibrationActive) const
{
	if (bHasLoggedCanInteractState && bLastCanInteractState == bCanInteract)
	{
		return;
	}

	bHasLoggedCanInteractState = true;
	bLastCanInteractState = bCanInteract;
	UE_LOG(LogECHO7, Display, TEXT("LASER_GATE_DIAG CanInteract changed: Result=%s LinkedGenerator=%s StageEnabled=%s Stabilized=%s CalibrationActive=%s GateOpening=%s GateOpened=%s"),
		bCanInteract ? TEXT("true") : TEXT("false"),
		bGeneratorValid ? TEXT("true") : TEXT("false"),
		bStageEnabled ? TEXT("true") : TEXT("false"),
		bGeneratorStabilized ? TEXT("true") : TEXT("false"),
		bCalibrationActive ? TEXT("true") : TEXT("false"),
		bOpening ? TEXT("true") : TEXT("false"),
		bOpened ? TEXT("true") : TEXT("false"));
}
