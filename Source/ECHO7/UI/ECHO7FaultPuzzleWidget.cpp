// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7FaultPuzzleWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"

namespace ECHO7FaultPuzzle
{
	constexpr int32 GridSize = 3;
	constexpr float TargetPuzzleWidth = 1140.0f;
	constexpr float DefaultPuzzleAspectRatio = 16.0f / 9.0f;
	constexpr float CompletionDisplayDuration = 1.5f;
	constexpr float SwapHighlightDuration = 0.16f;
	const FLinearColor Cyan = FLinearColor(0.12f, 0.86f, 1.0f, 1.0f);
}

void UECHO7FaultPuzzleTileWidget::ConfigureTile(
	UECHO7FaultPuzzleWidget* InOwningPuzzle,
	UTexture2D* SourceTexture,
	int32 InSourceTileIndex,
	int32 InBoardSlotIndex)
{
	OwningPuzzle = InOwningPuzzle;
	PuzzleSourceTexture = SourceTexture;
	SourceTileIndex = InSourceTileIndex;
	BoardSlotIndex = InBoardSlotIndex;
	bLockedSolved = false;
	BuildTileLayout();
	RefreshTileBrush();
	SetSolvedAppearance(false);
}

void UECHO7FaultPuzzleTileWidget::SetBoardSlotIndex(int32 InBoardSlotIndex)
{
	BoardSlotIndex = InBoardSlotIndex;
}

void UECHO7FaultPuzzleTileWidget::SetTileDisplaySize(const FVector2D& InTileDisplaySize)
{
	TileDisplaySize = InTileDisplaySize;
	RefreshTileBrush();
}

void UECHO7FaultPuzzleTileWidget::SetSolvedAppearance(bool bSolved)
{
	bLockedSolved = bSolved;
	bDragging = false;
	SetRenderScale(FVector2D::UnitVector);
	SetRenderOpacity(1.0f);
	if (TileBorder)
	{
		TileBorder->SetBrush(FSlateRoundedBoxBrush(
			bSolved ? FLinearColor::Transparent : FLinearColor(0.008f, 0.035f, 0.065f, 0.98f),
			bSolved ? 0.0f : 2.0f,
			bSolved ? FLinearColor::Transparent : FLinearColor(0.10f, 0.58f, 0.72f, 0.85f),
			bSolved ? 0.0f : 1.0f));
		TileBorder->SetPadding(bSolved ? FMargin(0.0f) : FMargin(2.0f));
	}
}

TSharedRef<SWidget> UECHO7FaultPuzzleTileWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	BuildTileLayout();
	RefreshTileBrush();
	return Super::RebuildWidget();
}

FReply UECHO7FaultPuzzleTileWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bLockedSolved && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UECHO7FaultPuzzleTileWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (!bLockedSolved && !bDragging)
	{
		SetHoveredAppearance(true);
		SetRenderScale(FVector2D(1.015f));
	}
}

void UECHO7FaultPuzzleTileWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (!bLockedSolved && !bDragging)
	{
		SetHoveredAppearance(false);
		SetRenderScale(FVector2D::UnitVector);
	}
}

void UECHO7FaultPuzzleTileWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (bLockedSolved || !OwningPuzzle.IsValid())
	{
		return;
	}

	UDragDropOperation* DragOperation = NewObject<UDragDropOperation>(this);
	DragOperation->Payload = this;
	DragOperation->Pivot = EDragPivot::MouseDown;
	if (TileImage)
	{
		UImage* DragPreview = NewObject<UImage>(DragOperation);
		DragPreview->SetBrush(TileImage->GetBrush());
		DragPreview->SetRenderOpacity(0.78f);
		DragOperation->DefaultDragVisual = DragPreview;
	}

	bDragging = true;
	SetRenderOpacity(0.42f);
	SetHoveredAppearance(true);
	SetRenderScale(FVector2D(1.02f));
	OutOperation = DragOperation;
}

void UECHO7FaultPuzzleTileWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	if (!bLockedSolved && InOperation && InOperation->Payload != this)
	{
		SetHoveredAppearance(true);
		SetRenderScale(FVector2D(1.015f));
	}
}

void UECHO7FaultPuzzleTileWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	if (!bLockedSolved)
	{
		SetHoveredAppearance(false);
		SetRenderScale(FVector2D::UnitVector);
	}
}

bool UECHO7FaultPuzzleTileWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHoveredAppearance(false);
	SetRenderScale(FVector2D::UnitVector);
	UECHO7FaultPuzzleTileWidget* DraggedTile = InOperation ? Cast<UECHO7FaultPuzzleTileWidget>(InOperation->Payload) : nullptr;
	if (DraggedTile)
	{
		DraggedTile->bDragging = false;
		DraggedTile->SetRenderOpacity(1.0f);
		DraggedTile->SetRenderScale(FVector2D::UnitVector);
		DraggedTile->SetHoveredAppearance(false);
	}

	if (!bLockedSolved && DraggedTile && DraggedTile != this && OwningPuzzle.IsValid())
	{
		OwningPuzzle->SwapTiles(DraggedTile->GetBoardSlotIndex(), BoardSlotIndex);
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UECHO7FaultPuzzleTileWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	if (InOperation && InOperation->Payload == this)
	{
		bDragging = false;
		SetRenderOpacity(1.0f);
		SetRenderScale(FVector2D::UnitVector);
		SetHoveredAppearance(false);
	}
}

void UECHO7FaultPuzzleTileWidget::BuildTileLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	TileBorder = Cast<UBorder>(WidgetTree->RootWidget);
	if (!TileBorder)
	{
		TileBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TileBorder"));
		WidgetTree->RootWidget = TileBorder;
	}
	TileBorder->SetPadding(bLockedSolved ? FMargin(0.0f) : FMargin(2.0f));
	TileBorder->SetVisibility(ESlateVisibility::Visible);

	if (!TileImage)
	{
		TileImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("TileImage")));
	}
	if (!TileImage)
	{
		TileImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TileImage"));
	}
	if (TileImage->GetParent() != TileBorder)
	{
		TileImage->RemoveFromParent();
		TileBorder->SetContent(TileImage);
	}
	TileImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UECHO7FaultPuzzleTileWidget::RefreshTileBrush()
{
	if (!TileImage)
	{
		return;
	}

	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = TileDisplaySize;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	if (PuzzleSourceTexture && SourceTileIndex >= 0)
	{
		const int32 SourceColumn = SourceTileIndex % ECHO7FaultPuzzle::GridSize;
		const int32 SourceRow = SourceTileIndex / ECHO7FaultPuzzle::GridSize;
		Brush.SetResourceObject(PuzzleSourceTexture);
		Brush.SetUVRegion(FBox2D(
			FVector2D(static_cast<float>(SourceColumn) / ECHO7FaultPuzzle::GridSize, static_cast<float>(SourceRow) / ECHO7FaultPuzzle::GridSize),
			FVector2D(static_cast<float>(SourceColumn + 1) / ECHO7FaultPuzzle::GridSize, static_cast<float>(SourceRow + 1) / ECHO7FaultPuzzle::GridSize)));
	}
	else
	{
		Brush.TintColor = FSlateColor(FLinearColor(0.05f, 0.14f, 0.19f, 1.0f));
	}
	TileImage->SetBrush(Brush);
}

void UECHO7FaultPuzzleTileWidget::SetHoveredAppearance(bool bHovered)
{
	if (!TileBorder || bLockedSolved)
	{
		return;
	}
	TileBorder->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.015f, 0.065f, 0.105f, 0.98f),
		2.0f,
		bHovered ? FLinearColor(0.72f, 1.0f, 1.0f, 1.0f) : ECHO7FaultPuzzle::Cyan,
		bHovered ? 3.0f : 1.5f));
}

UECHO7FaultPuzzleWidget::UECHO7FaultPuzzleWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UECHO7FaultPuzzleWidget::InitializePuzzle(UTexture2D* InSourceTexture, bool bShuffleOnOpen, const TArray<int32>& ExistingTileOrder)
{
	SourceTexture = InSourceTexture;
	TilesBySlot.Reset();
	RequestedTileOrder = ExistingTileOrder;
	if (RequestedTileOrder.Num() != ECHO7FaultPuzzle::GridSize * ECHO7FaultPuzzle::GridSize)
	{
		RequestedTileOrder.Reset();
		for (int32 TileIndex = 0; TileIndex < ECHO7FaultPuzzle::GridSize * ECHO7FaultPuzzle::GridSize; ++TileIndex)
		{
			RequestedTileOrder.Add(TileIndex);
		}
		if (bShuffleOnOpen)
		{
			for (int32 TileIndex = RequestedTileOrder.Num() - 1; TileIndex > 0; --TileIndex)
			{
				RequestedTileOrder.Swap(TileIndex, FMath::RandRange(0, TileIndex));
			}
			if (RequestedTileOrder[0] == 0 && RequestedTileOrder[1] == 1)
			{
				RequestedTileOrder.Swap(0, 1);
			}
		}
	}

	bPuzzleInitialized = true;
	bCompletionPending = false;
	BuildLayout();
	BuildOrRestoreTiles();
}

TArray<int32> UECHO7FaultPuzzleWidget::GetTileOrder() const
{
	TArray<int32> TileOrder;
	for (const UECHO7FaultPuzzleTileWidget* Tile : TilesBySlot)
	{
		TileOrder.Add(Tile ? Tile->GetSourceTileIndex() : INDEX_NONE);
	}
	return TileOrder;
}

void UECHO7FaultPuzzleWidget::SetTileGap(float InTileGap)
{
	TileGap = FMath::Max(0.0f, InTileGap);
	RefreshPuzzleSizing();
}

void UECHO7FaultPuzzleWidget::SwapTiles(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (bCompletionPending
		|| !TilesBySlot.IsValidIndex(SourceSlotIndex)
		|| !TilesBySlot.IsValidIndex(TargetSlotIndex)
		|| SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	TilesBySlot.Swap(SourceSlotIndex, TargetSlotIndex);
	PlaceTilesInGrid();
	PlaySwapHighlight();
	CheckForSolvedPuzzle();
}

TSharedRef<SWidget> UECHO7FaultPuzzleWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	BuildLayout();
	if (bPuzzleInitialized)
	{
		BuildOrRestoreTiles();
	}
	return Super::RebuildWidget();
}

void UECHO7FaultPuzzleWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CompletionTimer);
		World->GetTimerManager().ClearTimer(SwapHighlightTimer);
	}
	Super::NativeDestruct();
}

FReply UECHO7FaultPuzzleWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bCompletionPending && InKeyEvent.GetKey() == EKeys::Escape)
	{
		CancelPuzzle();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UECHO7FaultPuzzleWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FaultPuzzleRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!DimBackdrop)
	{
		DimBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBackdrop"));
	}
	if (DimBackdrop->GetParent() != RootCanvas)
	{
		DimBackdrop->RemoveFromParent();
		RootCanvas->AddChildToCanvas(DimBackdrop);
	}
	if (UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(DimBackdrop->Slot))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}
	DimBackdrop->SetBrushColor(FLinearColor(0.0f, 0.008f, 0.025f, 0.83f));
	DimBackdrop->SetVisibility(ESlateVisibility::Visible);

	if (!TerminalPanel)
	{
		TerminalPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TerminalPanel"));
	}
	if (TerminalPanel->GetParent() != RootCanvas)
	{
		TerminalPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(TerminalPanel);
	}
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(TerminalPanel->Slot))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(1280.0f, 900.0f));
		PanelSlot->SetZOrder(5);
	}
	TerminalPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.004f, 0.018f, 0.050f, 0.99f), 6.0f, FLinearColor(0.12f, 0.72f, 0.88f, 0.95f), 1.5f));
	TerminalPanel->SetPadding(FMargin(32.0f, 24.0f));

	if (!PanelContent)
	{
		PanelContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelContent"));
	}
	if (PanelContent->GetParent() != TerminalPanel)
	{
		PanelContent->RemoveFromParent();
		TerminalPanel->SetContent(PanelContent);
	}

	auto AddPanelChild = [this](UWidget* Child, const FMargin& ChildPadding)
	{
		if (Child->GetParent() != PanelContent)
		{
			Child->RemoveFromParent();
			PanelContent->AddChildToVerticalBox(Child);
		}
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Child->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(ChildPadding);
		}
	};

	if (!TitleText)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	}
	TitleText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleTitle", "KHÔI PHỤC DỮ LIỆU HỆ THỐNG"));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 34;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(ECHO7FaultPuzzle::Cyan));
	TitleText->SetShadowColorAndOpacity(FLinearColor::Black);
	TitleText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	TitleText->SetJustification(ETextJustify::Center);
	AddPanelChild(TitleText, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	if (!SubtitleText)
	{
		SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
	}
	SubtitleText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleSubtitle", "GIAO THỨC KHÔI PHỤC HÌNH ẢNH"));
	FSlateFontInfo SubtitleFont = SubtitleText->GetFont();
	SubtitleFont.Size = 16;
	SubtitleText->SetFont(SubtitleFont);
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.84f, 0.92f)));
	SubtitleText->SetShadowColorAndOpacity(FLinearColor::Black);
	SubtitleText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	SubtitleText->SetJustification(ETextJustify::Center);
	AddPanelChild(SubtitleText, FMargin(0.0f, 0.0f, 0.0f, 2.0f));

	if (!StatusText)
	{
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	}
	StatusText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleStatus", "▰  KẾT NỐI DỮ LIỆU  //  03 × 03  //  ĐỒNG BỘ HÓA HÌNH ẢNH"));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 12;
	StatusText->SetFont(StatusFont);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.95f, 1.0f, 0.82f)));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	AddPanelChild(StatusText, FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	if (!HelpText)
	{
		HelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelpText"));
	}
	HelpText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleHelp", "Kéo các mảnh dữ liệu về đúng vị trí để khôi phục hình ảnh."));
	FSlateFontInfo HelpFont = HelpText->GetFont();
	HelpFont.Size = 18;
	HelpText->SetFont(HelpFont);
	HelpText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleHelp", "Kéo một mảnh dữ liệu vào mảnh khác để hoán đổi vị trí."));
	HelpText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	HelpText->SetShadowColorAndOpacity(FLinearColor::Black);
	HelpText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	HelpText->SetJustification(ETextJustify::Center);
	HelpText->SetAutoWrapText(false);
	AddPanelChild(HelpText, FMargin(0.0f, 10.0f, 0.0f, 2.0f));

	if (!DragHintText)
	{
		DragHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DragHintText"));
	}
	DragHintText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleDragHint", "MỌI MẢNH ĐỀU CÓ THỂ DI CHUYỂN"));
	FSlateFontInfo DragHintFont = DragHintText->GetFont();
	DragHintFont.Size = 13;
	DragHintText->SetFont(DragHintFont);
	DragHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.40f, 0.90f, 1.0f, 0.88f)));
	DragHintText->SetJustification(ETextJustify::Center);
	DragHintText->SetVisibility(bCompletionPending ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	AddPanelChild(DragHintText, FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	if (!PuzzleGrid)
	{
		PuzzleGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("PuzzleGrid"));
	}
	if (!PuzzleFrame)
	{
		PuzzleFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PuzzleFrame"));
	}
	if (PuzzleGrid->GetParent() != PuzzleFrame)
	{
		PuzzleGrid->RemoveFromParent();
		PuzzleFrame->SetContent(PuzzleGrid);
	}
	if (!PuzzleAreaSizeBox)
	{
		PuzzleAreaSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PuzzleAreaSizeBox"));
	}
	if (PuzzleFrame->GetParent() != PuzzleAreaSizeBox)
	{
		PuzzleFrame->RemoveFromParent();
		PuzzleAreaSizeBox->SetContent(PuzzleFrame);
	}
	RefreshPuzzleSizing();
	SetPuzzleFrameSolvedAppearance(bCompletionPending);
	AddPanelChild(PuzzleAreaSizeBox, FMargin(0.0f, 0.0f, 0.0f, 0.0f));

	if (!SuccessText)
	{
		SuccessText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SuccessText"));
	}
	SuccessText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleSuccess", "DỮ LIỆU ĐÃ ĐƯỢC KHÔI PHỤC"));
	FSlateFontInfo SuccessFont = SuccessText->GetFont();
	SuccessFont.Size = 28;
	SuccessText->SetFont(SuccessFont);
	SuccessText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleSuccess", "KHÔI PHỤC DỮ LIỆU THÀNH CÔNG"));
	SuccessText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 1.0f, 0.68f)));
	SuccessText->SetShadowColorAndOpacity(FLinearColor::Black);
	SuccessText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	SuccessText->SetJustification(ETextJustify::Center);
	SuccessText->SetVisibility(bCompletionPending ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	AddPanelChild(SuccessText, FMargin(0.0f, 8.0f, 0.0f, 0.0f));

	if (!FooterText)
	{
		FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FooterText"));
	}
	FooterText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleFooter", "[ESC] THOÁT"));
	FSlateFontInfo FooterFont = FooterText->GetFont();
	FooterFont.Size = 13;
	FooterText->SetFont(FooterFont);
	FooterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.52f, 0.68f, 0.76f, 0.86f)));
	FooterText->SetJustification(ETextJustify::Center);
	FooterText->SetVisibility(bCompletionPending ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	AddPanelChild(FooterText, FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UECHO7FaultPuzzleWidget::BuildOrRestoreTiles()
{
	if (!PuzzleGrid || RequestedTileOrder.Num() != ECHO7FaultPuzzle::GridSize * ECHO7FaultPuzzle::GridSize)
	{
		return;
	}

	if (TilesBySlot.Num() == RequestedTileOrder.Num())
	{
		PlaceTilesInGrid();
		return;
	}

	TilesBySlot.Reset();
	for (int32 SlotIndex = 0; SlotIndex < RequestedTileOrder.Num(); ++SlotIndex)
	{
		UECHO7FaultPuzzleTileWidget* Tile = WidgetTree->ConstructWidget<UECHO7FaultPuzzleTileWidget>(
			UECHO7FaultPuzzleTileWidget::StaticClass(),
			*FString::Printf(TEXT("FaultPuzzleTile%d"), SlotIndex));
		Tile->ConfigureTile(this, SourceTexture, RequestedTileOrder[SlotIndex], SlotIndex);
		TilesBySlot.Add(Tile);
	}
	PlaceTilesInGrid();
}

void UECHO7FaultPuzzleWidget::PlaceTilesInGrid()
{
	if (!PuzzleGrid)
	{
		return;
	}
	RefreshPuzzleSizing();
	PuzzleGrid->ClearChildren();

	for (int32 SlotIndex = 0; SlotIndex < TilesBySlot.Num(); ++SlotIndex)
	{
		UECHO7FaultPuzzleTileWidget* Tile = TilesBySlot[SlotIndex];
		if (!Tile)
		{
			continue;
		}
		Tile->SetBoardSlotIndex(SlotIndex);
		PuzzleGrid->AddChildToUniformGrid(Tile, SlotIndex / ECHO7FaultPuzzle::GridSize, SlotIndex % ECHO7FaultPuzzle::GridSize);
		if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(Tile->Slot))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
		Tile->SetSolvedAppearance(bCompletionPending);
	}
}

FVector2D UECHO7FaultPuzzleWidget::GetPuzzleImageSize() const
{
	float AspectRatio = ECHO7FaultPuzzle::DefaultPuzzleAspectRatio;
	if (SourceTexture && SourceTexture->GetSizeY() > 0)
	{
		AspectRatio = static_cast<float>(SourceTexture->GetSizeX()) / static_cast<float>(SourceTexture->GetSizeY());
	}

	return FVector2D(ECHO7FaultPuzzle::TargetPuzzleWidth, ECHO7FaultPuzzle::TargetPuzzleWidth / FMath::Max(AspectRatio, KINDA_SMALL_NUMBER));
}

void UECHO7FaultPuzzleWidget::RefreshPuzzleSizing()
{
	if (!PuzzleGrid)
	{
		return;
	}

	const FVector2D ImageSize = GetPuzzleImageSize();
	const FVector2D TileDisplaySize = ImageSize / static_cast<float>(ECHO7FaultPuzzle::GridSize);
	const float ActiveTileGap = bCompletionPending ? 0.0f : TileGap;
	const FVector2D GridSizeWithGaps = ImageSize + FVector2D(
		ECHO7FaultPuzzle::GridSize * ActiveTileGap,
		ECHO7FaultPuzzle::GridSize * ActiveTileGap);

	if (PuzzleAreaSizeBox)
	{
		PuzzleAreaSizeBox->SetWidthOverride(GridSizeWithGaps.X);
		PuzzleAreaSizeBox->SetHeightOverride(GridSizeWithGaps.Y);
	}

	PuzzleGrid->SetMinDesiredSlotWidth(TileDisplaySize.X + ActiveTileGap);
	PuzzleGrid->SetMinDesiredSlotHeight(TileDisplaySize.Y + ActiveTileGap);
	PuzzleGrid->SetSlotPadding(FMargin(ActiveTileGap * 0.5f));
	for (UECHO7FaultPuzzleTileWidget* Tile : TilesBySlot)
	{
		if (Tile)
		{
			Tile->SetTileDisplaySize(TileDisplaySize);
		}
	}
}

void UECHO7FaultPuzzleWidget::SetPuzzleFrameSolvedAppearance(bool bSolved)
{
	if (!PuzzleFrame)
	{
		return;
	}

	PuzzleFrame->SetPadding(bSolved ? FMargin(0.0f) : FMargin(6.0f));
	PuzzleFrame->SetBrush(FSlateRoundedBoxBrush(
		bSolved ? FLinearColor::Transparent : FLinearColor(0.003f, 0.025f, 0.055f, 0.98f),
		bSolved ? 0.0f : 4.0f,
		bSolved ? FLinearColor(0.20f, 1.0f, 1.0f, 1.0f) : FLinearColor(0.08f, 0.64f, 0.83f, 0.95f),
		bSolved ? 3.0f : 1.25f));
}

void UECHO7FaultPuzzleWidget::PlaySwapHighlight()
{
	if (!PuzzleFrame || bCompletionPending)
	{
		return;
	}

	PuzzleFrame->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.02f, 0.14f, 0.19f, 0.98f),
		4.0f,
		FLinearColor(0.62f, 1.0f, 1.0f, 1.0f),
		2.5f));
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SwapHighlightTimer, this, &UECHO7FaultPuzzleWidget::ClearSwapHighlight, ECHO7FaultPuzzle::SwapHighlightDuration, false);
	}
}

void UECHO7FaultPuzzleWidget::ClearSwapHighlight()
{
	SetPuzzleFrameSolvedAppearance(bCompletionPending);
}

void UECHO7FaultPuzzleWidget::CheckForSolvedPuzzle()
{
	for (int32 SlotIndex = 0; SlotIndex < TilesBySlot.Num(); ++SlotIndex)
	{
		if (!TilesBySlot[SlotIndex] || TilesBySlot[SlotIndex]->GetSourceTileIndex() != SlotIndex)
		{
			return;
		}
	}
	FinishSolvedPuzzle();
}

void UECHO7FaultPuzzleWidget::FinishSolvedPuzzle()
{
	if (bCompletionPending)
	{
		return;
	}
	bCompletionPending = true;
	PlaceTilesInGrid();
	SetPuzzleFrameSolvedAppearance(true);
	if (DragHintText)
	{
		DragHintText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (FooterText)
	{
		FooterText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (StatusText)
	{
		StatusText->SetText(NSLOCTEXT("ECHO7", "FaultPuzzleSolvedStatus", "■  TÍNH TOÀN VẸN DỮ LIỆU: 100%  //  ĐỒNG BỘ HÓA HOÀN TẤT"));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 1.0f, 0.76f, 1.0f)));
	}
	if (SuccessText)
	{
		SuccessText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CompletionTimer, this, &UECHO7FaultPuzzleWidget::BroadcastPuzzleSolved, ECHO7FaultPuzzle::CompletionDisplayDuration, false);
	}
	else
	{
		BroadcastPuzzleSolved();
	}
}

void UECHO7FaultPuzzleWidget::BroadcastPuzzleSolved()
{
	OnPuzzleSolved.Broadcast();
}

void UECHO7FaultPuzzleWidget::CancelPuzzle()
{
	OnPuzzleCancelled.Broadcast();
}
