// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(SDataAssetManagerLog, All, All)

FString DataAssetManager::GetAssetDiskSize(const FAssetData& AssetData)
{
	TTuple<double, double, FString, FString> SizeAndText(0.0, 0.0, TEXT("Unknown"), TEXT("Unknown"));
	auto& SizeInKb = SizeAndText.Get<0>();
	auto& SizeInMb = SizeAndText.Get<1>();
	auto& TextKb = SizeAndText.Get<2>();
	auto& TextMb = SizeAndText.Get<3>();
	constexpr double ConversionFactor = 1024.0;

	FString PackageFileName;
	if (FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), &PackageFileName))
	{
		const auto FileSize = IFileManager::Get().FileSize(*PackageFileName);
		if (FileSize == INDEX_NONE)
		{
			return TEXT("Unknown");
		}

		SizeInKb = static_cast<double>(FileSize) / ConversionFactor;
		TextKb = FString::Printf(TEXT("%.1f Kb"), SizeInKb);

		if (SizeInKb >= ConversionFactor)
		{
			SizeInMb = SizeInKb / ConversionFactor;
			TextMb = FString::Printf(TEXT("%.1f Mb"), SizeInMb);
			return TextMb;
		}

		return TextKb;
	}

	return TEXT("Unknown");
}

const UDataAssetManagerSettings* DataAssetManager::GetPluginSettings()
{
	const UDataAssetManagerSettings* Settings = GetDefault<UDataAssetManagerSettings>();
	return Settings;
}

bool DataAssetManager::DeleteMultiplyAsset(const TArray<FAssetData>& Assets)
{
	if (Assets.Num() == 0)
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("%s No assets to delete!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	int32 DeletedCount = ObjectTools::DeleteAssets(Assets);
	UE_LOG(SDataAssetManagerLog, Log, TEXT("%s Deleted %d assets"), ANSI_TO_TCHAR(__FUNCTION__), DeletedCount);

	return DeletedCount > 0;
}

void DataAssetManager::CreateNewDataAsset(UClass* AssetClass, const FString& Directory)
{
	if (!AssetClass || !AssetClass->IsChildOf(UDataAsset::StaticClass()))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("%s Invalid class provided for DataAsset creation."), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	FString AssetPath = Directory;
	FPaths::NormalizeDirectoryName(AssetPath);

	if (AssetPath.IsEmpty())
	{
		AssetPath = TEXT("/Game");
	}
	else if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = TEXT("/Game/") + AssetPath;
	}


	if (AssetPath.EndsWith("/"))
	{
		AssetPath = AssetPath.LeftChop(1);
	}

	UE_LOG(SDataAssetManagerLog, Warning, TEXT("Final AssetPath: %s"), *AssetPath);

	const FString BaseAssetName = TEXT("NewDataAsset");
	FString FinalAssetName = BaseAssetName;

	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools);

	int32 Suffix = 1;
	FString TestPackageName = AssetPath / FinalAssetName;
	FString TestObjectPath = TestPackageName + TEXT(".") + FinalAssetName;

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);

	while (AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(TestObjectPath)).IsValid())
	{
		FinalAssetName = BaseAssetName + FString::Printf(TEXT("_%d"), Suffix);
		TestPackageName = AssetPath / FinalAssetName;
		TestObjectPath = TestPackageName + TEXT(".") + FinalAssetName;
		Suffix++;
	}

	UE_LOG(SDataAssetManagerLog, Warning, TEXT("Creating asset: Name=%s, Path=%s, Class=%s"),
		*FinalAssetName, *AssetPath, *AssetClass->GetName());

	UObject* const NewAsset = AssetToolsModule.Get().CreateAsset(FinalAssetName, AssetPath, AssetClass, nullptr);

	if (NewAsset)
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("Successfully created DataAsset: %s"), *NewAsset->GetPathName());

		NewAsset->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(NewAsset);

		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(DataAssetManager::ModuleName::ContentBrowser);
		TArray<UObject*> AssetsToSync;
		AssetsToSync.Add(NewAsset);
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
	}
	else
	{
		UE_LOG(SDataAssetManagerLog, Error, TEXT("FAILED to create DataAsset: Name=%s, Path=%s"),
			*FinalAssetName, *AssetPath);
	}
}

void DataAssetManager::ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction)
{
	TArray<FAssetIdentifier> AssetIdentifiers;
	/** Converts asset data to identifiers for reference viewer / size map / audit tools */
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(RefAssetData, AssetIdentifiers);
	ProcessFunction(AssetIdentifiers);
}

FString DataAssetManager::PathNormalize(const FString& InPath)
{
	if (InPath.IsEmpty()) return {};

	// Ensure the path dont starts with a slash or a disk drive letter
	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return {};
	}

	FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
	FPaths::RemoveDuplicateSlashes(Path);

	// Collapse any ".." or "." references in the path
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	// Ensure the path does not end with a trailing slash
	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\")))
	{
		Path = Path.LeftChop(1);
	}

	return Path;
}

TTuple<FString, FString> DataAssetManager::GetNormalizedAndProjectPath(const FString& InPath)
{
	return TTuple<FString, FString>(
		PathNormalize(InPath),
		FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1)
	);
}

FString DataAssetManager::PathConvert(const FString& InPath, bool bToAbsolute)
{
	const auto PathsData = GetNormalizedAndProjectPath(InPath);
	const FString& PathNormalized = PathsData.Get<0>();
	const FString& PathProjectContent = PathsData.Get<1>();

	if (PathNormalized.IsEmpty())
	{
		return {};
	}

	const FString LocalPathRoot = GetPathRootToString();

	const bool bIsRoot = PathNormalized.StartsWith(LocalPathRoot);
	const bool bIsProject = PathNormalized.StartsWith(PathProjectContent);

	if (bToAbsolute)
	{
		if (bIsProject)
		{
			return PathNormalized;
		}
		if (bIsRoot)
		{
			FString Path = PathNormalized;
			Path.RemoveFromStart(LocalPathRoot);
			return Path.IsEmpty() ? PathProjectContent : PathProjectContent / Path;
		}
	}
	else
	{
		if (bIsRoot)
		{
			return PathNormalized;
		}
		if (bIsProject)
		{
			FString Path = PathNormalized;
			Path.RemoveFromStart(PathProjectContent);
			return Path.IsEmpty() ? LocalPathRoot : LocalPathRoot / Path;
		}
	}

	return {};
}

FString DataAssetManager::PathConvertToAbsolute(const FString& InPath)
{
	return PathConvert(InPath, true);
}

FString DataAssetManager::PathConvertToRelative(const FString& InPath)
{
	return PathConvert(InPath, false);
}

bool DataAssetManager::FolderIsEmpty(const FString& InPath)
{
	if (InPath.IsEmpty())
	{
		return false;
	}

	const FName PathRel = FName(*PathConvertToRelative(InPath));
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (const bool HasAsset = AssetRegistry.Get().HasAssets(PathRel, true))
	{
		return false;
	}

	const FString PathAbs = PathConvertToAbsolute(InPath);
	if (PathAbs.IsEmpty()) 
	{
		return false;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *PathAbs, TEXT("*"), true, false);

	return Files.Num() == 0;
}

FString DataAssetManager::GetPathExternalActors()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalActorsFolderName());
}

FString DataAssetManager::GetPathExternalObjects()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalObjectsFolderName());
}

bool DataAssetManager::FolderIsExternal(const FString& InPath)
{
	return InPath.StartsWith(GetPathExternalActors()) || InPath.StartsWith(GetPathExternalObjects());
}

bool DataAssetManager::SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath)
{
	if (!IsValid(DataAsset))
	{
		return false;
	}

	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(DataAsset->GetClass(), DataAsset, JsonString, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to serialize DataAsset to JSON"));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to save JSON file: %s"), *FilePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("DataAsset saved to JSON: %s"), *FilePath);
	return true;
}

bool DataAssetManager::LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath)
{
	if (!IsValid(DataAsset))
	{
		return false;
	}

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load JSON file: %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON from file: %s"), *FilePath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), DataAsset->GetClass(), DataAsset, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize JSON to DataAsset"));
		return false;
	}

	DataAsset->PostEditChange();
	DataAsset->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("DataAsset loaded from JSON: %s"), *FilePath);
	return true;
}

