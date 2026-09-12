// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ECHO7.h"
#include "Engine/World.h"
#include "TimerManager.h"

TSharedRef<SWidget> UECHO7HUDWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UECHO7HUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MessageTimer);
	}

	Super::NativeDestruct();
}

void UECHO7HUDWidget::SetObjectiveText(const FText& NewObjective)
{
	if (ObjectiveText)
	{
		ObjectiveText->SetText(NewObjective);
	}
}

void UECHO7HUDWidget::SetProgress(int32 CurrentProgress, int32 TotalProgress)
{
	if (RecoveryText)
	{
		RecoveryText->SetText(FText::Format(
			NSLOCTEXT("ECHO7", "HUDProgressFormat", "{0}/{1}"),
			FText::AsNumber(CurrentProgress),
			FText::AsNumber(TotalProgress)));
	}
}

void UECHO7HUDWidget::ShowMessage(const FText& Message, float Duration)
{
	ShowMessageLines({ Message }, Duration);
}

void UECHO7HUDWidget::ShowWarning(const FText& Message, float Duration)
{
	ShowWarningLines({ Message }, Duration);
}

void UECHO7HUDWidget::ShowMessageLines(const TArray<FText>& Lines, float Duration)
{
	ShowMessageInternal(Lines, Duration, false);
}

void UECHO7HUDWidget::ShowWarningLines(const TArray<FText>& Lines, float Duration)
{
	ShowMessageInternal(Lines, Duration, true);
}

void UECHO7HUDWidget::ShowTutorialLines(const TArray<FText>& Lines, float Duration, int32 FontSize)
{
	ShowMessageInternal(Lines, Duration, false, FMath::Max(12, FontSize), 0.50f);
}

void UECHO7HUDWidget::ClearTemporaryMessage()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MessageTimer);
	}
	ClearMessage();
}

void UECHO7HUDWidget::SetInteractionPrompt(const FText& Prompt)
{
	if (Prompt.IsEmpty())
	{
		ClearInteractionPrompt();
		return;
	}

	if (!InteractionPromptPanel || !InteractionPromptText)
	{
		return;
	}

	InteractionPromptText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "HUDInteractionPromptFormat", "[E] {0}"),
		Prompt));
	InteractionPromptPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UECHO7HUDWidget::ClearInteractionPrompt()
{
	if (InteractionPromptPanel)
	{
		InteractionPromptPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UECHO7HUDWidget::SetRunTimerText(const FText& FormattedTime)
{
	CachedRunTimerText = FormattedTime.IsEmpty()
		? NSLOCTEXT("ECHO7", "HUDRunTimerZero", "00:00")
		: FormattedTime;

	if (RunTimerText)
	{
		RunTimerText->SetText(FText::Format(
			NSLOCTEXT("ECHO7", "HUDRunTimerFormat", "TIME: {0}"),
			CachedRunTimerText));
	}
}

void UECHO7HUDWidget::BuildLayout()
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

	auto AddTextBlock = [this, RootCanvas](TObjectPtr<UTextBlock>& TextBlock, FName Name, const FText& Text, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, ETextJustify::Type Justification)
	{
		const bool bHadTextBlock = TextBlock != nullptr;
		if (!TextBlock)
		{
			TextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(Name));
		}
		if (TextBlock)
		{
			if (!bHadTextBlock && TextBlock->GetText().IsEmpty())
			{
				TextBlock->SetText(Text);
			}
		}
		else
		{
			TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
			TextBlock->SetText(Text);
		}

		TextBlock->SetJustification(Justification);
		TextBlock->SetAutoWrapText(true);

		if (TextBlock->GetParent() != RootCanvas)
		{
			TextBlock->RemoveFromParent();
			RootCanvas->AddChildToCanvas(TextBlock);
		}

		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(TextBlock->Slot))
		{
			Slot->SetAnchors(Anchors);
			Slot->SetAlignment(Alignment);
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	};

	AddTextBlock(ObjectiveHeaderText, TEXT("ObjectiveHeader"), NSLOCTEXT("ECHO7", "HUDObjectiveHeader", "NHIỆM VỤ"), FAnchors(0.0f, 0.0f), FVector2D::ZeroVector, FVector2D(36.0f, 34.0f), FVector2D(680.0f, 26.0f), ETextJustify::Left);
	AddTextBlock(ObjectiveText, TEXT("ObjectiveText"), FText::GetEmpty(), FAnchors(0.0f, 0.0f), FVector2D::ZeroVector, FVector2D(36.0f, 64.0f), FVector2D(680.0f, 64.0f), ETextJustify::Left);

	AddTextBlock(RecoveryHeaderText, TEXT("RecoveryHeader"), NSLOCTEXT("ECHO7", "HUDRecoveryHeader", "KHÔI PHỤC HỆ THỐNG"), FAnchors(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(-36.0f, 34.0f), FVector2D(460.0f, 26.0f), ETextJustify::Right);
	AddTextBlock(RecoveryText, TEXT("RecoveryText"), NSLOCTEXT("ECHO7", "HUDInitialProgress", "0/3"), FAnchors(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(-36.0f, 64.0f), FVector2D(460.0f, 38.0f), ETextJustify::Right);

	auto ConfigureHUDText = [](UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, bool bAutoWrap, float WrapAt)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		TextBlock->SetShadowOffset(FVector2D(1.0f, 2.0f));
		TextBlock->SetAutoWrapText(bAutoWrap);
		TextBlock->SetWrapTextAt(WrapAt);
	};

	ConfigureHUDText(ObjectiveHeaderText, 18, FLinearColor(0.45f, 0.88f, 1.0f, 1.0f), false, 0.0f);
	ConfigureHUDText(ObjectiveText, 22, FLinearColor::White, true, 680.0f);
	ConfigureHUDText(RecoveryHeaderText, 18, FLinearColor(0.45f, 0.88f, 1.0f, 1.0f), false, 0.0f);
	ConfigureHUDText(RecoveryText, 26, FLinearColor::White, false, 0.0f);

	if (!MessageLinesContainer)
	{
		MessageLinesContainer = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("MessageLinesContainer")));
	}
	if (!MessageLinesContainer)
	{
		MessageLinesContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MessageLinesContainer"));
	}
	if (MessageLinesContainer->GetParent() != RootCanvas)
	{
		MessageLinesContainer->RemoveFromParent();
		RootCanvas->AddChildToCanvas(MessageLinesContainer);
	}
	if (UCanvasPanelSlot* MessageSlot = Cast<UCanvasPanelSlot>(MessageLinesContainer->Slot))
	{
		MessageSlot->SetAnchors(FAnchors(0.5f, 0.70f));
		MessageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MessageSlot->SetPosition(FVector2D::ZeroVector);
		MessageSlot->SetAutoSize(true);
	}
	BuildMessageLineRows();

	if (!InteractionPromptPanel)
	{
		InteractionPromptPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("InteractionPromptPanel")));
	}

	if (!InteractionPromptPanel)
	{
		InteractionPromptPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InteractionPromptPanel"));
	}
	if (InteractionPromptPanel->GetParent() != RootCanvas)
	{
		InteractionPromptPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(InteractionPromptPanel);
	}
	if (UCanvasPanelSlot* InteractionPromptSlot = Cast<UCanvasPanelSlot>(InteractionPromptPanel->Slot))
	{
		InteractionPromptSlot->SetAnchors(FAnchors(0.5f, 0.84f));
		InteractionPromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		InteractionPromptSlot->SetPosition(FVector2D(0.0f, -22.0f));
		InteractionPromptSlot->SetAutoSize(true);
		InteractionPromptSlot->SetZOrder(15);
	}

	InteractionPromptPanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.0f, 0.025f, 0.065f, 0.84f),
		8.0f,
		FLinearColor(0.16f, 0.84f, 1.0f, 0.96f),
		1.25f));
	InteractionPromptPanel->SetPadding(FMargin(22.0f, 11.0f));
	InteractionPromptPanel->SetHorizontalAlignment(HAlign_Center);
	InteractionPromptPanel->SetVerticalAlignment(VAlign_Center);
	InteractionPromptPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!InteractionPromptText)
	{
		InteractionPromptText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("InteractionPromptText")));
	}

	if (!InteractionPromptText)
	{
		InteractionPromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InteractionPromptText"));
	}

	if (InteractionPromptText->GetParent() != InteractionPromptPanel)
	{
		InteractionPromptText->RemoveFromParent();
		InteractionPromptPanel->SetContent(InteractionPromptText);
	}

	FSlateFontInfo InteractionPromptFont = InteractionPromptText->GetFont();
	InteractionPromptFont.Size = 26;
	InteractionPromptText->SetFont(InteractionPromptFont);
	InteractionPromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.97f, 1.0f, 1.0f)));
	InteractionPromptText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
	InteractionPromptText->SetShadowOffset(FVector2D(1.5f, 2.0f));
	InteractionPromptText->SetJustification(ETextJustify::Center);
	InteractionPromptText->SetAutoWrapText(true);
	InteractionPromptText->SetWrapTextAt(800.0f);
	ClearInteractionPrompt();

	if (!RunTimerPanel)
	{
		RunTimerPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("RunTimerPanel")));
	}
	if (!RunTimerPanel)
	{
		RunTimerPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RunTimerPanel"));
	}
	if (RunTimerPanel->GetParent() != RootCanvas)
	{
		RunTimerPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(RunTimerPanel);
	}
	if (UCanvasPanelSlot* RunTimerSlot = Cast<UCanvasPanelSlot>(RunTimerPanel->Slot))
	{
		RunTimerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		RunTimerSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		RunTimerSlot->SetPosition(FVector2D(0.0f, -28.0f));
		RunTimerSlot->SetAutoSize(true);
	}
	RunTimerPanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.0f, 0.025f, 0.055f, 0.78f),
		6.0f,
		FLinearColor(0.18f, 0.82f, 1.0f, 0.90f),
		1.0f));
	RunTimerPanel->SetPadding(FMargin(16.0f, 8.0f));
	RunTimerPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!RunTimerText)
	{
		RunTimerText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("RunTimerText")));
	}
	if (!RunTimerText)
	{
		RunTimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RunTimerText"));
	}
	if (RunTimerText->GetParent() != RunTimerPanel)
	{
		RunTimerText->RemoveFromParent();
		RunTimerPanel->SetContent(RunTimerText);
	}

	FSlateFontInfo RunTimerFont = RunTimerText->GetFont();
	RunTimerFont.Size = 20;
	RunTimerText->SetFont(RunTimerFont);
	RunTimerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.94f, 1.0f, 1.0f)));
	RunTimerText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	RunTimerText->SetShadowOffset(FVector2D(1.0f, 2.0f));
	RunTimerText->SetJustification(ETextJustify::Center);
	RunTimerText->SetAutoWrapText(false);
	RunTimerText->SetWrapTextAt(0.0f);
	SetRunTimerText(CachedRunTimerText);

	ClearMessage();
}

void UECHO7HUDWidget::BuildMessageLineRows()
{
	if (!WidgetTree || !MessageLinesContainer)
	{
		return;
	}

	while (MessageLineRows.Num() < ActiveMessageLines.Num())
	{
		const int32 LineIndex = MessageLineRows.Num();
		UTextBlock* LineRow = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("MessageLine%d"), LineIndex));
		LineRow->SetJustification(ETextJustify::Center);
		LineRow->SetAutoWrapText(false);
		LineRow->SetClipping(EWidgetClipping::ClipToBounds);
		LineRow->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		LineRow->SetShadowOffset(FVector2D(1.0f, 2.0f));
		MessageLineRows.Add(LineRow);
	}

	for (int32 LineIndex = 0; LineIndex < MessageLineRows.Num(); ++LineIndex)
	{
		UTextBlock* LineRow = MessageLineRows[LineIndex];
		if (!LineRow)
		{
			continue;
		}
		if (LineRow->GetParent() != MessageLinesContainer)
		{
			LineRow->RemoveFromParent();
			MessageLinesContainer->AddChildToVerticalBox(LineRow);
		}
		if (UVerticalBoxSlot* LineSlot = Cast<UVerticalBoxSlot>(LineRow->Slot))
		{
			LineSlot->SetHorizontalAlignment(HAlign_Center);
			LineSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
	}
}

void UECHO7HUDWidget::ShowMessageInternal(const TArray<FText>& Lines, float Duration, bool bIsWarning, int32 FontSize, float VerticalAnchor)
{
	if (!MessageLinesContainer || Lines.IsEmpty())
	{
		return;
	}

	ActiveMessageLines = Lines;
	BuildMessageLineRows();
	if (UCanvasPanelSlot* MessageSlot = Cast<UCanvasPanelSlot>(MessageLinesContainer->Slot))
	{
		MessageSlot->SetAnchors(FAnchors(0.5f, VerticalAnchor));
		MessageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
	for (int32 LineIndex = 0; LineIndex < MessageLineRows.Num(); ++LineIndex)
	{
		UTextBlock* LineRow = MessageLineRows[LineIndex];
		if (!LineRow)
		{
			continue;
		}

		const bool bHasLine = ActiveMessageLines.IsValidIndex(LineIndex);
		LineRow->SetText(bHasLine ? ActiveMessageLines[LineIndex] : FText::GetEmpty());
		FSlateFontInfo MessageFont = LineRow->GetFont();
		MessageFont.Size = FontSize != INDEX_NONE ? FontSize : (bIsWarning ? 24 : 22);
		LineRow->SetFont(MessageFont);
		LineRow->SetColorAndOpacity(FSlateColor(bIsWarning ? FLinearColor(1.0f, 0.34f, 0.30f, 1.0f) : FLinearColor::White));
		LineRow->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, FontSize != INDEX_NONE ? 1.0f : 0.85f));
		LineRow->SetShadowOffset(FontSize != INDEX_NONE ? FVector2D(2.0f, 3.0f) : FVector2D(1.0f, 2.0f));
		if (UVerticalBoxSlot* LineSlot = Cast<UVerticalBoxSlot>(LineRow->Slot))
		{
			LineSlot->SetPadding(FMargin(0.0f, FontSize != INDEX_NONE ? 6.0f : 2.0f));
		}
		LineRow->SetVisibility(bHasLine ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	MessageLinesContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MessageTimer, this, &UECHO7HUDWidget::ClearMessage, FMath::Max(Duration, 0.01f), false);
	}
}

void UECHO7HUDWidget::ClearMessage()
{
	if (MessageLinesContainer)
	{
		MessageLinesContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
}
