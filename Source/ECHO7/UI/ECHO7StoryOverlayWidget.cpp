// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7StoryOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TimerManager.h"

UECHO7StoryOverlayWidget::UECHO7StoryOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UECHO7StoryOverlayWidget::SetStoryLines(const TArray<FText>& Lines, int32 FontSize)
{
	RequestedLines = Lines;
	RequestedFontSize = FMath::Max(12, FontSize);
	bShowingCountdown = false;
	BuildLayout();

	if (CountdownText)
	{
		CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	for (int32 RowIndex = 0; RowIndex < StoryRows.Num(); ++RowIndex)
	{
		UTextBlock* StoryRow = StoryRows[RowIndex];
		if (!StoryRow)
		{
			continue;
		}

		if (RequestedLines.IsValidIndex(RowIndex) && !RequestedLines[RowIndex].IsEmpty())
		{
			StoryRow->SetText(RequestedLines[RowIndex]);
			StoryRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			StoryRow->SetText(FText::GetEmpty());
			StoryRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UECHO7StoryOverlayWidget::ShowCountdown(int32 CountdownValue, int32 FontSize)
{
	bShowingCountdown = true;
	RequestedCountdownFontSize = FMath::Max(12, FontSize);
	BuildLayout();

	for (UTextBlock* StoryRow : StoryRows)
	{
		if (StoryRow)
		{
			StoryRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (CountdownText)
	{
		CountdownText->SetText(FText::AsNumber(CountdownValue));
		CountdownText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UECHO7StoryOverlayWidget::ClearContent()
{
	RequestedLines.Reset();
	bShowingCountdown = false;
	for (UTextBlock* StoryRow : StoryRows)
	{
		if (StoryRow)
		{
			StoryRow->SetText(FText::GetEmpty());
			StoryRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (CountdownText)
	{
		CountdownText->SetText(FText::GetEmpty());
		CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UECHO7StoryOverlayWidget::PlayGlitch(float Duration)
{
	BuildLayout();
	if (!GlitchFlash)
	{
		return;
	}

	GlitchFlash->SetColorAndOpacity(FLinearColor(0.40f, 0.90f, 1.0f, 0.22f));
	GlitchFlash->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	for (int32 StripIndex = 0; StripIndex < GlitchStrips.Num(); ++StripIndex)
	{
		UImage* Strip = GlitchStrips[StripIndex];
		if (!Strip)
		{
			continue;
		}

		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			const float VerticalPosition = FMath::FRandRange(0.08f, 0.92f);
			StripSlot->SetAnchors(FAnchors(0.0f, VerticalPosition, 1.0f, FMath::Min(1.0f, VerticalPosition + FMath::FRandRange(0.008f, 0.025f))));
			StripSlot->SetOffsets(FMargin(0.0f));
		}
		Strip->SetColorAndOpacity(FLinearColor(0.48f, 0.95f, 1.0f, FMath::FRandRange(0.18f, 0.48f)));
		Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GlitchTimer);
		World->GetTimerManager().SetTimer(GlitchTimer, this, &UECHO7StoryOverlayWidget::ClearGlitch, FMath::Max(0.01f, Duration), false);
	}
	else
	{
		ClearGlitch();
	}
}

TSharedRef<SWidget> UECHO7StoryOverlayWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7StoryOverlayWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GlitchTimer);
	}

	Super::NativeDestruct();
}

void UECHO7StoryOverlayWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StoryOverlayRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!DarkBackdrop)
	{
		DarkBackdrop = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DarkBackdrop")));
	}
	if (!DarkBackdrop)
	{
		DarkBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DarkBackdrop"));
	}
	if (DarkBackdrop->GetParent() != RootCanvas)
	{
		DarkBackdrop->RemoveFromParent();
		RootCanvas->AddChildToCanvas(DarkBackdrop);
	}
	if (UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(DarkBackdrop->Slot))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(0);
	}
	DarkBackdrop->SetBrushColor(FLinearColor(0.005f, 0.012f, 0.03f, 0.80f));
	DarkBackdrop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!StoryRowsContainer)
	{
		StoryRowsContainer = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("StoryRowsContainer")));
	}
	if (!StoryRowsContainer)
	{
		StoryRowsContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StoryRowsContainer"));
	}
	if (StoryRowsContainer->GetParent() != RootCanvas)
	{
		StoryRowsContainer->RemoveFromParent();
		RootCanvas->AddChildToCanvas(StoryRowsContainer);
	}
	if (UCanvasPanelSlot* StorySlot = Cast<UCanvasPanelSlot>(StoryRowsContainer->Slot))
	{
		StorySlot->SetAnchors(FAnchors(0.5f, 0.5f));
		StorySlot->SetAlignment(FVector2D(0.5f, 0.5f));
		StorySlot->SetPosition(FVector2D::ZeroVector);
		StorySlot->SetAutoSize(true);
		StorySlot->SetZOrder(10);
	}
	StoryRowsContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	while (StoryRows.Num() < RequestedLines.Num())
	{
		const int32 RowIndex = StoryRows.Num();
		StoryRows.Add(WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("StoryRow%d"), RowIndex)));
	}
	for (UTextBlock* StoryRow : StoryRows)
	{
		if (!StoryRow)
		{
			continue;
		}
		if (StoryRow->GetParent() != StoryRowsContainer)
		{
			StoryRow->RemoveFromParent();
			StoryRowsContainer->AddChildToVerticalBox(StoryRow);
		}
		if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(StoryRow->Slot))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetPadding(FMargin(0.0f, 6.0f));
		}
		FSlateFontInfo Font = StoryRow->GetFont();
		Font.Size = RequestedFontSize;
		StoryRow->SetFont(Font);
		StoryRow->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		StoryRow->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		StoryRow->SetShadowOffset(FVector2D(2.0f, 3.0f));
		StoryRow->SetJustification(ETextJustify::Center);
		StoryRow->SetAutoWrapText(false);
		StoryRow->SetClipping(EWidgetClipping::ClipToBounds);
	}

	if (!CountdownText)
	{
		CountdownText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CountdownText")));
	}
	if (!CountdownText)
	{
		CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
	}
	if (CountdownText->GetParent() != RootCanvas)
	{
		CountdownText->RemoveFromParent();
		RootCanvas->AddChildToCanvas(CountdownText);
	}
	if (UCanvasPanelSlot* CountdownSlot = Cast<UCanvasPanelSlot>(CountdownText->Slot))
	{
		CountdownSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CountdownSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CountdownSlot->SetPosition(FVector2D::ZeroVector);
		CountdownSlot->SetAutoSize(true);
		CountdownSlot->SetZOrder(20);
	}
	FSlateFontInfo CountdownFont = CountdownText->GetFont();
	CountdownFont.Size = RequestedCountdownFontSize;
	CountdownText->SetFont(CountdownFont);
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.95f, 1.0f)));
	CountdownText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	CountdownText->SetShadowOffset(FVector2D(3.0f, 4.0f));
	CountdownText->SetJustification(ETextJustify::Center);
	if (!bShowingCountdown)
	{
		CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!GlitchFlash)
	{
		GlitchFlash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("GlitchFlash"));
	}
	if (GlitchFlash->GetParent() != RootCanvas)
	{
		GlitchFlash->RemoveFromParent();
		RootCanvas->AddChildToCanvas(GlitchFlash);
	}
	if (UCanvasPanelSlot* FlashSlot = Cast<UCanvasPanelSlot>(GlitchFlash->Slot))
	{
		FlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FlashSlot->SetOffsets(FMargin(0.0f));
		FlashSlot->SetZOrder(30);
	}
	GlitchFlash->SetVisibility(ESlateVisibility::Collapsed);

	while (GlitchStrips.Num() < 3)
	{
		GlitchStrips.Add(WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("StoryGlitchStrip%d"), GlitchStrips.Num())));
	}
	for (int32 StripIndex = 0; StripIndex < GlitchStrips.Num(); ++StripIndex)
	{
		UImage* Strip = GlitchStrips[StripIndex];
		if (Strip->GetParent() != RootCanvas)
		{
			Strip->RemoveFromParent();
			RootCanvas->AddChildToCanvas(Strip);
		}
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.02f));
			StripSlot->SetOffsets(FMargin(0.0f));
			StripSlot->SetZOrder(31 + StripIndex);
		}
		Strip->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UECHO7StoryOverlayWidget::ClearGlitch()
{
	if (GlitchFlash)
	{
		GlitchFlash->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UImage* Strip : GlitchStrips)
	{
		if (Strip)
		{
			Strip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
