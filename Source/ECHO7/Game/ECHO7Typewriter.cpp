// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/ECHO7Typewriter.h"

#include "Internationalization/BreakIterator.h"
#include "Misc/Char.h"

void FECHO7TypewriterState::Begin(const FText& InText)
{
	Reset();
	SourceText = InText.ToString();
	if (SourceText.IsEmpty())
	{
		return;
	}

	const TSharedRef<IBreakIterator> CharacterIterator = FBreakIterator::CreateCharacterBoundaryIterator();
	CharacterIterator->SetString(SourceText);
	CharacterIterator->ResetToBeginning();
	for (int32 BoundaryIndex = CharacterIterator->MoveToNext(); BoundaryIndex != INDEX_NONE; BoundaryIndex = CharacterIterator->MoveToNext())
	{
		if (BoundaryIndex > 0 && BoundaryIndex <= SourceText.Len())
		{
			GraphemeBoundaries.Add(BoundaryIndex);
		}
	}

	if (GraphemeBoundaries.IsEmpty() || GraphemeBoundaries.Last() != SourceText.Len())
	{
		GraphemeBoundaries.Add(SourceText.Len());
	}
}

void FECHO7TypewriterState::Reset()
{
	SourceText.Reset();
	GraphemeBoundaries.Reset();
	VisibleGraphemeCount = 0;
	TypedNonWhitespaceGraphemeCount = 0;
}

bool FECHO7TypewriterState::Advance(float ElapsedSeconds, float CharactersPerSecond, int32& OutPreviousVisibleCount, int32& OutNewVisibleCount)
{
	OutPreviousVisibleCount = VisibleGraphemeCount;
	OutNewVisibleCount = VisibleGraphemeCount;
	if (GraphemeBoundaries.IsEmpty())
	{
		return false;
	}

	const int32 TargetVisibleCount = FMath::Clamp(
		FMath::FloorToInt(FMath::Max(0.0f, ElapsedSeconds) * FMath::Max(1.0f, CharactersPerSecond)),
		0,
		GraphemeBoundaries.Num());
	if (TargetVisibleCount <= VisibleGraphemeCount)
	{
		return false;
	}

	VisibleGraphemeCount = TargetVisibleCount;
	OutNewVisibleCount = VisibleGraphemeCount;
	return true;
}

bool FECHO7TypewriterState::ShouldPlayTypingSoundForRange(int32 PreviousVisibleCount, int32 NewVisibleCount, int32 SoundEveryNCharacters)
{
	if (PreviousVisibleCount >= NewVisibleCount)
	{
		return false;
	}

	const int32 SafeSoundInterval = FMath::Max(1, SoundEveryNCharacters);
	bool bShouldPlaySound = false;
	for (int32 GraphemeIndex = PreviousVisibleCount; GraphemeIndex < NewVisibleCount && GraphemeBoundaries.IsValidIndex(GraphemeIndex); ++GraphemeIndex)
	{
		const int32 GraphemeStartIndex = GraphemeIndex > 0 ? GraphemeBoundaries[GraphemeIndex - 1] : 0;
		const int32 GraphemeEndIndex = GraphemeBoundaries[GraphemeIndex];
		bool bWhitespaceOnly = true;
		for (int32 CharacterIndex = GraphemeStartIndex; CharacterIndex < GraphemeEndIndex; ++CharacterIndex)
		{
			if (!FChar::IsWhitespace(SourceText[CharacterIndex]))
			{
				bWhitespaceOnly = false;
				break;
			}
		}
		if (!bWhitespaceOnly)
		{
			++TypedNonWhitespaceGraphemeCount;
			bShouldPlaySound |= TypedNonWhitespaceGraphemeCount % SafeSoundInterval == 0;
		}
	}
	return bShouldPlaySound;
}

FText FECHO7TypewriterState::GetVisibleText() const
{
	if (VisibleGraphemeCount <= 0 || GraphemeBoundaries.IsEmpty())
	{
		return FText::GetEmpty();
	}
	return FText::FromString(SourceText.Left(GraphemeBoundaries[VisibleGraphemeCount - 1]));
}

bool FECHO7TypewriterState::IsComplete() const
{
	return !GraphemeBoundaries.IsEmpty() && VisibleGraphemeCount >= GraphemeBoundaries.Num();
}

bool FECHO7TypewriterState::IsEmpty() const
{
	return GraphemeBoundaries.IsEmpty();
}
