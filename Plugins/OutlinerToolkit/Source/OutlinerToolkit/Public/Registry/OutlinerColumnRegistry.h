// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ISceneOutlinerColumn;
class ISceneOutliner;
class FSceneOutlinerModule;

/**
 * @brief Utility class responsible for Scene Outliner column registration.
 *
 * Provides centralized registration logic for all custom Scene Outliner
 * columns used by the plugin. This class encapsulates the setup of
 * column visibility, ordering priority, and column factory creation.
 *
 * The registry is initialized during module startup to make all
 * custom columns globally available in the Unreal Editor Scene Outliner.
 */
class OUTLINERTOOLKIT_API FOutlinerColumnRegistry final
{
public:
	/**
	 * @brief Registers all custom Scene Outliner columns used by the plugin.
	 *
	 * Loads the SceneOutliner module and registers every custom column
	 * with its corresponding visibility state and display priority.
	 *
	 * The registration process makes the columns available globally
	 * for all Scene Outliner instances within the editor.
	 */
	static void RegisterColumns();

private:
	/**
	 * @brief Registers a custom Scene Outliner column type.
	 *
	 * @tparam TColumn The custom Scene Outliner column class type.
	 *
	 * @param Module
	 * Reference to the SceneOutliner module responsible for column registration.
	 *
	 * @param Priority
	 * Display priority used to determine the column ordering inside the Scene Outliner.
	 */
	template<typename TColumn>
	static void RegisterColumn(FSceneOutlinerModule& Module, int32 Priority);
};


