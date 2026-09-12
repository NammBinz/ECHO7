// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7LaserGateGlitchWidget.generated.h"

class UBorder;

/** Native non-interactive full-screen security-failure glitch for an opening laser gate. */
UCLASS()
class ECHO7_API UECHO7LaserGateGlitchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void PlayGlitch(float InDuration);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void ApplyGlitchPulse();
	void ScheduleNextPulse();
	void FinishGlitch();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FullScreenFlash;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> GlitchStrips;

	FTimerHandle PulseTimer;
	FTimerHandle EndTimer;
	bool bActive = false;
};
