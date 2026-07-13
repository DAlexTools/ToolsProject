// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

/**
 * Log category for the Data Asset Management System.
 * Used to log asset creation, deletion, and processing events.
 */
DECLARE_LOG_CATEGORY_EXTERN(SDataAssetManagerLog, Log, All);

/**
 * Log category for Data Asset Manager widgets.
 * Used to log user interface (UI) events, such as button clicks and widget updates.
 */
DECLARE_LOG_CATEGORY_EXTERN(SDataAssetManagerWidgetLog, Log, All);

/**
 * Console variable for enabling/disabling Data Asset Manager debug logging.
 *
 * Parameters:
 * - 0: Disabled
 * - 1: Enabled
 */
extern TAutoConsoleVariable<bool> CVarDebugDataAssetManager;
