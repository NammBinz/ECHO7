// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/BaseInteractableActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "ECHO7.h"

ABaseInteractableActor::ABaseInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitBoxExtent(FVector(50.0f));
	CollisionComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMeshComponent->SetupAttachment(CollisionComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetVisibility(true);

	InteractionPrompt = NSLOCTEXT("ECHO7", "DefaultInteractionPrompt", "Tương tác");
}

void ABaseInteractableActor::Interact_Implementation(AActor* Interactor)
{
	const FString Message = FString::Printf(TEXT("Đã tương tác với %s"), *GetName());

	UE_LOG(LogECHO7, Display, TEXT("%s"), *Message);
}

FText ABaseInteractableActor::GetInteractionPrompt_Implementation() const
{
	return InteractionPrompt;
}

bool ABaseInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}
