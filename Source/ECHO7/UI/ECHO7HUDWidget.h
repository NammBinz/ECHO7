// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7HUDWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

/** Native HUD for objectives, recovery progress, and temporary system messages. */
UCLASS()
class ECHO7_API UECHO7HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetObjectiveText(const FText& NewObjective);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetProgress(int32 CurrentProgress, int32 TotalProgress);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowMessage(const FText& Message, float Duration);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowWarning(const FText& Message, float Duration);

	/** Displays authored message rows; each entry is rendered as one native text row. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowMessageLines(const TArray<FText>& Lines, float Duration);

	/** Displays authored warning rows; each entry is rendered as one native text row. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowWarningLines(const TArray<FText>& Lines, float Duration);

	/** Displays cumulative tutorial rows as a large centered text group. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowTutorialLines(const TArray<FText>& Lines, float Duration, int32 FontSize);

	/** Clears the currently displayed temporary story or warning rows. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearTemporaryMessage();

	/** Displays the prompt for the interactable currently under the player's crosshair. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetInteractionPrompt(const FText& Prompt);

	/** Hides the current interaction prompt. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearInteractionPrompt();

	/** Updates the compact bottom-center run timer without owning timer state. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetRunTimerText(const FText& FormattedTime);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void BuildMessageLineRows();
	void ShowMessageInternal(const TArray<FText>& Lines, float Duration, bool bIsWarning, int32 FontSize = INDEX_NONE, float VerticalAnchor = 0.70f);
	void ClearMessage();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RecoveryHeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RecoveryText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> MessageLinesContainer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> MessageLineRows;

	TArray<FText> ActiveMessageLines;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InteractionPromptPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InteractionPromptText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RunTimerPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RunTimerText;

	UPROPERTY(Transient)
	FText CachedRunTimerText;

	FTimerHandle MessageTimer;
};
