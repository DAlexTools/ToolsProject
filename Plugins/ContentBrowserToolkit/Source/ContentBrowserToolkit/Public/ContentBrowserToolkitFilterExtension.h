// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "ContentBrowserFrontEndFilterExtension.h"
#include "ContentBrowserToolkitFilterExtension.generated.h"

UCLASS()
class CONTENTBROWSERTOOLKIT_API UContentBrowserToolkitFilterExtension : public UContentBrowserFrontEndFilterExtension
{
	GENERATED_BODY()

public:
	virtual void AddFrontEndFilterExtensions(TSharedPtr<FFrontendFilterCategory> DefaultCategory, TArray<TSharedRef<FFrontendFilter>>& InOutFilterList) const override;
};
