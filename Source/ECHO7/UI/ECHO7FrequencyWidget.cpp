// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7FrequencyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace ECHO7FrequencyWidget
{
	constexpr float OpeningGlitchPulseMinimumInterval = 0.03f;
	constexpr float OpeningGlitchPulseMaximumInterval = 0.06f;
}

void UECHO7FrequencyWidget::ConfigureOpeningGlitch(float InDuration)
{
	OpeningGlitchDuration = FMath::Max(0.0f, InDuration);
}

void UECHO7FrequencyWidget::InitializeCalibration(float InMinimumFrequency, float InMaximumFrequency, float InTargetFrequency, float InTargetTolerance, float InStartingFrequency, float InRequiredStableDuration)
{
	MinimumFrequency = FMath::Min(InMinimumFrequency, InMaximumFrequency);
	MaximumFrequency = FMath::Max(InMinimumFrequency, InMaximumFrequency);
	if (FMath::IsNearlyEqual(MinimumFrequency, MaximumFrequency))
	{
		MaximumFrequency = MinimumFrequency + 1.0f;
	}

	TargetFrequency = FMath::Clamp(InTargetFrequency, MinimumFrequency, MaximumFrequency);
	TargetTolerance = FMath::Max(0.0f, InTargetTolerance);
	CurrentFrequency = FMath::Clamp(InStartingFrequency, MinimumFrequency, MaximumFrequency);
	StableTime = 0.0f;
	RequiredStableDuration = FMath::Max(0.01f, InRequiredStableDuration);
	bInitialized = true;
	bInputArmed = false;
	bIncreaseHeld = false;
	bCompletionPending = false;
	bOpeningGlitchActive = false;
	RefreshDisplay();
}

void UECHO7FrequencyWidget::UpdateCalibrationState(float InCurrentFrequency, float InStableTime)
{
	CurrentFrequency = FMath::Clamp(InCurrentFrequency, MinimumFrequency, MaximumFrequency);
	StableTime = FMath::Max(0.0f, InStableTime);
	RefreshDisplay();
}

void UECHO7FrequencyWidget::ShowSuccess()
{
	if (bCompletionPending)
	{
		return;
	}

	bCompletionPending = true;
	bIncreaseHeld = false;
	if (FeedbackText)
	{
		FeedbackText->SetText(NSLOCTEXT("ECHO7", "FrequencySuccess", "MÁY PHÁT ĐÃ ỔN ĐỊNH"));
		FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.65f)));
		FeedbackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SuccessTimer, this, &UECHO7FrequencyWidget::FinishSuccessfulCalibration, 0.9f, false);
	}
	else
	{
		FinishSuccessfulCalibration();
	}
}

TSharedRef<SWidget> UECHO7FrequencyWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7FrequencyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshDisplay();
	StartCalibrationTimer();
}

void UECHO7FrequencyWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputArmTimer);
		World->GetTimerManager().ClearTimer(SuccessTimer);
		World->GetTimerManager().ClearTimer(OpeningGlitchPulseTimer);
		World->GetTimerManager().ClearTimer(OpeningGlitchEndTimer);
	}

	Super::NativeDestruct();
}

void UECHO7FrequencyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateCalibration(InDeltaTime);
}

FReply UECHO7FrequencyWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && !bCompletionPending)
	{
		CancelCalibration();
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::E)
	{
		if (bInputArmed && !bCompletionPending)
		{
			bIncreaseHeld = true;
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UECHO7FrequencyWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E)
	{
		bIncreaseHeld = false;
		return FReply::Handled();
	}

	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UECHO7FrequencyWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = RootCanvas;
	}

	if (!CalibrationPanel)
	{
		CalibrationPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("CalibrationPanel")));
	}

	if (!CalibrationPanel)
	{
		CalibrationPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CalibrationPanel"));
	}

	if (CalibrationPanel->GetParent() != RootCanvas)
	{
		CalibrationPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(CalibrationPanel);
	}

	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(CalibrationPanel->Slot))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(760.0f, 450.0f));
	}

	CalibrationPanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.012f, 0.025f, 0.055f, 0.90f),
		12.0f,
		FLinearColor(0.18f, 0.52f, 0.72f, 0.70f),
		1.0f));
	CalibrationPanel->SetPadding(FMargin(32.0f, 26.0f));

	if (!ContentBox)
	{
		ContentBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("ContentBox")));
	}

	if (!ContentBox)
	{
		ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	}

	if (CalibrationPanel->GetContent() != ContentBox)
	{
		ContentBox->RemoveFromParent();
		CalibrationPanel->SetContent(ContentBox);
	}

	auto ConfigureText = [this](TObjectPtr<UTextBlock>& TextBlock, FName Name, int32 FontSize)
	{
		if (!TextBlock)
		{
			TextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(Name));
		}

		if (!TextBlock)
		{
			TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		TextBlock->SetShadowOffset(FVector2D(1.0f, 2.0f));
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetWrapTextAt(660.0f);
	};

	auto AddToContent = [this](UWidget* Widget, const FMargin& SlotPadding)
	{
		if (Widget->GetParent() != ContentBox)
		{
			Widget->RemoveFromParent();
			ContentBox->AddChildToVerticalBox(Widget);
		}

		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Widget->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(SlotPadding);
		}
	};

	ConfigureText(TitleText, TEXT("TitleText"), 28);
	TitleText->SetText(NSLOCTEXT("ECHO7", "FrequencyTitle", "HIỆU CHỈNH TẦN SỐ"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.88f, 1.0f, 1.0f)));
	AddToContent(TitleText, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	ConfigureText(FrequencyText, TEXT("FrequencyText"), 24);
	AddToContent(FrequencyText, FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	if (!FrequencyBarContainer)
	{
		FrequencyBarContainer = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("FrequencyBarContainer")));
	}

	if (!FrequencyBarContainer)
	{
		FrequencyBarContainer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FrequencyBarContainer"));
	}

	FrequencyBarContainer->SetWidthOverride(620.0f);
	FrequencyBarContainer->SetHeightOverride(54.0f);
	if (FrequencyBarContainer->GetParent() != ContentBox)
	{
		FrequencyBarContainer->RemoveFromParent();
		ContentBox->AddChildToVerticalBox(FrequencyBarContainer);
	}

	if (UVerticalBoxSlot* FrequencyBarContainerSlot = Cast<UVerticalBoxSlot>(FrequencyBarContainer->Slot))
	{
		FrequencyBarContainerSlot->SetHorizontalAlignment(HAlign_Center);
		FrequencyBarContainerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	if (!FrequencyBarOverlay)
	{
		FrequencyBarOverlay = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("FrequencyBarOverlay")));
	}

	if (!FrequencyBarOverlay)
	{
		FrequencyBarOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("FrequencyBarOverlay"));
	}

	if (FrequencyBarContainer->GetContent() != FrequencyBarOverlay)
	{
		FrequencyBarOverlay->RemoveFromParent();
		FrequencyBarContainer->SetContent(FrequencyBarOverlay);
	}

	if (!FrequencyBar)
	{
		FrequencyBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("FrequencyBar")));
	}

	if (!FrequencyBar)
	{
		FrequencyBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("FrequencyBar"));
	}

	if (FrequencyBar->GetParent() != FrequencyBarOverlay)
	{
		FrequencyBar->RemoveFromParent();
		FrequencyBarOverlay->AddChildToOverlay(FrequencyBar);
	}

	if (UOverlaySlot* FrequencyBarSlot = Cast<UOverlaySlot>(FrequencyBar->Slot))
	{
		FrequencyBarSlot->SetHorizontalAlignment(HAlign_Fill);
		FrequencyBarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	FProgressBarStyle FrequencyBarStyle;
	FrequencyBarStyle.SetBackgroundImage(FSlateRoundedBoxBrush(
		FLinearColor(0.008f, 0.015f, 0.03f, 1.0f),
		4.0f,
		FLinearColor(0.20f, 0.42f, 0.62f, 1.0f),
		1.5f));
	FrequencyBarStyle.SetFillImage(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.72f, 1.0f, 1.0f), 3.0f));
	FrequencyBar->SetWidgetStyle(FrequencyBarStyle);
	FrequencyBar->SetFillColorAndOpacity(FLinearColor::White);
	FrequencyBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	FrequencyBar->SetBarFillStyle(EProgressBarFillStyle::Scale);
	FrequencyBar->SetBorderPadding(FVector2D(3.0f, 3.0f));

	if (!FrequencyOverlayCanvas)
	{
		FrequencyOverlayCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("FrequencyOverlayCanvas")));
	}

	if (!FrequencyOverlayCanvas)
	{
		FrequencyOverlayCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FrequencyOverlayCanvas"));
	}
	FrequencyOverlayCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (FrequencyOverlayCanvas->GetParent() != FrequencyBarOverlay)
	{
		FrequencyOverlayCanvas->RemoveFromParent();
		FrequencyBarOverlay->AddChildToOverlay(FrequencyOverlayCanvas);
	}

	if (UOverlaySlot* FrequencyOverlayCanvasSlot = Cast<UOverlaySlot>(FrequencyOverlayCanvas->Slot))
	{
		FrequencyOverlayCanvasSlot->SetHorizontalAlignment(HAlign_Fill);
		FrequencyOverlayCanvasSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (!TargetRangeOverlay)
	{
		TargetRangeOverlay = Cast<UBorder>(WidgetTree->FindWidget(TEXT("TargetRangeOverlay")));
	}

	if (!TargetRangeOverlay)
	{
		TargetRangeOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TargetRangeOverlay"));
	}

	if (TargetRangeOverlay->GetParent() != FrequencyOverlayCanvas)
	{
		TargetRangeOverlay->RemoveFromParent();
		FrequencyOverlayCanvas->AddChildToCanvas(TargetRangeOverlay);
	}

	TargetRangeOverlay->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.90f, 0.04f, 0.08f, 0.28f),
		3.0f,
		FLinearColor(1.0f, 0.16f, 0.18f, 0.95f),
		1.25f));
	TargetRangeOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!PostTargetFrequencyFill)
	{
		PostTargetFrequencyFill = Cast<UBorder>(WidgetTree->FindWidget(TEXT("PostTargetFrequencyFill")));
	}

	if (!PostTargetFrequencyFill)
	{
		PostTargetFrequencyFill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PostTargetFrequencyFill"));
	}

	if (PostTargetFrequencyFill->GetParent() != FrequencyOverlayCanvas)
	{
		PostTargetFrequencyFill->RemoveFromParent();
		FrequencyOverlayCanvas->AddChildToCanvas(PostTargetFrequencyFill);
	}

	PostTargetFrequencyFill->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.72f, 1.0f, 1.0f), 2.0f));
	PostTargetFrequencyFill->SetVisibility(ESlateVisibility::Collapsed);

	if (!CurrentFrequencyMarker)
	{
		CurrentFrequencyMarker = Cast<UBorder>(WidgetTree->FindWidget(TEXT("CurrentFrequencyMarker")));
	}

	if (!CurrentFrequencyMarker)
	{
		CurrentFrequencyMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrentFrequencyMarker"));
	}

	if (CurrentFrequencyMarker->GetParent() != FrequencyOverlayCanvas)
	{
		CurrentFrequencyMarker->RemoveFromParent();
		FrequencyOverlayCanvas->AddChildToCanvas(CurrentFrequencyMarker);
	}

	CurrentFrequencyMarker->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 1.0f));
	CurrentFrequencyMarker->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	ConfigureText(TargetRangeText, TEXT("TargetRangeText"), 18);
	TargetRangeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.48f, 0.45f, 1.0f)));
	AddToContent(TargetRangeText, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	ConfigureText(IncreaseInstructionText, TEXT("IncreaseInstructionText"), 17);
	IncreaseInstructionText->SetText(NSLOCTEXT("ECHO7", "FrequencyIncreaseInstruction", "Giữ [E] để tăng tần số"));
	AddToContent(IncreaseInstructionText, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	ConfigureText(DecreaseInstructionText, TEXT("DecreaseInstructionText"), 17);
	DecreaseInstructionText->SetText(NSLOCTEXT("ECHO7", "FrequencyDecreaseInstruction", "Thả [E] để giảm tần số"));
	AddToContent(DecreaseInstructionText, FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	ConfigureText(StabilityText, TEXT("StabilityText"), 19);
	AddToContent(StabilityText, FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	ConfigureText(FeedbackText, TEXT("FeedbackText"), 20);
	AddToContent(FeedbackText, FMargin(0.0f));
	FeedbackText->SetVisibility(ESlateVisibility::Collapsed);

	if (!OpeningGlitchOverlay)
	{
		OpeningGlitchOverlay = Cast<UBorder>(WidgetTree->FindWidget(TEXT("OpeningGlitchOverlay")));
	}
	if (!OpeningGlitchOverlay)
	{
		OpeningGlitchOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OpeningGlitchOverlay"));
	}
	if (OpeningGlitchOverlay->GetParent() != RootCanvas)
	{
		OpeningGlitchOverlay->RemoveFromParent();
		RootCanvas->AddChildToCanvas(OpeningGlitchOverlay);
	}
	if (UCanvasPanelSlot* GlitchSlot = Cast<UCanvasPanelSlot>(OpeningGlitchOverlay->Slot))
	{
		GlitchSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		GlitchSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GlitchSlot->SetPosition(FVector2D::ZeroVector);
		GlitchSlot->SetSize(FVector2D(760.0f, 450.0f));
		GlitchSlot->SetZOrder(10);
	}
	OpeningGlitchOverlay->SetVisibility(ESlateVisibility::Collapsed);

	while (OpeningGlitchStrips.Num() < 3)
	{
		const int32 StripIndex = OpeningGlitchStrips.Num();
		OpeningGlitchStrips.Add(WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("OpeningGlitchStrip%d"), StripIndex)));
	}
	for (int32 StripIndex = 0; StripIndex < OpeningGlitchStrips.Num(); ++StripIndex)
	{
		UBorder* Strip = OpeningGlitchStrips[StripIndex];
		if (Strip->GetParent() != RootCanvas)
		{
			Strip->RemoveFromParent();
			RootCanvas->AddChildToCanvas(Strip);
		}
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			StripSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			StripSlot->SetZOrder(11 + StripIndex);
		}
		Strip->SetVisibility(ESlateVisibility::Collapsed);
	}

	RefreshDisplay();
}

void UECHO7FrequencyWidget::RefreshDisplay()
{
	if (!FrequencyText || !FrequencyBar || !TargetRangeText || !StabilityText)
	{
		return;
	}

	const int32 DisplayPercent = FMath::RoundToInt(GetFrequencyPercent() * 100.0f);
	FrequencyText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "FrequencyCurrent", "TẦN SỐ: {0}%"),
		FText::AsNumber(DisplayPercent)));

	const float TargetLowerBound = FMath::Clamp(TargetFrequency - TargetTolerance, MinimumFrequency, MaximumFrequency);
	const float TargetUpperBound = FMath::Clamp(TargetFrequency + TargetTolerance, MinimumFrequency, MaximumFrequency);
	const bool bIsInsideTargetZone = CurrentFrequency >= TargetLowerBound && CurrentFrequency <= TargetUpperBound;
	TargetRangeText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "FrequencyTargetRange", "VÙNG MỤC TIÊU: {0} - {1}"),
		FText::AsNumber(FMath::RoundToInt(TargetLowerBound)),
		FText::AsNumber(FMath::RoundToInt(TargetUpperBound))));

	constexpr float BarWidth = 620.0f;
	constexpr float BarHeight = 54.0f;
	constexpr float BarInset = 4.0f;
	constexpr float MinimumVisualTargetWidth = 3.0f;
	const float UsableBarWidth = BarWidth - (BarInset * 2.0f);
	const float UsableBarHeight = BarHeight - (BarInset * 2.0f);
	const float FrequencyRange = FMath::Max(KINDA_SMALL_NUMBER, MaximumFrequency - MinimumFrequency);
	const float TargetLowerPercent = FMath::Clamp((TargetLowerBound - MinimumFrequency) / FrequencyRange, 0.0f, 1.0f);
	const float TargetUpperPercent = FMath::Clamp((TargetUpperBound - MinimumFrequency) / FrequencyRange, 0.0f, 1.0f);
	const float CurrentFrequencyPercent = GetFrequencyPercent();
	// The position marker communicates values at or beyond the target zone so cyan never obscures the red range.
	FrequencyBar->SetPercent(FMath::Min(CurrentFrequencyPercent, TargetLowerPercent));
	const float TargetPositionX = BarInset + TargetLowerPercent * UsableBarWidth;
	const float TargetWidth = FMath::Max(MinimumVisualTargetWidth, (TargetUpperPercent - TargetLowerPercent) * UsableBarWidth);
	const float TargetVisualPositionX = FMath::Min(TargetPositionX, BarWidth - BarInset - MinimumVisualTargetWidth);
	const float TargetVisualWidth = FMath::Clamp(TargetWidth, MinimumVisualTargetWidth, BarWidth - BarInset - TargetVisualPositionX);

	if (UCanvasPanelSlot* TargetRangeSlot = TargetRangeOverlay ? Cast<UCanvasPanelSlot>(TargetRangeOverlay->Slot) : nullptr)
	{
		TargetRangeSlot->SetPosition(FVector2D(TargetVisualPositionX, BarInset));
		TargetRangeSlot->SetSize(FVector2D(TargetVisualWidth, UsableBarHeight));
		TargetRangeSlot->SetZOrder(1);
	}

	if (TargetRangeOverlay)
	{
		TargetRangeOverlay->SetBrush(FSlateRoundedBoxBrush(
			bIsInsideTargetZone ? FLinearColor(1.0f, 0.05f, 0.08f, 0.38f) : FLinearColor(0.90f, 0.04f, 0.08f, 0.28f),
			3.0f,
			bIsInsideTargetZone ? FLinearColor(1.0f, 0.34f, 0.36f, 1.0f) : FLinearColor(1.0f, 0.16f, 0.18f, 0.95f),
			bIsInsideTargetZone ? 1.75f : 1.25f));
	}

	const float PostTargetPositionX = BarInset + TargetUpperPercent * UsableBarWidth;
	const float PostTargetWidth = FMath::Max(0.0f, (CurrentFrequencyPercent - TargetUpperPercent) * UsableBarWidth);
	if (UCanvasPanelSlot* PostTargetFillSlot = PostTargetFrequencyFill ? Cast<UCanvasPanelSlot>(PostTargetFrequencyFill->Slot) : nullptr)
	{
		PostTargetFillSlot->SetPosition(FVector2D(PostTargetPositionX, BarInset));
		PostTargetFillSlot->SetSize(FVector2D(FMath::Min(PostTargetWidth, BarWidth - BarInset - PostTargetPositionX), UsableBarHeight));
		PostTargetFillSlot->SetZOrder(0);
	}
	if (PostTargetFrequencyFill)
	{
		PostTargetFrequencyFill->SetVisibility(PostTargetWidth > KINDA_SMALL_NUMBER
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	constexpr float MarkerWidth = 4.0f;
	const float MarkerPositionX = FMath::Clamp(
		BarInset + CurrentFrequencyPercent * UsableBarWidth - (MarkerWidth * 0.5f),
		BarInset,
		BarWidth - BarInset - MarkerWidth);
	if (UCanvasPanelSlot* MarkerSlot = CurrentFrequencyMarker ? Cast<UCanvasPanelSlot>(CurrentFrequencyMarker->Slot) : nullptr)
	{
		MarkerSlot->SetPosition(FVector2D(MarkerPositionX, BarInset));
		MarkerSlot->SetSize(FVector2D(MarkerWidth, UsableBarHeight));
		MarkerSlot->SetZOrder(2);
	}

	StabilityText->SetColorAndOpacity(FSlateColor(
		bIsInsideTargetZone ? FLinearColor(0.56f, 1.0f, 0.84f, 1.0f) : FLinearColor::White));

	FNumberFormattingOptions TimeFormat;
	TimeFormat.SetMinimumFractionalDigits(1);
	TimeFormat.SetMaximumFractionalDigits(1);
	StabilityText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "FrequencyStability", "ỔN ĐỊNH: {0} / {1} GIÂY"),
		FText::AsNumber(StableTime, &TimeFormat),
		FText::AsNumber(RequiredStableDuration, &TimeFormat)));
}

void UECHO7FrequencyWidget::StartCalibrationTimer()
{
	if (!bInitialized || bCompletionPending)
	{
		return;
	}

	if (OpeningGlitchDuration > 0.0f)
	{
		StartOpeningGlitch();
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(InputArmTimer, this, &UECHO7FrequencyWidget::ArmInput, 0.15f, false);
	}
}

void UECHO7FrequencyWidget::StartOpeningGlitch()
{
	if (bOpeningGlitchActive || bCompletionPending)
	{
		return;
	}

	bOpeningGlitchActive = true;
	bInputArmed = false;
	bIncreaseHeld = false;
	ApplyOpeningGlitchPulse();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(OpeningGlitchEndTimer, this, &UECHO7FrequencyWidget::FinishOpeningGlitch, OpeningGlitchDuration, false);
	}
	else
	{
		FinishOpeningGlitch();
	}
}

void UECHO7FrequencyWidget::ApplyOpeningGlitchPulse()
{
	if (!bOpeningGlitchActive)
	{
		return;
	}

	if (OpeningGlitchOverlay)
	{
		const FLinearColor FlashColor = FMath::RandBool()
			? FLinearColor(0.02f, 0.38f, 0.55f, 0.40f)
			: FLinearColor(0.78f, 0.94f, 1.0f, 0.22f);
		OpeningGlitchOverlay->SetBrush(FSlateRoundedBoxBrush(FlashColor, 4.0f));
		OpeningGlitchOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	for (UBorder* Strip : OpeningGlitchStrips)
	{
		if (!Strip)
		{
			continue;
		}
		Strip->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.40f, 0.92f, 1.0f, FMath::FRandRange(0.18f, 0.55f)), 1.0f));
		Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetPosition(FVector2D(FMath::FRandRange(-16.0f, 16.0f), FMath::FRandRange(-205.0f, 205.0f)));
			StripSlot->SetSize(FVector2D(FMath::FRandRange(420.0f, 740.0f), FMath::FRandRange(3.0f, 10.0f)));
		}
	}

	ScheduleNextOpeningGlitchPulse();
}

void UECHO7FrequencyWidget::ScheduleNextOpeningGlitchPulse()
{
	if (bOpeningGlitchActive)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				OpeningGlitchPulseTimer,
				this,
				&UECHO7FrequencyWidget::ApplyOpeningGlitchPulse,
				FMath::FRandRange(ECHO7FrequencyWidget::OpeningGlitchPulseMinimumInterval, ECHO7FrequencyWidget::OpeningGlitchPulseMaximumInterval),
				false);
		}
	}
}

void UECHO7FrequencyWidget::FinishOpeningGlitch()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OpeningGlitchPulseTimer);
	}
	bOpeningGlitchActive = false;
	if (OpeningGlitchOverlay)
	{
		OpeningGlitchOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UBorder* Strip : OpeningGlitchStrips)
	{
		if (Strip)
		{
			Strip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!bCompletionPending)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(InputArmTimer, this, &UECHO7FrequencyWidget::ArmInput, 0.15f, false);
		}
	}
}

void UECHO7FrequencyWidget::ArmInput()
{
	if (APlayerController* PlayerController = GetOwningPlayer(); PlayerController && PlayerController->IsInputKeyDown(EKeys::E))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(InputArmTimer, this, &UECHO7FrequencyWidget::ArmInput, 0.05f, false);
		}
		return;
	}

	bInputArmed = true;
}

void UECHO7FrequencyWidget::UpdateCalibration(float DeltaSeconds)
{
	if (!bInitialized || !bInputArmed || bCompletionPending)
	{
		return;
	}

	OnCalibrationTick.Broadcast(bIncreaseHeld, FMath::Max(0.0f, DeltaSeconds));
}

void UECHO7FrequencyWidget::FinishSuccessfulCalibration()
{
	OnCalibrationSucceeded.Broadcast();
	RemoveFromParent();
}

void UECHO7FrequencyWidget::CancelCalibration()
{
	if (bCompletionPending)
	{
		return;
	}

	bIncreaseHeld = false;
	OnCalibrationCancelled.Broadcast();
	RemoveFromParent();
}

float UECHO7FrequencyWidget::GetFrequencyPercent() const
{
	return FMath::Clamp((CurrentFrequency - MinimumFrequency) / FMath::Max(KINDA_SMALL_NUMBER, MaximumFrequency - MinimumFrequency), 0.0f, 1.0f);
}
