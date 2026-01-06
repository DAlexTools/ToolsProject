// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Utility function library for string and general helper functions.
 *
 * This class contains static helper functions that can be used throughout
 * the validator system. It is marked as `final` to prevent inheritance.
 */
class VALIDATORX_API FUtilsFunctionLibrary final
{
public:
	/**
	 * @brief Inserts spaces before uppercase letters in a string.
	 *
	 * Converts strings like "ValidatorName" to "Validator Name" for improved readability.
	 *
	 * @param Input The input string to process.
	 * @return A new string with spaces inserted before uppercase letters.
	 */
	[[nodiscard]] static FString AddSpacesBeforeUppercase(const FString& Input);
};