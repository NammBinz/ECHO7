// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7EndingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ECHO7PlayerController.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

UECHO7EndingWidget::UECHO7EndingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CompletionTimeValue = NSLOCTEXT("ECHO7", "EndingCompletionTimeZero", "00:00");
	SetIsFocusable(true);
}

void UECHO7EndingWidget::ResetEndingText()
{
	CompletedEndingLines.Reset();
	ActiveEndingLine = FText::GetEmpty();
	RefreshDisplayedText();
}

void UECHO7EndingWidget::SetEndingText(const TArray<FText>& CompletedLines, const FText& ActiveLine)
{
	CompletedEndingLines = CompletedLines;
	ActiveEndingLine = ActiveLine;
	RefreshDisplayedText();
}

void UECHO7EndingWidget::SetEndingLineCount(int32 LineCount)
{
	RequestedEndingLineCount = FMath::Max(0, LineCount);
	BuildLayout();
	RefreshDisplayedText();
}

void UECHO7EndingWidget::SetFinalBlackOpacity(float Opacity)
{
	FinalBlackOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (FinalBlackOverlay)
	{
		FinalBlackOverlay->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, FinalBlackOpacity));
		FinalBlackOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UECHO7EndingWidget::SetGlitchFlash(const FLinearColor& Color, float Opacity)
{
	GlitchFlashColor = Color;
	GlitchFlashOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (GlitchFlashOverlay)
	{
		GlitchFlashOverlay->SetColorAndOpacity(FLinearColor(GlitchFlashColor.R, GlitchFlashColor.G, GlitchFlashColor.B, GlitchFlashOpacity));
		GlitchFlashOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UECHO7EndingWidget::SetRandomGlitchStrips(const FLinearColor& BaseColor, int32 StripCount)
{
	const int32 ActiveStripCount = FMath::Clamp(StripCount, 0, GlitchStrips.Num());
	for (int32 StripIndex = 0; StripIndex < GlitchStrips.Num(); ++StripIndex)
	{
		UImage* Strip = GlitchStrips[StripIndex];
		if (!Strip)
		{
			continue;
		}

		if (StripIndex >= ActiveStripCount)
		{
			Strip->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const float VerticalPosition = FMath::FRandRange(0.03f, 0.94f);
		const float StripHeight = FMath::FRandRange(0.008f, 0.045f);
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetAnchors(FAnchors(0.0f, VerticalPosition, 1.0f, FMath::Min(1.0f, VerticalPosition + StripHeight)));
			StripSlot->SetOffsets(FMargin(0.0f));
		}

		const float StripOpacity = FMath::FRandRange(0.10f, 0.32f);
		Strip->SetColorAndOpacity(FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B, StripOpacity));
		Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UECHO7EndingWidget::ClearGlitchVisuals()
{
	SetGlitchFlash(FLinearColor::Black, 0.0f);
	for (UImage* Strip : GlitchStrips)
	{
		if (Strip)
		{
			Strip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UECHO7EndingWidget::SetStoryVisible(bool bVisible)
{
	bStoryVisible = bVisible;
	for (UTextBlock* EndingRow : EndingRows)
	{
		if (EndingRow)
		{
			EndingRow->SetRenderOpacity(1.0f);
		}
	}
	RefreshDisplayedText();
	RefreshAuxiliaryVisibility();
}

void UECHO7EndingWidget::SetCompletionTime(const FText& FormattedTime)
{
	CompletionTimeValue = FormattedTime.IsEmpty()
		? NSLOCTEXT("ECHO7", "EndingCompletionTimeZero", "00:00")
		: FormattedTime;
	if (CompletionTimeValueText)
	{
		CompletionTimeValueText->SetText(FText::Format(
			NSLOCTEXT("ECHO7", "EndingCompletionTimeFormat", "TOTAL: {0}"),
			CompletionTimeValue));
	}
}

void UECHO7EndingWidget::SetRestartInputEnabled(bool bEnabled)
{
	bRestartInputEnabled = bEnabled;
}

FReply UECHO7EndingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bRestartInputEnabled && InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (AECHO7PlayerController* PlayerController = Cast<AECHO7PlayerController>(GetOwningPlayer()))
		{
			PlayerController->HandleEndingRestartKey();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

TSharedRef<SWidget> UECHO7EndingWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7EndingWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!FinalBlackOverlay)
	{
		FinalBlackOverlay = Cast<UImage>(WidgetTree->FindWidget(TEXT("FinalBlackOverlay")));
	}
	if (!FinalBlackOverlay)
	{
		FinalBlackOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FinalBlackOverlay"));
	}
	if (FinalBlackOverlay->GetParent() != RootCanvas)
	{
		FinalBlackOverlay->RemoveFromParent();
		RootCanvas->AddChildToCanvas(FinalBlackOverlay);
	}
	if (UCanvasPanelSlot* FinalBlackSlot = Cast<UCanvasPanelSlot>(FinalBlackOverlay->Slot))
	{
		FinalBlackSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FinalBlackSlot->SetOffsets(FMargin(0.0f));
		FinalBlackSlot->SetZOrder(0);
	}
	FinalBlackOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!GlitchFlashOverlay)
	{
		GlitchFlashOverlay = Cast<UImage>(WidgetTree->FindWidget(TEXT("GlitchFlashOverlay")));
	}
	if (!GlitchFlashOverlay)
	{
		GlitchFlashOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("GlitchFlashOverlay"));
	}
	if (GlitchFlashOverlay->GetParent() != RootCanvas)
	{
		GlitchFlashOverlay->RemoveFromParent();
		RootCanvas->AddChildToCanvas(GlitchFlashOverlay);
	}
	if (UCanvasPanelSlot* GlitchFlashSlot = Cast<UCanvasPanelSlot>(GlitchFlashOverlay->Slot))
	{
		GlitchFlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		GlitchFlashSlot->SetOffsets(FMargin(0.0f));
		GlitchFlashSlot->SetZOrder(1);
	}
	GlitchFlashOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	while (GlitchStrips.Num() < 3)
	{
		const int32 StripIndex = GlitchStrips.Num();
		const FName StripName(*FString::Printf(TEXT("GlitchStrip%d"), StripIndex));
		UImage* Strip = Cast<UImage>(WidgetTree->FindWidget(StripName));
		if (!Strip)
		{
			Strip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), StripName);
		}
		GlitchStrips.Add(Strip);
	}

	for (int32 StripIndex = 0; StripIndex < GlitchStrips.Num(); ++StripIndex)
	{
		UImage* Strip = GlitchStrips[StripIndex];
		if (!Strip)
		{
			continue;
		}

		if (Strip->GetParent() != RootCanvas)
		{
			Strip->RemoveFromParent();
			RootCanvas->AddChildToCanvas(Strip);
		}
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.02f));
			StripSlot->SetOffsets(FMargin(0.0f));
			StripSlot->SetZOrder(2 + StripIndex);
		}
		Strip->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!CompletionTimePanel)
	{
		CompletionTimePanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("CompletionTimePanel")));
	}
	if (!CompletionTimePanel)
	{
		CompletionTimePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CompletionTimePanel"));
	}
	if (CompletionTimePanel->GetParent() != RootCanvas)
	{
		CompletionTimePanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(CompletionTimePanel);
	}
	if (UCanvasPanelSlot* CompletionPanelSlot = Cast<UCanvasPanelSlot>(CompletionTimePanel->Slot))
	{
		CompletionPanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CompletionPanelSlot->SetAlignment(FVector2D::ZeroVector);
		CompletionPanelSlot->SetPosition(FVector2D(44.0f, 44.0f));
		CompletionPanelSlot->SetAutoSize(true);
		CompletionPanelSlot->SetZOrder(10);
	}
	CompletionTimePanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.0f, 0.02f, 0.05f, 0.58f),
		4.0f,
		FLinearColor(0.15f, 0.78f, 1.0f, 0.75f),
		1.0f));
	CompletionTimePanel->SetPadding(FMargin(10.0f, 6.0f));

	if (!CompletionTimeValueText)
	{
		CompletionTimeValueText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CompletionTimeValueText")));
	}
	if (!CompletionTimeValueText)
	{
		CompletionTimeValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompletionTimeValueText"));
	}
	if (CompletionTimeValueText->GetParent() != CompletionTimePanel)
	{
		CompletionTimeValueText->RemoveFromParent();
		CompletionTimePanel->SetContent(CompletionTimeValueText);
	}
	CompletionTimeValueText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "EndingCompletionTimeFormat", "TOTAL: {0}"),
		CompletionTimeValue));
	FSlateFontInfo CompletionValueFont = CompletionTimeValueText->GetFont();
	CompletionValueFont.Size = 24;
	CompletionTimeValueText->SetFont(CompletionValueFont);
	CompletionTimeValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.92f, 1.0f, 1.0f)));
	CompletionTimeValueText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	CompletionTimeValueText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	CompletionTimeValueText->SetJustification(ETextJustify::Left);
	CompletionTimeValueText->SetAutoWrapText(false);

	if (!EndingRowsContainer)
	{
		EndingRowsContainer = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("EndingRowsContainer")));
	}
	if (!EndingRowsContainer)
	{
		EndingRowsContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EndingRowsContainer"));
	}
	if (EndingRowsContainer->GetParent() != RootCanvas)
	{
		EndingRowsContainer->RemoveFromParent();
		RootCanvas->AddChildToCanvas(EndingRowsContainer);
	}
	if (UCanvasPanelSlot* EndingRowsSlot = Cast<UCanvasPanelSlot>(EndingRowsContainer->Slot))
	{
		EndingRowsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		EndingRowsSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		EndingRowsSlot->SetPosition(FVector2D::ZeroVector);
		EndingRowsSlot->SetAutoSize(true);
		EndingRowsSlot->SetZOrder(10);
	}
	EndingRowsContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	while (EndingRows.Num() < RequestedEndingLineCount)
	{
		const int32 RowIndex = EndingRows.Num();
		const FName RowName(*FString::Printf(TEXT("EndingRow%d"), RowIndex));
		UTextBlock* EndingRow = Cast<UTextBlock>(WidgetTree->FindWidget(RowName));
		if (!EndingRow)
		{
			EndingRow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), RowName);
		}
		EndingRows.Add(EndingRow);
	}

	for (UTextBlock* EndingRow : EndingRows)
	{
		if (!EndingRow)
		{
			continue;
		}

		if (EndingRow->GetParent() != EndingRowsContainer)
		{
			EndingRow->RemoveFromParent();
			EndingRowsContainer->AddChildToVerticalBox(EndingRow);
		}
		if (UVerticalBoxSlot* EndingRowSlot = Cast<UVerticalBoxSlot>(EndingRow->Slot))
		{
			EndingRowSlot->SetHorizontalAlignment(HAlign_Center);
			EndingRowSlot->SetPadding(FMargin(0.0f, 8.0f));
		}

		FSlateFontInfo Font = EndingRow->GetFont();
		Font.Size = 30;
		EndingRow->SetFont(Font);
		EndingRow->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		EndingRow->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		EndingRow->SetShadowOffset(FVector2D(1.0f, 2.0f));
		EndingRow->SetJustification(ETextJustify::Center);
		EndingRow->SetAutoWrapText(false);
		EndingRow->SetClipping(EWidgetClipping::ClipToBounds);
	}

	if (!EndingCreditPanel)
	{
		EndingCreditPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("EndingCreditPanel")));
	}
	if (!EndingCreditPanel)
	{
		EndingCreditPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EndingCreditPanel"));
	}
	if (EndingCreditPanel->GetParent() != RootCanvas)
	{
		EndingCreditPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(EndingCreditPanel);
	}
	if (UCanvasPanelSlot* CreditSlot = Cast<UCanvasPanelSlot>(EndingCreditPanel->Slot))
	{
		CreditSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		CreditSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		CreditSlot->SetPosition(FVector2D(-44.0f, 44.0f));
		CreditSlot->SetAutoSize(true);
		CreditSlot->SetZOrder(10);
	}
	EndingCreditPanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.0f, 0.025f, 0.055f, 0.78f),
		6.0f,
		FLinearColor(0.18f, 0.82f, 1.0f, 0.90f),
		1.0f));
	EndingCreditPanel->SetPadding(FMargin(16.0f, 8.0f));
	EndingCreditPanel->SetHorizontalAlignment(HAlign_Right);
	EndingCreditPanel->SetVerticalAlignment(VAlign_Center);

	if (!EndingCreditText)
	{
		EndingCreditText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EndingCreditText")));
	}
	if (!EndingCreditText)
	{
		EndingCreditText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndingCreditText"));
	}
	if (EndingCreditText->GetParent() != EndingCreditPanel)
	{
		EndingCreditText->RemoveFromParent();
		EndingCreditPanel->SetContent(EndingCreditText);
	}
	EndingCreditText->SetText(NSLOCTEXT("ECHO7", "EndingCredit", "MỘT SẢN PHẨM CỦA NAMM BINZ"));
	FSlateFontInfo CreditFont = EndingCreditText->GetFont();
	CreditFont.Size = 24;
	EndingCreditText->SetFont(CreditFont);
	EndingCreditText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.94f, 1.0f, 1.0f)));
	EndingCreditText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	EndingCreditText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	EndingCreditText->SetJustification(ETextJustify::Right);
	EndingCreditText->SetAutoWrapText(false);
	SetFinalBlackOpacity(FinalBlackOpacity);
	SetGlitchFlash(GlitchFlashColor, GlitchFlashOpacity);
	SetStoryVisible(bStoryVisible);

	RefreshDisplayedText();
	RefreshAuxiliaryVisibility();
}

void UECHO7EndingWidget::RefreshDisplayedText()
{
	const int32 RequiredRowCount = CompletedEndingLines.Num() + (ActiveEndingLine.IsEmpty() ? 0 : 1);
	if (RequiredRowCount > RequestedEndingLineCount)
	{
		RequestedEndingLineCount = RequiredRowCount;
		BuildLayout();
	}

	for (int32 RowIndex = 0; RowIndex < EndingRows.Num(); ++RowIndex)
	{
		UTextBlock* EndingRow = EndingRows[RowIndex];
		if (!EndingRow)
		{
			continue;
		}

		const bool bIsCompletedRow = CompletedEndingLines.IsValidIndex(RowIndex);
		const bool bIsActiveRow = !bIsCompletedRow && RowIndex == CompletedEndingLines.Num() && !ActiveEndingLine.IsEmpty();
		if (bIsCompletedRow)
		{
			EndingRow->SetText(CompletedEndingLines[RowIndex]);
			EndingRow->SetVisibility(bStoryVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		else if (bIsActiveRow)
		{
			EndingRow->SetText(ActiveEndingLine);
			EndingRow->SetVisibility(bStoryVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		else
		{
			EndingRow->SetText(FText::GetEmpty());
			EndingRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UECHO7EndingWidget::RefreshAuxiliaryVisibility()
{
	if (CompletionTimePanel)
	{
		CompletionTimePanel->SetVisibility(bStoryVisible
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (EndingCreditPanel)
	{
		EndingCreditPanel->SetVisibility(bStoryVisible
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
