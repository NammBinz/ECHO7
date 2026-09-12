// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Unicode-safe state for revealing one text row at a time without splitting grapheme clusters. */
class ECHO7_API FECHO7TypewriterState
{
public:
	void Begin(const FText& InText);
	void Reset();

	/** Advances visible graphemes from elapsed time. Returns true when new graphemes became visible. */
	bool Advance(float ElapsedSeconds, float CharactersPerSecond, int32& OutPreviousVisibleCount, int32& OutNewVisibleCount);

	/** Returns whether the specified newly revealed range should emit one throttled typing sound. */
	bool ShouldPlayTypingSoundForRange(int32 PreviousVisibleCount, int32 NewVisibleCount, int32 SoundEveryNCharacters);

	FText GetVisibleText() const;
	bool IsComplete() const;
	bool IsEmpty() const;

private:
	FString SourceText;
	TArray<int32> GraphemeBoundaries;
	int32 VisibleGraphemeCount = 0;
	int32 TypedNonWhitespaceGraphemeCount = 0;
};
