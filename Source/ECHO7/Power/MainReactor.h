// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MainReactor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReactorActivatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReactorProgressChangedSignature, int32, InstalledCoreCount, int32, RequiredCoreCount);

/** Tracks Energy Cores installed in the main reactor. */
UCLASS(Blueprintable)
class ECHO7_API AMainReactor : public AActor
{
	GENERATED_BODY()

public:
	AMainReactor();

	/** Registers one successfully installed Energy Core. */
	UFUNCTION(BlueprintCallable, Category = "Reactor")
	bool RegisterInstalledCore();

	UFUNCTION(BlueprintPure, Category = "Reactor")
	int32 GetInstalledCoreCount() const { return InstalledCoreCount; }

	UFUNCTION(BlueprintPure, Category = "Reactor")
	int32 GetRequiredCoreCount() const { return RequiredCoreCount; }

	UFUNCTION(BlueprintPure, Category = "Reactor")
	bool IsReactorOnline() const { return bIsOnline; }

	/** Broadcast when all required cores have been installed. */
	UPROPERTY(BlueprintAssignable, Category = "Reactor")
	FReactorActivatedSignature OnReactorActivated;

	/** Broadcast after a core is successfully registered. */
	UPROPERTY(BlueprintAssignable, Category = "Reactor")
	FReactorProgressChangedSignature OnReactorProgressChanged;

protected:
	/** Number of cores required before this reactor becomes online. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reactor", meta = (ClampMin = "1"))
	int32 RequiredCoreCount = 3;

	/** Number of cores installed in this reactor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reactor")
	int32 InstalledCoreCount = 0;

	/** True once this reactor has activated. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Reactor")
	bool bIsOnline = false;

	/** Optional visual representation assigned later in the editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

private:
	void ActivateReactor();
};
