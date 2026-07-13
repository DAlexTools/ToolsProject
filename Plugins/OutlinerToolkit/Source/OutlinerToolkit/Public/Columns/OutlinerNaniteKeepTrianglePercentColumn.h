// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

class OUTLINERTOOLKIT_API FOutlinerNaniteKeepTrianglePercentColumn : public FOutlinerToolkitColumnBase
{
public:
	FOutlinerNaniteKeepTrianglePercentColumn(ISceneOutliner& SceneOutliner)
		: FOutlinerToolkitColumnBase(SceneOutliner)
	{
	}

	virtual FName GetColumnID() override { return FName("NaniteKeepTrianglePercent"); }
	static FName GetID() { return FName("NaniteKeepTrianglePercent"); }

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;
	virtual bool SupportsSorting() const override { return false; }
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	TOptional<float> GetKeepTrianglePercentValue(TWeakObjectPtr<AActor> WeakActor) const;
	void OnKeepTrianglePercentCommitted(float NewValue, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor) const;
};
