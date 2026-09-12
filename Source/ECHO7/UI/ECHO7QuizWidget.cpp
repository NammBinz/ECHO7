// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ECHO7QuizWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace ECHO7Quiz
{
	constexpr float GlitchPulseMinimumInterval = 0.035f;
	constexpr float GlitchPulseMaximumInterval = 0.075f;
	constexpr float EnergyChargeUpdateInterval = 0.02f;
}

void UECHO7QuizWidget::InitializeQuiz(const TArray<FQuizQuestion>& InQuestions)
{
	QuizQuestions = InQuestions;
	CurrentQuestionIndex = 0;
	ChargedCellCount = 0;
	ChargingCellIndex = INDEX_NONE;
	bQuizInitialized = true;
	bQuestionContentVisible = false;
	bQuestionGlitchActive = false;
	bShowSuccessAfterGlitch = false;
	bAwaitingResponse = false;
	bCompletionPending = false;
	BuildEnergyCells();
	RefreshQuestion();

	if (IsInViewport())
	{
		BeginQuestionPresentation();
	}
}

void UECHO7QuizWidget::ConfigurePresentation(
	float InEnergyChargeDuration,
	float InQuestionGlitchDuration,
	USoundBase* InQuestionAppearSound,
	USoundBase* InCorrectAnswerSound,
	USoundBase* InIncorrectAnswerSound,
	USoundBase* InEnergyChargeSound,
	USoundBase* InQuizGlitchSound)
{
	EnergyChargeDuration = FMath::Max(0.05f, InEnergyChargeDuration);
	QuestionGlitchDuration = FMath::Max(0.05f, InQuestionGlitchDuration);
	QuestionAppearSound = InQuestionAppearSound;
	CorrectAnswerSound = InCorrectAnswerSound;
	IncorrectAnswerSound = InIncorrectAnswerSound;
	EnergyChargeSound = InEnergyChargeSound;
	QuizGlitchSound = InQuizGlitchSound;
}

TSharedRef<SWidget> UECHO7QuizWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildLayout();
	return Super::RebuildWidget();
}

void UECHO7QuizWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshQuestion();
	if (bQuizInitialized)
	{
		BeginQuestionPresentation();
	}
}

void UECHO7QuizWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
		World->GetTimerManager().ClearTimer(EnergyChargeTimer);
		World->GetTimerManager().ClearTimer(QuestionGlitchPulseTimer);
		World->GetTimerManager().ClearTimer(QuestionGlitchEndTimer);
		World->GetTimerManager().ClearTimer(FeedbackFlashTimer);
	}

	Super::NativeDestruct();
}

FReply UECHO7QuizWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape && !bCompletionPending)
	{
		CancelQuiz();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UECHO7QuizWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!DimBackdrop)
	{
		DimBackdrop = Cast<UBorder>(WidgetTree->FindWidget(TEXT("DimBackdrop")));
	}
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
		BackdropSlot->SetZOrder(0);
	}
	DimBackdrop->SetBrushColor(FLinearColor(0.002f, 0.008f, 0.025f, 0.82f));
	DimBackdrop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!QuizPanel)
	{
		QuizPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("QuizPanel")));
	}
	if (!QuizPanel)
	{
		QuizPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuizPanel"));
	}
	if (QuizPanel->GetParent() != RootCanvas)
	{
		QuizPanel->RemoveFromParent();
		RootCanvas->AddChildToCanvas(QuizPanel);
	}
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(QuizPanel->Slot))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(900.0f, 580.0f));
		PanelSlot->SetZOrder(5);
	}
	QuizPanel->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.008f, 0.025f, 0.070f, 0.96f),
		10.0f,
		FLinearColor(0.16f, 0.84f, 1.0f, 0.90f),
		1.5f));
	QuizPanel->SetPadding(FMargin(34.0f, 28.0f));

	if (!ContentBox)
	{
		ContentBox = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("ContentBox")));
	}

	if (!ContentBox)
	{
		ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	}

	if (QuizPanel->GetContent() != ContentBox)
	{
		ContentBox->RemoveFromParent();
		QuizPanel->SetContent(ContentBox);
	}

	auto ConfigureText = [this](TObjectPtr<UTextBlock>& TextBlock, FName Name, int32 FontSize, bool bAutoWrap = true)
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
		TextBlock->SetAutoWrapText(bAutoWrap);
		TextBlock->SetWrapTextAt(bAutoWrap ? 760.0f : 0.0f);
		TextBlock->SetClipping(EWidgetClipping::Inherit);
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

	ConfigureText(TitleText, TEXT("TitleText"), 32, false);
	TitleText->SetText(NSLOCTEXT("ECHO7", "QuizTitle", "XÁC MINH BẢO MẬT"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.92f, 1.0f, 1.0f)));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.12f, 0.18f, 1.0f));
	TitleText->SetShadowOffset(FVector2D(1.0f, 3.0f));
	AddToContent(TitleText, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	ConfigureText(SubtitleText, TEXT("SubtitleText"), 15, false);
	SubtitleText->SetText(NSLOCTEXT("ECHO7", "QuizSubtitle", "GIAO THỨC KIỂM TRA TRUY CẬP"));
	SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.68f, 0.78f, 1.0f)));
	AddToContent(SubtitleText, FMargin(0.0f, 0.0f, 0.0f, 20.0f));

	ConfigureText(ProgressLabelText, TEXT("ProgressLabelText"), 13, false);
	ProgressLabelText->SetText(NSLOCTEXT("ECHO7", "QuizProgressLabel", "TIẾN TRÌNH XÁC MINH"));
	ProgressLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.78f, 0.90f, 1.0f)));
	AddToContent(ProgressLabelText, FMargin(0.0f, 0.0f, 0.0f, 7.0f));

	if (!EnergyCellsContainer)
	{
		EnergyCellsContainer = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("EnergyCellsContainer")));
	}
	if (!EnergyCellsContainer)
	{
		EnergyCellsContainer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EnergyCellsContainer"));
	}
	AddToContent(EnergyCellsContainer, FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	if (UVerticalBoxSlot* ProgressSlot = Cast<UVerticalBoxSlot>(EnergyCellsContainer->Slot))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Center);
	}

	ConfigureText(CounterText, TEXT("CounterText"), 19, false);
	CounterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.92f, 1.0f, 1.0f)));
	AddToContent(CounterText, FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	if (!QuestionLinesContainer)
	{
		QuestionLinesContainer = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("QuestionLinesContainer")));
	}
	if (!QuestionLinesContainer)
	{
		QuestionLinesContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestionLinesContainer"));
	}
	if (!QuestionBox)
	{
		QuestionBox = Cast<UBorder>(WidgetTree->FindWidget(TEXT("QuestionBox")));
	}
	if (!QuestionBox)
	{
		QuestionBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestionBox"));
	}
	QuestionBox->SetBrush(FSlateRoundedBoxBrush(
		FLinearColor(0.012f, 0.060f, 0.115f, 0.95f),
		6.0f,
		FLinearColor(0.12f, 0.48f, 0.68f, 0.78f),
		1.0f));
	QuestionBox->SetPadding(FMargin(24.0f, 18.0f));
	if (QuestionLinesContainer->GetParent() != QuestionBox)
	{
		QuestionLinesContainer->RemoveFromParent();
		QuestionBox->SetContent(QuestionLinesContainer);
	}
	AddToContent(QuestionBox, FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	if (!AnswerGrid)
	{
		AnswerGrid = Cast<UUniformGridPanel>(WidgetTree->FindWidget(TEXT("AnswerGrid")));
	}
	if (!AnswerGrid)
	{
		AnswerGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("AnswerGrid"));
	}
	AnswerGrid->SetMinDesiredSlotWidth(395.0f);
	AnswerGrid->SetMinDesiredSlotHeight(58.0f);
	AnswerGrid->SetSlotPadding(FMargin(6.0f));
	AddToContent(AnswerGrid, FMargin(0.0f, 0.0f, 0.0f, 14.0f));

	auto ConfigureAnswerButton = [this, &ConfigureText](int32 AnswerIndex, TObjectPtr<UButton>& Button, TObjectPtr<UTextBlock>& TextBlock, FName ButtonName, FName TextName)
	{
		if (!Button)
		{
			Button = Cast<UButton>(WidgetTree->FindWidget(ButtonName));
		}

		if (!Button)
		{
			Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		}

		ConfigureText(TextBlock, TextName, 18, false);
		TextBlock->SetJustification(ETextJustify::Center);
		if (TextBlock->GetParent() != Button)
		{
			TextBlock->RemoveFromParent();
			Button->SetContent(TextBlock);
		}

		ApplyAnswerButtonStyle(
			Button,
			FLinearColor(0.025f, 0.115f, 0.195f, 0.98f),
			FLinearColor(0.15f, 0.47f, 0.66f, 0.88f));

		while (AnswerButtonContainers.Num() <= AnswerIndex)
		{
			const int32 ContainerIndex = AnswerButtonContainers.Num();
			AnswerButtonContainers.Add(WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				*FString::Printf(TEXT("AnswerButtonContainer%d"), ContainerIndex)));
		}

		USizeBox* ButtonContainer = AnswerButtonContainers[AnswerIndex];
		ButtonContainer->SetWidthOverride(395.0f);
		ButtonContainer->SetHeightOverride(58.0f);
		if (Button->GetParent() != ButtonContainer)
		{
			Button->RemoveFromParent();
			ButtonContainer->SetContent(Button);
		}
		if (ButtonContainer->GetParent() != AnswerGrid)
		{
			ButtonContainer->RemoveFromParent();
			AnswerGrid->AddChildToUniformGrid(ButtonContainer, AnswerIndex / 2, AnswerIndex % 2);
		}
		if (UUniformGridSlot* ButtonSlot = Cast<UUniformGridSlot>(ButtonContainer->Slot))
		{
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}
	};

	ConfigureAnswerButton(0, AnswerAButton, AnswerAText, TEXT("AnswerAButton"), TEXT("AnswerAText"));
	ConfigureAnswerButton(1, AnswerBButton, AnswerBText, TEXT("AnswerBButton"), TEXT("AnswerBText"));
	ConfigureAnswerButton(2, AnswerCButton, AnswerCText, TEXT("AnswerCButton"), TEXT("AnswerCText"));
	ConfigureAnswerButton(3, AnswerDButton, AnswerDText, TEXT("AnswerDButton"), TEXT("AnswerDText"));
	AnswerAButton->OnClicked.AddUniqueDynamic(this, &UECHO7QuizWidget::HandleAnswerA);
	AnswerBButton->OnClicked.AddUniqueDynamic(this, &UECHO7QuizWidget::HandleAnswerB);
	AnswerCButton->OnClicked.AddUniqueDynamic(this, &UECHO7QuizWidget::HandleAnswerC);
	AnswerDButton->OnClicked.AddUniqueDynamic(this, &UECHO7QuizWidget::HandleAnswerD);

	ConfigureText(FeedbackText, TEXT("FeedbackText"), 19, false);
	AddToContent(FeedbackText, FMargin(0.0f, 2.0f, 0.0f, 4.0f));
	FeedbackText->SetVisibility(ESlateVisibility::Collapsed);

	ConfigureText(FooterText, TEXT("FooterText"), 13, false);
	FooterText->SetText(NSLOCTEXT("ECHO7", "QuizFooter", "[ESC] Thoát"));
	FooterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.40f, 0.60f, 0.70f, 1.0f)));
	AddToContent(FooterText, FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	if (!GlitchOverlay)
	{
		GlitchOverlay = Cast<UBorder>(WidgetTree->FindWidget(TEXT("GlitchOverlay")));
	}
	if (!GlitchOverlay)
	{
		GlitchOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GlitchOverlay"));
	}
	if (GlitchOverlay->GetParent() != RootCanvas)
	{
		GlitchOverlay->RemoveFromParent();
		RootCanvas->AddChildToCanvas(GlitchOverlay);
	}
	if (UCanvasPanelSlot* GlitchSlot = Cast<UCanvasPanelSlot>(GlitchOverlay->Slot))
	{
		GlitchSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		GlitchSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		GlitchSlot->SetPosition(FVector2D::ZeroVector);
		GlitchSlot->SetSize(FVector2D(900.0f, 580.0f));
		GlitchSlot->SetZOrder(10);
	}
	GlitchOverlay->SetVisibility(ESlateVisibility::Collapsed);

	while (GlitchStrips.Num() < 3)
	{
		const int32 StripIndex = GlitchStrips.Num();
		GlitchStrips.Add(WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("QuizGlitchStrip%d"), StripIndex)));
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
			StripSlot->SetSize(FVector2D(900.0f, 12.0f));
			StripSlot->SetZOrder(11 + StripIndex);
		}
		Strip->SetVisibility(ESlateVisibility::Collapsed);
	}

	BuildEnergyCells();
	BuildQuestionLineRows();
	SetQuestionContentVisible(bQuestionContentVisible);
	RefreshQuestion();
}

void UECHO7QuizWidget::RefreshQuestion()
{
	if (!QuizQuestions.IsValidIndex(CurrentQuestionIndex) || !CounterText || !AnswerAText || !AnswerBText || !AnswerCText || !AnswerDText || !FeedbackText)
	{
		return;
	}

	const FQuizQuestion& Question = QuizQuestions[CurrentQuestionIndex];
	BuildEnergyCells();
	CounterText->SetText(FText::Format(
		NSLOCTEXT("ECHO7", "QuizCounter", "CÂU {0}/{1}"),
		FText::AsNumber(CurrentQuestionIndex + 1),
		FText::AsNumber(QuizQuestions.Num())));
	BuildQuestionLineRows();
	const TArray<FText>* DisplayLines = &Question.QuestionLines;
	TArray<FText> LegacyQuestionLine;
	if (DisplayLines->IsEmpty())
	{
		LegacyQuestionLine.Add(Question.QuestionText);
		DisplayLines = &LegacyQuestionLine;
	}
	for (int32 LineIndex = 0; LineIndex < QuestionLineRows.Num(); ++LineIndex)
	{
		UTextBlock* LineRow = QuestionLineRows[LineIndex];
		if (!LineRow)
		{
			continue;
		}

		if (DisplayLines->IsValidIndex(LineIndex))
		{
			LineRow->SetText((*DisplayLines)[LineIndex]);
			LineRow->SetVisibility(bQuestionContentVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		else
		{
			LineRow->SetText(FText::GetEmpty());
			LineRow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	AnswerAText->SetText(FText::Format(NSLOCTEXT("ECHO7", "QuizAnswerA", "[A] {0}"), Question.AnswerA));
	AnswerBText->SetText(FText::Format(NSLOCTEXT("ECHO7", "QuizAnswerB", "[B] {0}"), Question.AnswerB));
	AnswerCText->SetText(FText::Format(NSLOCTEXT("ECHO7", "QuizAnswerC", "[C] {0}"), Question.AnswerC));
	AnswerDText->SetText(FText::Format(NSLOCTEXT("ECHO7", "QuizAnswerD", "[D] {0}"), Question.AnswerD));
	ApplyAnswerButtonStyle(AnswerAButton, FLinearColor(0.025f, 0.115f, 0.195f, 0.98f), FLinearColor(0.15f, 0.47f, 0.66f, 0.88f));
	ApplyAnswerButtonStyle(AnswerBButton, FLinearColor(0.025f, 0.115f, 0.195f, 0.98f), FLinearColor(0.15f, 0.47f, 0.66f, 0.88f));
	ApplyAnswerButtonStyle(AnswerCButton, FLinearColor(0.025f, 0.115f, 0.195f, 0.98f), FLinearColor(0.15f, 0.47f, 0.66f, 0.88f));
	ApplyAnswerButtonStyle(AnswerDButton, FLinearColor(0.025f, 0.115f, 0.195f, 0.98f), FLinearColor(0.15f, 0.47f, 0.66f, 0.88f));
	FeedbackText->SetText(FText::GetEmpty());
	FeedbackText->SetVisibility(ESlateVisibility::Collapsed);
	SetAnswerButtonsEnabled(bQuestionContentVisible && !bAwaitingResponse && !bCompletionPending);
}

void UECHO7QuizWidget::BuildQuestionLineRows()
{
	if (!QuestionLinesContainer || !QuizQuestions.IsValidIndex(CurrentQuestionIndex))
	{
		return;
	}

	const FQuizQuestion& Question = QuizQuestions[CurrentQuestionIndex];
	const int32 RequiredRowCount = Question.QuestionLines.IsEmpty() ? 1 : Question.QuestionLines.Num();
	while (QuestionLineRows.Num() < RequiredRowCount)
	{
		const int32 RowIndex = QuestionLineRows.Num();
		UTextBlock* LineRow = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("QuestionLine%d"), RowIndex));
		FSlateFontInfo Font = LineRow->GetFont();
		Font.Size = 24;
		LineRow->SetFont(Font);
		LineRow->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		LineRow->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
		LineRow->SetShadowOffset(FVector2D(1.0f, 2.0f));
		LineRow->SetJustification(ETextJustify::Center);
		LineRow->SetAutoWrapText(true);
		LineRow->SetWrapTextAt(700.0f);
		LineRow->SetClipping(EWidgetClipping::Inherit);
		QuestionLineRows.Add(LineRow);
	}

	for (UTextBlock* LineRow : QuestionLineRows)
	{
		if (LineRow->GetParent() != QuestionLinesContainer)
		{
			LineRow->RemoveFromParent();
			QuestionLinesContainer->AddChildToVerticalBox(LineRow);
		}
		if (UVerticalBoxSlot* LineSlot = Cast<UVerticalBoxSlot>(LineRow->Slot))
		{
			LineSlot->SetHorizontalAlignment(HAlign_Center);
			LineSlot->SetPadding(FMargin(0.0f, 3.0f));
		}
	}
}

void UECHO7QuizWidget::BuildEnergyCells()
{
	if (!EnergyCellsContainer || !WidgetTree)
	{
		return;
	}

	const int32 RequiredCellCount = QuizQuestions.Num();
	while (EnergyCells.Num() < RequiredCellCount)
	{
		const int32 CellIndex = EnergyCells.Num();
		USizeBox* CellContainer = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("EnergyCellContainer%d"), CellIndex));
		CellContainer->SetWidthOverride(240.0f);
		CellContainer->SetHeightOverride(22.0f);
		UProgressBar* Cell = WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(),
			*FString::Printf(TEXT("EnergyCell%d"), CellIndex));
		Cell->SetPercent(0.0f);
		CellContainer->SetContent(Cell);
		EnergyCellContainers.Add(CellContainer);
		EnergyCells.Add(Cell);
	}

	for (int32 CellIndex = 0; CellIndex < EnergyCells.Num(); ++CellIndex)
	{
		USizeBox* CellContainer = EnergyCellContainers[CellIndex];
		if (CellContainer->GetParent() != EnergyCellsContainer)
		{
			CellContainer->RemoveFromParent();
			EnergyCellsContainer->AddChildToHorizontalBox(CellContainer);
		}
		if (UHorizontalBoxSlot* CellSlot = Cast<UHorizontalBoxSlot>(CellContainer->Slot))
		{
			CellSlot->SetVerticalAlignment(VAlign_Center);
			CellSlot->SetPadding(FMargin(5.0f, 0.0f));
		}
		SetEnergyCellProgress(CellIndex, CellIndex < ChargedCellCount ? 1.0f : 0.0f, CellIndex < ChargedCellCount);
	}
}

void UECHO7QuizWidget::SetEnergyCellProgress(int32 CellIndex, float Progress, bool bCharged)
{
	if (!EnergyCells.IsValidIndex(CellIndex) || !EnergyCells[CellIndex])
	{
		return;
	}

	UProgressBar* Cell = EnergyCells[CellIndex];
	const float SafeProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	const bool bIsCurrent = !bCharged && !bCompletionPending && CellIndex == CurrentQuestionIndex;
	const FLinearColor BorderColor = bCharged
		? FLinearColor(0.22f, 1.0f, 0.65f, 1.0f)
		: (bIsCurrent ? FLinearColor(0.20f, 0.92f, 1.0f, 1.0f) : FLinearColor(0.16f, 0.25f, 0.32f, 0.85f));
	const FLinearColor FillColor = bCharged
		? FLinearColor(0.12f, 0.92f, 0.58f, 1.0f)
		: (bIsCurrent ? FLinearColor(0.05f, 0.76f, 1.0f, 1.0f) : FLinearColor(0.04f, 0.15f, 0.22f, 1.0f));
	FProgressBarStyle CellStyle;
	CellStyle.SetBackgroundImage(FSlateRoundedBoxBrush(
		FLinearColor(0.005f, 0.020f, 0.045f, 0.98f),
		3.0f,
		BorderColor,
		bIsCurrent || bCharged ? 2.0f : 1.0f));
	CellStyle.SetFillImage(FSlateRoundedBoxBrush(
		FillColor,
		2.0f));
	Cell->SetWidgetStyle(CellStyle);
	Cell->SetFillColorAndOpacity(FLinearColor::White);
	Cell->SetPercent(SafeProgress);
}

void UECHO7QuizWidget::ApplyAnswerButtonStyle(
	UButton* Button,
	const FLinearColor& BackgroundColor,
	const FLinearColor& BorderColor) const
{
	if (!Button)
	{
		return;
	}

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateRoundedBoxBrush(BackgroundColor, 5.0f, BorderColor, 1.2f));
	ButtonStyle.SetHovered(FSlateRoundedBoxBrush(
		FLinearColor(
			FMath::Min(BackgroundColor.R + 0.035f, 1.0f),
			FMath::Min(BackgroundColor.G + 0.120f, 1.0f),
			FMath::Min(BackgroundColor.B + 0.150f, 1.0f),
			BackgroundColor.A),
		5.0f,
		FLinearColor(0.40f, 0.96f, 1.0f, 1.0f),
		2.0f));
	ButtonStyle.SetPressed(FSlateRoundedBoxBrush(
		FLinearColor(0.06f, 0.68f, 0.88f, 1.0f),
		5.0f,
		FLinearColor(0.80f, 1.0f, 1.0f, 1.0f),
		2.0f));
	ButtonStyle.SetDisabled(FSlateRoundedBoxBrush(
		FLinearColor(BackgroundColor.R, BackgroundColor.G, BackgroundColor.B, 0.72f),
		5.0f,
		FLinearColor(BorderColor.R, BorderColor.G, BorderColor.B, 0.72f),
		1.0f));
	Button->SetStyle(ButtonStyle);
}

void UECHO7QuizWidget::SetAnswerButtonFeedback(EQuizAnswer SelectedAnswer, bool bCorrect)
{
	UButton* SelectedButton = nullptr;
	switch (SelectedAnswer)
	{
	case EQuizAnswer::A:
		SelectedButton = AnswerAButton;
		break;
	case EQuizAnswer::B:
		SelectedButton = AnswerBButton;
		break;
	case EQuizAnswer::C:
		SelectedButton = AnswerCButton;
		break;
	case EQuizAnswer::D:
		SelectedButton = AnswerDButton;
		break;
	default:
		break;
	}

	if (bCorrect)
	{
		ApplyAnswerButtonStyle(SelectedButton, FLinearColor(0.035f, 0.34f, 0.20f, 1.0f), FLinearColor(0.25f, 1.0f, 0.62f, 1.0f));
	}
	else
	{
		ApplyAnswerButtonStyle(SelectedButton, FLinearColor(0.36f, 0.045f, 0.065f, 1.0f), FLinearColor(1.0f, 0.26f, 0.30f, 1.0f));
	}
}

void UECHO7QuizWidget::SetQuestionContentVisible(bool bVisible)
{
	bQuestionContentVisible = bVisible;
	const ESlateVisibility ContentVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (CounterText)
	{
		CounterText->SetVisibility(ContentVisibility);
	}
	if (QuestionBox)
	{
		QuestionBox->SetVisibility(ContentVisibility);
	}
	if (AnswerGrid)
	{
		AnswerGrid->SetVisibility(ContentVisibility);
	}
	SetAnswerButtonsEnabled(bVisible && !bAwaitingResponse && !bCompletionPending);
	RefreshQuestion();
}

void UECHO7QuizWidget::BeginQuestionPresentation()
{
	if (!bQuizInitialized || !QuizQuestions.IsValidIndex(CurrentQuestionIndex) || bQuestionGlitchActive || bCompletionPending)
	{
		return;
	}

	bAwaitingResponse = true;
	SetQuestionContentVisible(false);
	StartQuestionGlitch(false);
}

void UECHO7QuizWidget::StartQuestionGlitch(bool bShowSuccessWhenFinished)
{
	bQuestionGlitchActive = true;
	bShowSuccessAfterGlitch = bShowSuccessWhenFinished;
	ApplyQuestionGlitchPulse();
	PlayOptionalSound(QuizGlitchSound.Get());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			QuestionGlitchEndTimer,
			this,
			&UECHO7QuizWidget::FinishQuestionGlitch,
			QuestionGlitchDuration,
			false);
	}
	else
	{
		FinishQuestionGlitch();
	}
}

void UECHO7QuizWidget::ApplyQuestionGlitchPulse()
{
	if (!bQuestionGlitchActive)
	{
		return;
	}

	const FLinearColor PulseColor = FMath::RandBool()
		? FLinearColor(0.02f, 0.36f, 0.52f, 0.42f)
		: FLinearColor(0.08f, 0.72f, 0.92f, 0.30f);
	if (GlitchOverlay)
	{
		GlitchOverlay->SetBrush(FSlateRoundedBoxBrush(PulseColor, 4.0f));
		GlitchOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (QuizPanel)
	{
		QuizPanel->SetRenderTranslation(FVector2D(FMath::FRandRange(-7.0f, 7.0f), FMath::FRandRange(-4.0f, 4.0f)));
		QuizPanel->SetRenderOpacity(FMath::FRandRange(0.82f, 1.0f));
	}

	for (UBorder* Strip : GlitchStrips)
	{
		if (!Strip)
		{
			continue;
		}

		Strip->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.45f, 0.95f, 1.0f, FMath::FRandRange(0.20f, 0.62f)), 1.0f));
		Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* StripSlot = Cast<UCanvasPanelSlot>(Strip->Slot))
		{
			StripSlot->SetPosition(FVector2D(FMath::FRandRange(-18.0f, 18.0f), FMath::FRandRange(-210.0f, 210.0f)));
			StripSlot->SetSize(FVector2D(FMath::FRandRange(440.0f, 820.0f), FMath::FRandRange(3.0f, 12.0f)));
		}
	}

	ScheduleNextQuestionGlitchPulse();
}

void UECHO7QuizWidget::ScheduleNextQuestionGlitchPulse()
{
	if (!bQuestionGlitchActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			QuestionGlitchPulseTimer,
			this,
			&UECHO7QuizWidget::ApplyQuestionGlitchPulse,
			FMath::FRandRange(ECHO7Quiz::GlitchPulseMinimumInterval, ECHO7Quiz::GlitchPulseMaximumInterval),
			false);
	}
}

void UECHO7QuizWidget::FinishQuestionGlitch()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QuestionGlitchPulseTimer);
	}

	bQuestionGlitchActive = false;
	if (QuizPanel)
	{
		QuizPanel->SetRenderTranslation(FVector2D::ZeroVector);
		QuizPanel->SetRenderOpacity(1.0f);
	}
	if (GlitchOverlay)
	{
		GlitchOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UBorder* Strip : GlitchStrips)
	{
		if (Strip)
		{
			Strip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (bShowSuccessAfterGlitch)
	{
		FinishQuiz();
		return;
	}

	bAwaitingResponse = false;
	SetQuestionContentVisible(true);
	SetAnswerButtonsEnabled(true);
	PlayOptionalSound(QuestionAppearSound.Get());
}

void UECHO7QuizWidget::BeginEnergyCharge()
{
	ChargingCellIndex = CurrentQuestionIndex;
	EnergyChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SetEnergyCellProgress(ChargingCellIndex, 0.0f, false);
	PlayOptionalSound(EnergyChargeSound.Get());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EnergyChargeTimer, this, &UECHO7QuizWidget::UpdateEnergyCharge, ECHO7Quiz::EnergyChargeUpdateInterval, true);
		UpdateEnergyCharge();
	}
	else
	{
		FinishEnergyCharge();
	}
}

void UECHO7QuizWidget::UpdateEnergyCharge()
{
	if (ChargingCellIndex == INDEX_NONE || !GetWorld())
	{
		return;
	}

	const float ChargeAlpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - EnergyChargeStartTime) / EnergyChargeDuration, 0.0f, 1.0f);
	SetEnergyCellProgress(ChargingCellIndex, ChargeAlpha, ChargeAlpha >= 1.0f);
	if (ChargeAlpha >= 1.0f)
	{
		FinishEnergyCharge();
	}
}

void UECHO7QuizWidget::FinishEnergyCharge()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnergyChargeTimer);
	}

	if (ChargingCellIndex != INDEX_NONE)
	{
		SetEnergyCellProgress(ChargingCellIndex, 1.0f, true);
		ChargedCellCount = FMath::Max(ChargedCellCount, ChargingCellIndex + 1);
	}
	ChargingCellIndex = INDEX_NONE;

	if (CurrentQuestionIndex + 1 >= QuizQuestions.Num())
	{
		SetQuestionContentVisible(false);
		StartQuestionGlitch(true);
		return;
	}

	++CurrentQuestionIndex;
	BeginQuestionPresentation();
}

void UECHO7QuizWidget::TriggerFeedbackFlash(const FLinearColor& Color, float Opacity)
{
	if (bQuestionGlitchActive || !GlitchOverlay)
	{
		return;
	}

	GlitchOverlay->SetBrush(FSlateRoundedBoxBrush(FLinearColor(Color.R, Color.G, Color.B, Opacity), 4.0f));
	GlitchOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FeedbackFlashTimer, this, &UECHO7QuizWidget::ClearFeedbackFlash, 0.12f, false);
	}
}

void UECHO7QuizWidget::ClearFeedbackFlash()
{
	if (!bQuestionGlitchActive && GlitchOverlay)
	{
		GlitchOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UECHO7QuizWidget::PlayOptionalSound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UECHO7QuizWidget::HandleAnswerSelected(EQuizAnswer SelectedAnswer)
{
	if (bAwaitingResponse || !QuizQuestions.IsValidIndex(CurrentQuestionIndex))
	{
		return;
	}

	bAwaitingResponse = true;
	SetAnswerButtonsEnabled(false);

	const bool bCorrect = QuizQuestions[CurrentQuestionIndex].CorrectAnswer == SelectedAnswer;
	SetAnswerButtonFeedback(SelectedAnswer, bCorrect);
	FeedbackText->SetText(bCorrect
		? NSLOCTEXT("ECHO7", "QuizCorrectAnswer", "XÁC NHẬN CHÍNH XÁC")
		: NSLOCTEXT("ECHO7", "QuizIncorrectAnswer", "XÁC MINH THẤT BẠI"));
	FeedbackText->SetColorAndOpacity(FSlateColor(bCorrect ? FLinearColor(0.2f, 1.0f, 0.65f) : FLinearColor(1.0f, 0.3f, 0.25f)));
	FeedbackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (bCorrect)
	{
		PlayOptionalSound(CorrectAnswerSound.Get());
		TriggerFeedbackFlash(FLinearColor(0.2f, 1.0f, 0.65f), 0.16f);
		BeginEnergyCharge();
		return;
	}

	PlayOptionalSound(IncorrectAnswerSound.Get());
	TriggerFeedbackFlash(FLinearColor(1.0f, 0.3f, 0.25f), 0.18f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FeedbackTimer, this, &UECHO7QuizWidget::ResetAfterIncorrectAnswer, 0.75f, false);
	}
	else
	{
		ResetAfterIncorrectAnswer();
	}
}

void UECHO7QuizWidget::SetAnswerButtonsEnabled(bool bEnabled)
{
	if (AnswerAButton)
	{
		AnswerAButton->SetIsEnabled(bEnabled);
	}
	if (AnswerBButton)
	{
		AnswerBButton->SetIsEnabled(bEnabled);
	}
	if (AnswerCButton)
	{
		AnswerCButton->SetIsEnabled(bEnabled);
	}
	if (AnswerDButton)
	{
		AnswerDButton->SetIsEnabled(bEnabled);
	}
}

void UECHO7QuizWidget::AdvanceToNextQuestion()
{
	++CurrentQuestionIndex;
	BeginQuestionPresentation();
}

void UECHO7QuizWidget::ResetAfterIncorrectAnswer()
{
	bAwaitingResponse = false;
	if (FeedbackText)
	{
		FeedbackText->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetAnswerButtonsEnabled(true);
}

void UECHO7QuizWidget::FinishQuiz()
{
	bCompletionPending = true;
	bAwaitingResponse = true;
	SetQuestionContentVisible(false);
	FeedbackText->SetText(NSLOCTEXT("ECHO7", "QuizSuccess", "XÁC MINH THÀNH CÔNG"));
	FeedbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.65f)));
	FeedbackText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FeedbackTimer, this, &UECHO7QuizWidget::CompleteQuizAfterFeedback, 0.9f, false);
	}
	else
	{
		CompleteQuizAfterFeedback();
	}
}

void UECHO7QuizWidget::CompleteQuizAfterFeedback()
{
	OnQuizCompleted.Broadcast();
	RemoveFromParent();
}

void UECHO7QuizWidget::CancelQuiz()
{
	OnQuizCancelled.Broadcast();
	RemoveFromParent();
}

void UECHO7QuizWidget::HandleAnswerA()
{
	HandleAnswerSelected(EQuizAnswer::A);
}

void UECHO7QuizWidget::HandleAnswerB()
{
	HandleAnswerSelected(EQuizAnswer::B);
}

void UECHO7QuizWidget::HandleAnswerC()
{
	HandleAnswerSelected(EQuizAnswer::C);
}

void UECHO7QuizWidget::HandleAnswerD()
{
	HandleAnswerSelected(EQuizAnswer::D);
}
