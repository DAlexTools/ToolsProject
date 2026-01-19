#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"


#include "AssetToolsModule.h"
#include "Factories/TextureFactory.h"
#include "EditorAssetLibrary.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "TextureImportCommandlet.generated.h"

UCLASS()
class TOOLSPROJECTEDITOR_API UTextureImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UTextureImportCommandlet();

	virtual int32 Main(const FString& Params) override;


	// Статический метод для консольной команды
	static void ExecuteConsoleCommand(const TArray<FString>& Args);

private:
	void ImportTexturesFromFolder(const FString& SourceFolder, const FString& DestinationFolder);
};