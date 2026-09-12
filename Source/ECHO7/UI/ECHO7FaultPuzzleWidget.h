// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ECHO7FaultPuzzleWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UDragDropOperation;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;
class UVerticalBox;
class UECHO7FaultPuzzleWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFaultPuzzleSolvedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFaultPuzzleCancelledSignature);

/** One draggable 3x3 image tile owned by UECHO7FaultPuzzleWidget. */
UCLASS()
class ECHO7_API UECHO7FaultPuzzleTileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureTile(UECHO7FaultPuzzleWidget* InOwningPuzzle, UTexture2D* SourceTexture, int32 InSourceTileIndex, int32 InBoardSlotIndex);
	void SetBoardSlotIndex(int32 InBoardSlotIndex);
	void SetTileDisplaySize(const FVector2D& InTileDisplaySize);
	int32 GetSourceTileIndex() const { return SourceTileIndex; }
	int32 GetBoardSlotIndex() const { return BoardSlotIndex; }
	void SetSolvedAppearance(bool bSolved);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	void BuildTileLayout();
	void RefreshTileBrush();
	void SetHoveredAppearance(bool bHovered);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TileBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TileImage;

	TWeakObjectPtr<UECHO7FaultPuzzleWidget> OwningPuzzle;
	TObjectPtr<UTexture2D> PuzzleSourceTexture;
	FVector2D TileDisplaySize = FVector2D(368.0f, 207.0f);
	int32 SourceTileIndex = INDEX_NONE;
	int32 BoardSlotIndex = INDEX_NONE;
	bool bLockedSolved = false;
	bool bDragging = false;
};

/** Native 3x3 positional image-reconstruction puzzle used by the fault repair terminal. */
UCLASS()
class ECHO7_API UECHO7FaultPuzzleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UECHO7FaultPuzzleWidget(const FObjectInitializer& ObjectInitializer);

	/** Creates or restores the 3x3 board. Tile order entries identify the source image region in each board slot. */
	void InitializePuzzle(UTexture2D* InSourceTexture, bool bShuffleOnOpen, const TArray<int32>& ExistingTileOrder);

	/** Returns the current board order so a terminal can preserve an unfinished puzzle. */
	TArray<int32> GetTileOrder() const;

	void SetTileGap(float InTileGap);

	UPROPERTY(BlueprintAssignable, Category = "Fault Puzzle")
	FFaultPuzzleSolvedSignature OnPuzzleSolved;

	UPROPERTY(BlueprintAssignable, Category = "Fault Puzzle")
	FFaultPuzzleCancelledSignature OnPuzzleCancelled;

	/** Swaps two occupied board slots after a valid drag/drop. */
	void SwapTiles(int32 SourceSlotIndex, int32 TargetSlotIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	void BuildOrRestoreTiles();
	void PlaceTilesInGrid();
	void RefreshPuzzleSizing();
	FVector2D GetPuzzleImageSize() const;
	void SetPuzzleFrameSolvedAppearance(bool bSolved);
	void PlaySwapHighlight();
	void ClearSwapHighlight();
	void CheckForSolvedPuzzle();
	void FinishSolvedPuzzle();
	void BroadcastPuzzleSolved();
	void CancelPuzzle();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DimBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TerminalPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PanelContent;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HelpText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DragHintText;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> PuzzleAreaSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PuzzleFrame;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> PuzzleGrid;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SuccessText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FooterText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UECHO7FaultPuzzleTileWidget>> TilesBySlot;

	TObjectPtr<UTexture2D> SourceTexture;
	TArray<int32> RequestedTileOrder;
	FTimerHandle CompletionTimer;
	FTimerHandle SwapHighlightTimer;
	float TileGap = 4.0f;
	bool bPuzzleInitialized = false;
	bool bCompletionPending = false;
};
