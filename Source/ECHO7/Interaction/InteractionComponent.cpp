// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/InteractionComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/InteractableInterface.h"
#include "Engine/World.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocusedInteractable();
}

void UInteractionComponent::Interact()
{
	AActor* Interactable = FocusedInteractable.Get();
	AActor* Interactor = GetOwner();

	if (IsValid(Interactable)
		&& Interactable->Implements<UInteractableInterface>()
		&& IInteractableInterface::Execute_CanInteract(Interactable, Interactor))
	{
		IInteractableInterface::Execute_Interact(Interactable, Interactor);
	}
}

void UInteractionComponent::UpdateFocusedInteractable()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		SetFocusedInteractable(nullptr);
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetInteractionViewPoint(ViewLocation, ViewRotation))
	{
		SetFocusedInteractable(nullptr);
		return;
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractionTrace), false, Owner);
	FHitResult HitResult;

	AActor* NewFocusedInteractable = nullptr;
	if (World->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		if (IsValid(HitActor)
			&& HitActor->Implements<UInteractableInterface>()
			&& IInteractableInterface::Execute_CanInteract(HitActor, Owner))
		{
			NewFocusedInteractable = HitActor;
		}
	}

	SetFocusedInteractable(NewFocusedInteractable);
}

void UInteractionComponent::SetFocusedInteractable(AActor* NewFocusedInteractable)
{
	if (FocusedInteractable == NewFocusedInteractable)
	{
		return;
	}

	AActor* PreviousInteractable = FocusedInteractable.Get();
	FocusedInteractable = NewFocusedInteractable;
	OnFocusedInteractableChanged.Broadcast(PreviousInteractable, NewFocusedInteractable);
}

bool UInteractionComponent::GetInteractionViewPoint(FVector& ViewLocation, FRotator& ViewRotation) const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner && PawnOwner->GetController())
	{
		PawnOwner->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
		return true;
	}

	if (const AActor* Owner = GetOwner())
	{
		Owner->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		return true;
	}

	return false;
}
