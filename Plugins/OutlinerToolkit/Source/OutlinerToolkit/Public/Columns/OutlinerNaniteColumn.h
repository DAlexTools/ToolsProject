// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

class OUTLINERTOOLKIT_API FOutlinerNaniteColumn : public FOutlinerToolkitColumnBase
{
public:
	FOutlinerNaniteColumn(ISceneOutliner& SceneOutliner)
		: FOutlinerToolkitColumnBase(SceneOutliner)
	{
	}

	virtual FName GetColumnID() override { return FName("Nanite"); }
	static FName GetID() { return FName("Nanite"); }

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;
	virtual bool SupportsSorting() const override { return false; }
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	FReply GetNaniteColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const;
	FSlateColor GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const;
};
