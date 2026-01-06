// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/UtilsFunctionLibrary.h"

FString FUtilsFunctionLibrary::AddSpacesBeforeUppercase(const FString& Input)
{
	FString Result;
	Result.Reserve(Input.Len() * 2);

	for (int32 i = 0; i < Input.Len(); ++i)
	{
		const TCHAR Char = Input[i];
		if (i > 0 && FChar::IsUpper(Char) && !FChar::IsWhitespace(Input[i - 1]))
		{
			Result += TEXT(" ");
		}
		Result += Char;
	}

	return Result;
}