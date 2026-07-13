// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/**
 * @brief Details panel customization for UDataAssetManagerSettings.
 */
class DATAASSETMANAGER_API SDeveloperSettingsWidget : public IDetailCustomization
{
public:
	/**
	 * @brief Creates a customization instance for the Property Editor module.
	 * @return Shared reference to the customization.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/**
	 * @brief Customizes the settings details layout.
	 * @param DetailBuilder Builder used to modify the details panel.
	 */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
