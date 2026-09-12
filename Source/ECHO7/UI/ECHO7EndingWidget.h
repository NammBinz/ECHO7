// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7EndingWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UImage;
class UTextBlock;
class UVerticalBox;

/** Native text display used by the ending controller's typewriter sequence. */
UCLASS()
class ECHO7_API UECHO7EndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UECHO7EndingWidget(const FObjectInitializer& ObjectInitializer);

	/** Clears every completed and currently typing ending line. */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void ResetEndingText();

	/**
	 * Displays the completed story lines followed by the currently typing line.
	 * The controller owns typewriter progression and supplies the active partial line.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ending")
	void SetEndingText(const TArray<FText>& CompletedLines, const FText& ActiveLine);

	/** Allocates one reusable visual row for every configured EndingLines entry. */
	void SetEndingLineCount(int32 LineCount);

	/** Controls the persistent black layer behind the ending story. */
	void SetFinalBlackOpacity(float Opacity);

	/** Displays a single native fullscreen glitch flash above the scene. */
	void SetGlitchFlash(const FLinearColor& Color, float Opacity);

	/** Updates one to three temporary horizontal visual strips for the current glitch pulse. */
	void SetRandomGlitchStrips(const FLinearColor& BaseColor, int32 StripCount);

	/** Removes every temporary glitch visual without affecting the final black layer. */
	void ClearGlitchVisuals();

	/** Shows or hides only the story text, leaving transition layers active. */
	void SetStoryVisible(bool bVisible);

	/** Sets the frozen puzzle-completion time shown on the final screen. */
	void SetCompletionTime(const FText& FormattedTime);

	/** Enables hidden Enter-key handling after all ending text has completed. */
	void SetRestartInputEnabled(bool bEnabled);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void RefreshDisplayedText();
	void RefreshAuxiliaryVisibility();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FinalBlackOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GlitchFlashOverlay;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> GlitchStrips;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> EndingRowsContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> EndingRows;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CompletionTimePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CompletionTimeValueText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> EndingCreditPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EndingCreditText;

	UPROPERTY(Transient)
	TArray<FText> CompletedEndingLines;

	UPROPERTY(Transient)
	FText ActiveEndingLine;

	UPROPERTY(Transient)
	FText CompletionTimeValue;

	FLinearColor GlitchFlashColor = FLinearColor::Black;
	float FinalBlackOpacity = 0.0f;
	float GlitchFlashOpacity = 0.0f;
	bool bStoryVisible = false;
	bool bRestartInputEnabled = false;
	int32 RequestedEndingLineCount = 0;
};
