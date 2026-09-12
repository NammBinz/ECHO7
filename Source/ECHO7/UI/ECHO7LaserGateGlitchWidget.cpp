// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7LaserGateGlitchWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace ECHO7LaserGateGlitch
{
	constexpr float MinimumPulseInterval = 0.025f;
	constexpr float MaximumPulseInterval = 0.065f;
}

void UECHO7LaserGateGlitchWidget::PlayGlitch(float InDuration)
{
	if (bActive)
	{
		return;
	}
	bActive = true;
	ApplyGlitchPulse();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EndTimer, this, &UECHO7LaserGateGlitchWidget::FinishGlitch, FMath::Max(0.01f, InDuration), false);
	}
	else
	{
		FinishGlitch();
	}
}

TSharedRef<SWidget> UECHO7LaserGateGlitchWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7LaserGateGlitchWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimer);
		World->GetTimerManager().ClearTimer(EndTimer);
	}
	Super::NativeDestruct();
}

void UECHO7LaserGateGlitchWidget::BuildLayout()
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

	if (!FullScreenFlash)
	{
		FullScreenFlash = Cast<UBorder>(WidgetTree->FindWidget(TEXT("FullScreenFlash")));
	}
	if (!FullScreenFlash)
	{
		FullScreenFlash = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FullScreenFlash"));
	}
	if (FullScreenFlash->GetParent() != RootCanvas)
	{
		FullScreenFlash->RemoveFromParent();
		RootCanvas->AddChildToCanvas(FullScreenFlash);
	}
	if (UCanvasPanelSlot* FlashSlot = Cast<UCanvasPanelSlot>(FullScreenFlash->Slot))
	{
		FlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FlashSlot->SetOffsets(FMargin(0.0f));
		FlashSlot->SetZOrder(0);
	}
	FullScreenFlash->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	while (GlitchStrips.Num() < 6)
	{
		const int32 StripIndex = GlitchStrips.Num();
		GlitchStrips.Add(WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("GlitchStrip%d"), StripIndex)));
	}
	for (int32 StripIndex = 0; StripIndex < GlitchStrips.Num(); ++StripIndex)
	{
		UBorder* Strip = GlitchStrips[StripIndex];
		if (Strip->GetParent() != RootCanvas)
		{
			Strip->RemoveFromParent();
			RootCanvas->AddChildToCanvas(Strip);
		}
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			StripSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			StripSlot->SetZOrder(1 + StripIndex);
		}
		Strip->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UECHO7LaserGateGlitchWidget::ApplyGlitchPulse()
{
	if (!bActive)
	{
		return;
	}
	if (FullScreenFlash)
	{
		const int32 FlashStyle = FMath::RandRange(0, 2);
		const FLinearColor Color = FlashStyle == 0 ? FLinearColor(0.0f, 0.01f, 0.03f, 0.72f)
			: FlashStyle == 1 ? FLinearColor(0.0f, 0.52f, 0.72f, 0.38f)
			: FLinearColor(0.78f, 0.92f, 1.0f, 0.22f);
		FullScreenFlash->SetBrush(FSlateRoundedBoxBrush(Color, 0.0f));
	}
	for (UBorder* Strip : GlitchStrips)
	{
		if (!Strip)
		{
			continue;
		}
		Strip->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.36f, 0.92f, 1.0f, FMath::FRandRange(0.20f, 0.70f)), 1.0f));
		Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetPosition(FVector2D(FMath::FRandRange(-120.0f, 120.0f), FMath::FRandRange(-520.0f, 520.0f)));
			StripSlot->SetSize(FVector2D(FMath::FRandRange(680.0f, 2000.0f), FMath::FRandRange(3.0f, 18.0f)));
		}
	}
	ScheduleNextPulse();
}

void UECHO7LaserGateGlitchWidget::ScheduleNextPulse()
{
	if (bActive)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(PulseTimer, this, &UECHO7LaserGateGlitchWidget::ApplyGlitchPulse, FMath::FRandRange(ECHO7LaserGateGlitch::MinimumPulseInterval, ECHO7LaserGateGlitch::MaximumPulseInterval), false);
		}
	}
}

void UECHO7LaserGateGlitchWidget::FinishGlitch()
{
	bActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimer);
	}
	RemoveFromParent();
}
