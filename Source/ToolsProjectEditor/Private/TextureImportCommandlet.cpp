#include "TextureImportCommandlet.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AssetToolsModule.h"
#include "Factories/TextureFactory.h"
#include "EditorAssetLibrary.h"


UTextureImportCommandlet::UTextureImportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowProgress = true;
}

int32 UTextureImportCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Warning, TEXT("=== TextureImportCommandlet started ==="));

	FString SourceFolder;
	if (!FParse::Value(*Params, TEXT("SourceFolder="), SourceFolder))
	{
		UE_LOG(LogTemp, Error, TEXT("SourceFolder not specified!"));
		return -1;
	}
	TArray<FString> tokens;
	TArray<FString> switches;
	FString DestinationFolder = TEXT("/Game/ImportedTextures");
	FParse::Value(*Params, TEXT("DestinationFolder="), DestinationFolder);

	ParseCommandLine(*Params, tokens, switches);

	ImportTexturesFromFolder(SourceFolder, DestinationFolder);

	UE_LOG(LogTemp, Warning, TEXT("=== TextureImportCommandlet finished ==="));
	return 0;
}

void UTextureImportCommandlet::ExecuteConsoleCommand(const TArray<FString>& Args)
{
	if (Args.Num() == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("Usage: Import.Textures SourceFolder [DestinationFolder]"));
		return;
	}

	FString SourceFolder = Args[0];
	FString DestinationFolder = (Args.Num() > 1) ? Args[1] : TEXT("/Game/ImportedTextures");

	// Создаем экземпляр и запускаем импорт
	UTextureImportCommandlet* Commandlet = NewObject<UTextureImportCommandlet>();
	Commandlet->ImportTexturesFromFolder(SourceFolder, DestinationFolder);
}

void UTextureImportCommandlet::ImportTexturesFromFolder(const FString& SourceFolder, const FString& DestinationFolder)
{
	IFileManager&	FileManager = IFileManager::Get();
	TArray<FString> Files;
	FileManager.FindFilesRecursive(Files, *SourceFolder, TEXT("*.*"), true, false);

	UE_LOG(LogTemp, Log, TEXT("Found files: %d"), Files.Num());

	if (Files.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No files found in folder: %s"), *SourceFolder);
		return;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	int32			   ImportedCount = 0;

	for (const FString& FilePath : Files)
	{
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		if (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg") && Extension != TEXT("bmp") && Extension != TEXT("tga"))
			continue;

		UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
		TextureFactory->AddToRoot();
		TextureFactory->SuppressImportOverwriteDialog();

		TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(
			{ FilePath },
			DestinationFolder,
			TextureFactory);

		if (ImportedAssets.Num() > 0)
			ImportedCount++;

		TextureFactory->RemoveFromRoot();
	}

	UE_LOG(LogTemp, Warning, TEXT("Import completed. Imported: %d"), ImportedCount);
}