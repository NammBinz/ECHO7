// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7StoryOverlayWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * Reusable native full-screen story layer for focused challenge moments.
 * Callers own timing/typewriter state and provide the visible rows.
 */
UCLASS()
class ECHO7_API UECHO7StoryOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UECHO7StoryOverlayWidget(const FObjectInitializer& ObjectInitializer);

	/** Displays the supplied manual rows in the center of a readable dark overlay. */
	void SetStoryLines(const TArray<FText>& Lines, int32 FontSize);

	/** Replaces the story rows with a single large centered countdown value. */
	void ShowCountdown(int32 CountdownValue, int32 FontSize);

	/** Clears every visible story or countdown row while retaining the dark layer. */
	void ClearContent();

	/** Plays a short native cyan/white glitch pulse over the overlay. */
	void PlayGlitch(float Duration);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void ClearGlitch();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DarkBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> StoryRowsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> StoryRows;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GlitchFlash;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> GlitchStrips;

	FTimerHandle GlitchTimer;
	TArray<FText> RequestedLines;
	int32 RequestedFontSize = 48;
	int32 RequestedCountdownFontSize = 100;
	bool bShowingCountdown = false;
};
