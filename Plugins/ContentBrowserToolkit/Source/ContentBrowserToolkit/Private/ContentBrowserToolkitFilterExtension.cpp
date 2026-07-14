// Copyright 2025 DimAlek. All Rights Reserved.

#include "ContentBrowserToolkitFilterExtension.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/BlueprintSupport.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserItem.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Settings/ContentBrowserToolkitSettings.h"
#include "UObject/Object.h"

#include <initializer_list>

#define LOCTEXT_NAMESPACE "ContentBrowserToolkit"

namespace ContentBrowserToolkit
{
	using FAssetDataPredicate = TFunction<bool(const FAssetData&)>;

	struct FFilterSpec
	{
		FString Name;
		FText DisplayName;
		FText ToolTipText;
		FLinearColor Color;
		TFunction<bool(FAssetFilterType)> Predicate;
	};

	struct FCachedPackageAudit
	{
		bool bHasReferencers = false;
		bool bReferencedByMap = false;
		bool bHasDependencies = false;
		bool bReferencesEngineContent = false;
		bool bReferencesPluginContent = false;
		bool bReferencesProjectContent = false;
	};

	bool TryGetAssetData(FAssetFilterType InItem, FAssetData& OutAssetData)
	{
		return InItem.IsFile()
			&& InItem.Legacy_TryGetAssetData(OutAssetData)
			&& OutAssetData.IsValid();
	}

	bool IsTruthyTagValue(const FString& Value)
	{
		return Value.Equals(TEXT("True"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("Yes"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("1"), ESearchCase::IgnoreCase);
	}

	bool IsFalsyTagValue(const FString& Value)
	{
		return Value.Equals(TEXT("False"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("No"), ESearchCase::IgnoreCase)
			|| Value.Equals(TEXT("0"), ESearchCase::IgnoreCase);
	}

	bool HasMeaningfulTagValue(const FString& Value)
	{
		const FString TrimmedValue = Value.TrimStartAndEnd();
		return !TrimmedValue.IsEmpty()
			&& !TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase)
			&& !TrimmedValue.Equals(TEXT("()"), ESearchCase::CaseSensitive)
			&& !TrimmedValue.Equals(TEXT("{}"), ESearchCase::CaseSensitive);
	}

	bool GetStringTag(const FAssetData& AssetData, FName TagName, FString& OutValue)
	{
		return AssetData.GetTagValue(TagName, OutValue);
	}

	bool HasNonEmptyTag(const FAssetData& AssetData, FName TagName)
	{
		FString Value;
		return GetStringTag(AssetData, TagName, Value) && HasMeaningfulTagValue(Value);
	}

	bool GetBoolTag(const FAssetData& AssetData, FName TagName, bool& bOutValue)
	{
		FString Value;
		if (!GetStringTag(AssetData, TagName, Value))
		{
			return false;
		}

		if (IsTruthyTagValue(Value))
		{
			bOutValue = true;
			return true;
		}

		if (IsFalsyTagValue(Value))
		{
			bOutValue = false;
			return true;
		}

		return false;
	}

	bool GetIntTag(const FAssetData& AssetData, FName TagName, int64& OutValue)
	{
		FString Value;
		if (!GetStringTag(AssetData, TagName, Value))
		{
			return false;
		}

		Value.TrimStartAndEndInline();
		return LexTryParseString(OutValue, *Value);
	}

	bool GetFloatTag(const FAssetData& AssetData, FName TagName, double& OutValue)
	{
		FString Value;
		if (!GetStringTag(AssetData, TagName, Value))
		{
			return false;
		}

		Value.TrimStartAndEndInline();
		return LexTryParseString(OutValue, *Value);
	}

	FName GetClassName(const FAssetData& AssetData)
	{
		return AssetData.AssetClassPath.GetAssetName();
	}

	bool IsClass(const FAssetData& AssetData, const TCHAR* ClassName)
	{
		return GetClassName(AssetData) == FName(ClassName);
	}

	bool IsAnyClass(const FAssetData& AssetData, std::initializer_list<const TCHAR*> ClassNames)
	{
		const FName AssetClassName = GetClassName(AssetData);
		for (const TCHAR* ClassName : ClassNames)
		{
			if (AssetClassName == FName(ClassName))
			{
				return true;
			}
		}

		return false;
	}

	bool ClassContains(const FAssetData& AssetData, const TCHAR* Text)
	{
		return GetClassName(AssetData).ToString().Contains(Text, ESearchCase::IgnoreCase);
	}

	FString GetPackagePathString(const FAssetData& AssetData)
	{
		return AssetData.PackagePath.ToString();
	}

	bool PackagePathStartsWith(const FAssetData& AssetData, const TCHAR* Prefix)
	{
		return GetPackagePathString(AssetData).StartsWith(Prefix, ESearchCase::IgnoreCase);
	}

	bool PackageNameStartsWith(FName PackageName, const TCHAR* Prefix)
	{
		return PackageName.ToString().StartsWith(Prefix, ESearchCase::IgnoreCase);
	}

	bool IsProjectPackagePath(const FAssetData& AssetData)
	{
		return PackagePathStartsWith(AssetData, TEXT("/Game"));
	}

	bool IsEnginePackagePath(const FAssetData& AssetData)
	{
		return PackagePathStartsWith(AssetData, TEXT("/Engine"));
	}

	bool IsPluginPackagePath(const FAssetData& AssetData)
	{
		const FString PackagePath = GetPackagePathString(AssetData);
		return PackagePath.StartsWith(TEXT("/"))
			&& !PackagePath.StartsWith(TEXT("/Game"), ESearchCase::IgnoreCase)
			&& !PackagePath.StartsWith(TEXT("/Engine"), ESearchCase::IgnoreCase)
			&& !PackagePath.StartsWith(TEXT("/Script"), ESearchCase::IgnoreCase);
	}

	bool GetDiskSizeBytes(FAssetFilterType InItem, int64& OutDiskSizeBytes)
	{
		const FContentBrowserItemDataAttributeValue DiskSizeValue = InItem.GetItemAttribute(ContentBrowserItemAttributes::ItemDiskSize);
		if (!DiskSizeValue.IsValid())
		{
			return false;
		}

		OutDiskSizeBytes = DiskSizeValue.GetValue<int64>();
		return true;
	}

	bool GetBoolItemAttribute(FAssetFilterType InItem, FName AttributeName)
	{
		const FContentBrowserItemDataAttributeValue AttributeValue = InItem.GetItemAttribute(AttributeName);
		return AttributeValue.IsValid() && AttributeValue.GetValue<bool>();
	}

	bool GetVirtualizedDataAttribute(FAssetFilterType InItem)
	{
		const FContentBrowserItemDataAttributeValue AttributeValue = InItem.GetItemAttribute(ContentBrowserItemAttributes::VirtualizedData);
		return AttributeValue.IsValid() && AttributeValue.GetValue<FString>().Equals(TEXT("True"), ESearchCase::IgnoreCase);
	}

	bool IsBlueprintAsset(const FAssetData& AssetData)
	{
		return HasNonEmptyTag(AssetData, FBlueprintTags::GeneratedClassPath)
			|| HasNonEmptyTag(AssetData, FBlueprintTags::ParentClassPath)
			|| IsAnyClass(AssetData, { TEXT("Blueprint"), TEXT("WidgetBlueprint"), TEXT("AnimBlueprint"), TEXT("BlueprintInterface"), TEXT("BlueprintMacroLibrary") });
	}

	bool IsTextureAsset(const FAssetData& AssetData)
	{
		return IsAnyClass(AssetData, {
			TEXT("Texture2D"),
			TEXT("Texture2DArray"),
			TEXT("TextureCube"),
			TEXT("TextureRenderTarget2D"),
			TEXT("TextureRenderTargetCube"),
			TEXT("VolumeTexture"),
			TEXT("RuntimeVirtualTexture"),
			TEXT("SparseVolumeTexture")
		});
	}

	bool IsMaterialAsset(const FAssetData& AssetData)
	{
		return IsAnyClass(AssetData, {
			TEXT("Material"),
			TEXT("MaterialInstance"),
			TEXT("MaterialInstanceConstant"),
			TEXT("MaterialFunction"),
			TEXT("MaterialFunctionInstance"),
			TEXT("MaterialParameterCollection")
		});
	}

	bool IsAudioAsset(const FAssetData& AssetData)
	{
		return IsAnyClass(AssetData, {
			TEXT("SoundWave"),
			TEXT("SoundCue"),
			TEXT("MetaSoundSource"),
			TEXT("MetaSoundPatch"),
			TEXT("SoundClass"),
			TEXT("SoundMix"),
			TEXT("DialogueWave"),
			TEXT("DialogueVoice")
		});
	}

	bool IsAnimationAsset(const FAssetData& AssetData)
	{
		return IsAnyClass(AssetData, {
			TEXT("AnimSequence"),
			TEXT("AnimMontage"),
			TEXT("BlendSpace"),
			TEXT("BlendSpace1D"),
			TEXT("AimOffsetBlendSpace"),
			TEXT("PoseAsset"),
			TEXT("AnimBlueprint")
		});
	}

	bool IsDataLikeAsset(const FAssetData& AssetData)
	{
		return IsAnyClass(AssetData, {
			TEXT("DataAsset"),
			TEXT("PrimaryDataAsset"),
			TEXT("DataTable"),
			TEXT("CurveTable"),
			TEXT("CurveFloat"),
			TEXT("CurveVector"),
			TEXT("CurveLinearColor"),
			TEXT("UserDefinedStruct"),
			TEXT("UserDefinedEnum")
		}) || ClassContains(AssetData, TEXT("DataAsset"));
	}

	bool IsMeaningfulBlueprintListTag(const FAssetData& AssetData, FName TagName)
	{
		FString Value;
		return GetStringTag(AssetData, TagName, Value) && HasMeaningfulTagValue(Value);
	}

	bool ExtractSourceFilenames(const FAssetData& AssetData, TArray<FString>& OutFilenames)
	{
		FString SourceFileData;
		if (!GetStringTag(AssetData, UObject::SourceFileTagName(), SourceFileData) || SourceFileData.IsEmpty())
		{
			return false;
		}

		TOptional<FAssetImportInfo> ImportInfo = FAssetImportInfo::FromJson(SourceFileData);
		if (!ImportInfo.IsSet())
		{
			return false;
		}

		const FString PackageName = AssetData.PackageName.ToString();
		for (const FAssetImportInfo::FSourceFile& SourceFile : ImportInfo->SourceFiles)
		{
			if (!SourceFile.RelativeFilename.IsEmpty())
			{
				OutFilenames.Add(UAssetImportData::ResolveImportFilename(SourceFile.RelativeFilename, PackageName));
			}
		}

		return !OutFilenames.IsEmpty();
	}

	bool HasMissingSourceFile(const FAssetData& AssetData)
	{
		TArray<FString> SourceFilenames;
		if (!ExtractSourceFilenames(AssetData, SourceFilenames))
		{
			return false;
		}

		for (const FString& SourceFilename : SourceFilenames)
		{
			if (!FPaths::FileExists(SourceFilename))
			{
				return true;
			}
		}

		return false;
	}

	bool HasSourceOutsideProject(const FAssetData& AssetData)
	{
		TArray<FString> SourceFilenames;
		if (!ExtractSourceFilenames(AssetData, SourceFilenames))
		{
			return false;
		}

		const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		for (const FString& SourceFilename : SourceFilenames)
		{
			const FString AbsoluteSourceFilename = FPaths::ConvertRelativePathToFull(SourceFilename);
			if (!FPaths::IsUnderDirectory(AbsoluteSourceFilename, ProjectDir))
			{
				return true;
			}
		}

		return false;
	}

	FString GetExpectedPrefix(const FAssetData& AssetData)
	{
		if (IsClass(AssetData, TEXT("WidgetBlueprint")))
		{
			return TEXT("WBP_");
		}
		if (IsClass(AssetData, TEXT("AnimBlueprint")))
		{
			return TEXT("ABP_");
		}
		if (IsBlueprintAsset(AssetData))
		{
			return TEXT("BP_");
		}
		if (IsTextureAsset(AssetData))
		{
			return TEXT("T_");
		}
		if (IsClass(AssetData, TEXT("Material")))
		{
			return TEXT("M_");
		}
		if (IsAnyClass(AssetData, { TEXT("MaterialInstance"), TEXT("MaterialInstanceConstant") }))
		{
			return TEXT("MI_");
		}
		if (IsClass(AssetData, TEXT("MaterialFunction")))
		{
			return TEXT("MF_");
		}
		if (IsClass(AssetData, TEXT("MaterialParameterCollection")))
		{
			return TEXT("MPC_");
		}
		if (IsClass(AssetData, TEXT("StaticMesh")))
		{
			return TEXT("SM_");
		}
		if (IsClass(AssetData, TEXT("SkeletalMesh")))
		{
			return TEXT("SK_");
		}
		if (IsClass(AssetData, TEXT("Skeleton")))
		{
			return TEXT("SKEL_");
		}
		if (IsClass(AssetData, TEXT("PhysicsAsset")))
		{
			return TEXT("PHYS_");
		}
		if (IsClass(AssetData, TEXT("AnimSequence")))
		{
			return TEXT("A_");
		}
		if (IsClass(AssetData, TEXT("AnimMontage")))
		{
			return TEXT("AM_");
		}
		if (IsAnyClass(AssetData, { TEXT("BlendSpace"), TEXT("BlendSpace1D"), TEXT("AimOffsetBlendSpace") }))
		{
			return TEXT("BS_");
		}
		if (IsClass(AssetData, TEXT("NiagaraSystem")))
		{
			return TEXT("NS_");
		}
		if (IsClass(AssetData, TEXT("NiagaraEmitter")))
		{
			return TEXT("NE_");
		}
		if (IsClass(AssetData, TEXT("SoundWave")))
		{
			return TEXT("S_");
		}
		if (IsClass(AssetData, TEXT("SoundCue")))
		{
			return TEXT("SC_");
		}
		if (IsAnyClass(AssetData, { TEXT("MetaSoundSource"), TEXT("MetaSoundPatch") }))
		{
			return TEXT("MS_");
		}
		if (IsClass(AssetData, TEXT("DataTable")))
		{
			return TEXT("DT_");
		}
		if (IsClass(AssetData, TEXT("CurveTable")))
		{
			return TEXT("CT_");
		}
		if (IsAnyClass(AssetData, { TEXT("DataAsset"), TEXT("PrimaryDataAsset") }) || ClassContains(AssetData, TEXT("DataAsset")))
		{
			return TEXT("DA_");
		}
		if (IsClass(AssetData, TEXT("World")))
		{
			return TEXT("L_");
		}

		return FString();
	}

	bool ViolatesKnownPrefixConvention(const FAssetData& AssetData)
	{
		const FString ExpectedPrefix = GetExpectedPrefix(AssetData);
		return !ExpectedPrefix.IsEmpty() && !AssetData.AssetName.ToString().StartsWith(ExpectedPrefix, ESearchCase::CaseSensitive);
	}

	bool HasWhitespaceInName(const FAssetData& AssetData)
	{
		return AssetData.AssetName.ToString().Contains(TEXT(" "));
	}

	bool HasLowercasePrefix(const FAssetData& AssetData)
	{
		const FString AssetName = AssetData.AssetName.ToString();
		if (AssetName.IsEmpty())
		{
			return false;
		}

		const TCHAR FirstChar = AssetName[0];
		return FirstChar >= TCHAR('a') && FirstChar <= TCHAR('z');
	}

	bool IsWorldPackage(IAssetRegistry& AssetRegistry, FName PackageName)
	{
		TArray<FAssetData> PackageAssets;
		if (!AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets))
		{
			return false;
		}

		for (const FAssetData& PackageAsset : PackageAssets)
		{
			if (IsClass(PackageAsset, TEXT("World")))
			{
				return true;
			}
		}

		return false;
	}

	class FAssetAuditCache
	{
	public:
		static FAssetAuditCache& Get()
		{
			static FAssetAuditCache Instance;
			return Instance;
		}

		const FCachedPackageAudit& GetAudit(const FAssetData& AssetData) const
		{
			if (const FCachedPackageAudit* ExistingAudit = Cache.Find(AssetData.PackageName))
			{
				return *ExistingAudit;
			}

			return Cache.Add(AssetData.PackageName, BuildAudit(AssetData.PackageName));
		}

		void Reset() const
		{
			Cache.Reset();
		}

	private:
		FCachedPackageAudit BuildAudit(FName PackageName) const
		{
			FCachedPackageAudit Audit;

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			TArray<FName> Referencers;
			AssetRegistry.GetReferencers(PackageName, Referencers);
			for (const FName Referencer : Referencers)
			{
				if (Referencer == PackageName)
				{
					continue;
				}

				Audit.bHasReferencers = true;
				Audit.bReferencedByMap = Audit.bReferencedByMap || IsWorldPackage(AssetRegistry, Referencer);
			}

			TArray<FName> Dependencies;
			AssetRegistry.GetDependencies(PackageName, Dependencies);
			for (const FName Dependency : Dependencies)
			{
				if (Dependency == PackageName)
				{
					continue;
				}

				Audit.bHasDependencies = true;
				Audit.bReferencesEngineContent = Audit.bReferencesEngineContent || PackageNameStartsWith(Dependency, TEXT("/Engine"));
				Audit.bReferencesProjectContent = Audit.bReferencesProjectContent || PackageNameStartsWith(Dependency, TEXT("/Game"));
				Audit.bReferencesPluginContent = Audit.bReferencesPluginContent
					|| (PackageNameStartsWith(Dependency, TEXT("/"))
						&& !PackageNameStartsWith(Dependency, TEXT("/Game"))
						&& !PackageNameStartsWith(Dependency, TEXT("/Engine"))
						&& !PackageNameStartsWith(Dependency, TEXT("/Script")));
			}

			return Audit;
		}

		mutable TMap<FName, FCachedPackageAudit> Cache;
	};

	void AddFilter(
		TArray<FFilterSpec>& Specs,
		const TCHAR* Name,
		FText DisplayName,
		FText ToolTipText,
		FLinearColor Color,
		TFunction<bool(FAssetFilterType)> Predicate)
	{
		Specs.Add({
			Name,
			MoveTemp(DisplayName),
			MoveTemp(ToolTipText),
			Color,
			MoveTemp(Predicate)
		});
	}

	void AddAssetDataFilter(
		TArray<FFilterSpec>& Specs,
		const TCHAR* Name,
		FText DisplayName,
		FText ToolTipText,
		FLinearColor Color,
		FAssetDataPredicate Predicate)
	{
		AddFilter(
			Specs,
			Name,
			MoveTemp(DisplayName),
			MoveTemp(ToolTipText),
			Color,
			[Predicate = MoveTemp(Predicate)](FAssetFilterType InItem)
			{
				FAssetData AssetData;
				return TryGetAssetData(InItem, AssetData) && Predicate(AssetData);
			});
	}

	void AddGeneralFilters(TArray<FFilterSpec>& Specs)
	{
		const UContentBrowserToolkitSettings& Settings = *UContentBrowserToolkitSettings::Get();
		const FLinearColor GeneralColor(0.15f, 0.55f, 0.85f);
		const FLinearColor WarningColor(0.9f, 0.25f, 0.2f);
		const FLinearColor NamingColor(0.75f, 0.55f, 0.15f);

		AddAssetDataFilter(Specs, TEXT("CBTK_ProjectContent"), LOCTEXT("ProjectContentName", "Project Content"), LOCTEXT("ProjectContentTooltip", "Show assets under /Game."), GeneralColor, [](const FAssetData& AssetData) { return IsProjectPackagePath(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_EngineContent"), LOCTEXT("EngineContentName", "Engine Content"), LOCTEXT("EngineContentTooltip", "Show assets under /Engine."), GeneralColor, [](const FAssetData& AssetData) { return IsEnginePackagePath(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_PluginContent"), LOCTEXT("PluginContentName", "Plugin Content"), LOCTEXT("PluginContentTooltip", "Show assets under plugin mount points."), GeneralColor, [](const FAssetData& AssetData) { return IsPluginPackagePath(AssetData); });
		AddFilter(Specs, TEXT("CBTK_DeveloperContent"), LOCTEXT("DeveloperContentName", "Developer Content"), LOCTEXT("DeveloperContentTooltip", "Show assets marked as developer content."), GeneralColor, [](FAssetFilterType InItem) { return GetBoolItemAttribute(InItem, ContentBrowserItemAttributes::ItemIsDeveloperContent); });
		AddFilter(Specs, TEXT("CBTK_LocalizedContent"), LOCTEXT("LocalizedContentName", "Localized Content"), LOCTEXT("LocalizedContentTooltip", "Show assets marked as localized content."), GeneralColor, [](FAssetFilterType InItem) { return GetBoolItemAttribute(InItem, ContentBrowserItemAttributes::ItemIsLocalizedContent); });
		AddFilter(Specs, TEXT("CBTK_VirtualizedData"), LOCTEXT("VirtualizedDataName", "Virtualized Data"), LOCTEXT("VirtualizedDataTooltip", "Show assets with virtualized bulk data."), GeneralColor, [](FAssetFilterType InItem) { return GetVirtualizedDataAttribute(InItem); });

		AddAssetDataFilter(Specs, TEXT("CBTK_ImportedAssets"), LOCTEXT("ImportedAssetsName", "Imported Assets"), LOCTEXT("ImportedAssetsTooltip", "Show assets that keep source file import data and can usually be reimported."), GeneralColor, [](const FAssetData& AssetData) { return HasNonEmptyTag(AssetData, UObject::SourceFileTagName()); });
		AddAssetDataFilter(Specs, TEXT("CBTK_MissingSourceFile"), LOCTEXT("MissingSourceFileName", "Missing Source File"), LOCTEXT("MissingSourceFileTooltip", "Show imported assets whose source file path no longer exists on disk."), WarningColor, [](const FAssetData& AssetData) { return HasMissingSourceFile(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_SourceOutsideProject"), LOCTEXT("SourceOutsideProjectName", "Source Outside Project"), LOCTEXT("SourceOutsideProjectTooltip", "Show imported assets whose source files live outside the project directory."), NamingColor, [](const FAssetData& AssetData) { return HasSourceOutsideProject(AssetData); });

		AddAssetDataFilter(Specs, TEXT("CBTK_PrimaryAssets"), LOCTEXT("PrimaryAssetsName", "Primary Assets"), LOCTEXT("PrimaryAssetsTooltip", "Show assets with a valid PrimaryAssetId assigned by Asset Manager rules."), FLinearColor(0.35f, 0.75f, 0.35f), [](const FAssetData& AssetData) { return AssetData.GetPrimaryAssetId().IsValid(); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NonPrimaryAssets"), LOCTEXT("NonPrimaryAssetsName", "Non-Primary Assets"), LOCTEXT("NonPrimaryAssetsTooltip", "Show assets without a PrimaryAssetId."), GeneralColor, [](const FAssetData& AssetData) { return !AssetData.GetPrimaryAssetId().IsValid(); });

		AddFilter(
			Specs,
			TEXT("CBTK_LargeAssets"),
			FText::Format(LOCTEXT("LargeAssetsName", "Large Assets > {0} MB"), FText::AsNumber(Settings.LargeAssetThresholdMB)),
			LOCTEXT("LargeAssetsTooltip", "Show assets whose on-disk size is above the configured threshold."),
			WarningColor,
			[ThresholdBytes = static_cast<int64>(Settings.LargeAssetThresholdMB) * 1024ll * 1024ll](FAssetFilterType InItem)
			{
				int64 DiskSizeBytes = 0;
				return GetDiskSizeBytes(InItem, DiskSizeBytes) && DiskSizeBytes >= ThresholdBytes;
			});

		AddFilter(
			Specs,
			TEXT("CBTK_TinyAssets"),
			FText::Format(LOCTEXT("TinyAssetsName", "Tiny Assets < {0} KB"), FText::AsNumber(Settings.TinyAssetThresholdKB)),
			LOCTEXT("TinyAssetsTooltip", "Show very small assets, useful for finding placeholder or almost-empty packages."),
			NamingColor,
			[ThresholdBytes = static_cast<int64>(Settings.TinyAssetThresholdKB) * 1024ll](FAssetFilterType InItem)
			{
				int64 DiskSizeBytes = 0;
				return GetDiskSizeBytes(InItem, DiskSizeBytes) && DiskSizeBytes > 0 && DiskSizeBytes <= ThresholdBytes;
			});

		AddAssetDataFilter(Specs, TEXT("CBTK_NameViolatesPrefix"), LOCTEXT("NameViolatesPrefixName", "Name Violates Prefix"), LOCTEXT("NameViolatesPrefixTooltip", "Show common asset classes whose names do not start with the expected prefix."), NamingColor, [](const FAssetData& AssetData) { return ViolatesKnownPrefixConvention(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NameHasSpaces"), LOCTEXT("NameHasSpacesName", "Name Has Spaces"), LOCTEXT("NameHasSpacesTooltip", "Show assets with spaces in their object name."), NamingColor, [](const FAssetData& AssetData) { return HasWhitespaceInName(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NameStartsLowercase"), LOCTEXT("NameStartsLowercaseName", "Name Starts Lowercase"), LOCTEXT("NameStartsLowercaseTooltip", "Show assets whose name starts with a lowercase ASCII letter."), NamingColor, [](const FAssetData& AssetData) { return HasLowercasePrefix(AssetData); });
	}

	void AddBlueprintFilters(TArray<FFilterSpec>& Specs)
	{
		const FLinearColor BlueprintColor(0.55f, 0.45f, 0.95f);
		const FLinearColor WarningColor(0.9f, 0.25f, 0.2f);

		AddAssetDataFilter(Specs, TEXT("CBTK_AllBlueprints"), LOCTEXT("AllBlueprintsName", "Blueprint Assets"), LOCTEXT("AllBlueprintsTooltip", "Show Blueprint-like assets detected by class name or Blueprint asset registry tags."), BlueprintColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_DataOnlyBlueprints"), LOCTEXT("DataOnlyBlueprintsName", "Data-Only Blueprints"), LOCTEXT("DataOnlyBlueprintsTooltip", "Show Blueprint assets whose asset registry tags mark them as data-only."), BlueprintColor, [](const FAssetData& AssetData) { bool bIsDataOnly = false; return IsBlueprintAsset(AssetData) && GetBoolTag(AssetData, FBlueprintTags::IsDataOnly, bIsDataOnly) && bIsDataOnly; });
		AddAssetDataFilter(Specs, TEXT("CBTK_ScriptedBlueprints"), LOCTEXT("ScriptedBlueprintsName", "Scripted Blueprints"), LOCTEXT("ScriptedBlueprintsTooltip", "Show Blueprint assets whose data-only tag is false."), BlueprintColor, [](const FAssetData& AssetData) { bool bIsDataOnly = false; return IsBlueprintAsset(AssetData) && GetBoolTag(AssetData, FBlueprintTags::IsDataOnly, bIsDataOnly) && !bIsDataOnly; });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintsWithComponents"), LOCTEXT("BlueprintsWithComponentsName", "Blueprints With Components"), LOCTEXT("BlueprintsWithComponentsTooltip", "Show Blueprint assets that add at least one Blueprint-owned component."), BlueprintColor, [](const FAssetData& AssetData) { return AssetData.GetTagValueRef<int32>(FBlueprintTags::NumBlueprintComponents) > 0; });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintsWithoutComponents"), LOCTEXT("BlueprintsWithoutComponentsName", "Blueprints Without Components"), LOCTEXT("BlueprintsWithoutComponentsTooltip", "Show Blueprint assets that have no Blueprint-owned components."), BlueprintColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && AssetData.GetTagValueRef<int32>(FBlueprintTags::NumBlueprintComponents) == 0; });
		AddAssetDataFilter(Specs, TEXT("CBTK_ReplicatedBlueprints"), LOCTEXT("ReplicatedBlueprintsName", "Replicated Blueprints"), LOCTEXT("ReplicatedBlueprintsTooltip", "Show Blueprints with replicated properties."), BlueprintColor, [](const FAssetData& AssetData) { return AssetData.GetTagValueRef<int32>(FBlueprintTags::NumReplicatedProperties) > 0; });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintNativeParent"), LOCTEXT("BlueprintNativeParentName", "Blueprint Has Native Parent"), LOCTEXT("BlueprintNativeParentTooltip", "Show Blueprints with a native parent class path tag."), BlueprintColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && HasNonEmptyTag(AssetData, FBlueprintTags::NativeParentClassPath); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintProjectParent"), LOCTEXT("BlueprintProjectParentName", "Blueprint Has Project Parent"), LOCTEXT("BlueprintProjectParentTooltip", "Show Blueprints whose parent class path points into /Game."), BlueprintColor, [](const FAssetData& AssetData) { FString ParentClassPath; return IsBlueprintAsset(AssetData) && GetStringTag(AssetData, FBlueprintTags::ParentClassPath, ParentClassPath) && ParentClassPath.Contains(TEXT("/Game/")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintInterfaces"), LOCTEXT("BlueprintInterfacesName", "Blueprint Has Interfaces"), LOCTEXT("BlueprintInterfacesTooltip", "Show Blueprints with implemented interface data in asset registry tags."), BlueprintColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && IsMeaningfulBlueprintListTag(AssetData, FBlueprintTags::ImplementedInterfaces); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintFindInBlueprintsData"), LOCTEXT("BlueprintFibDataName", "Blueprint Has FiB Data"), LOCTEXT("BlueprintFibDataTooltip", "Show Blueprints that have Find-in-Blueprints searchable data saved."), BlueprintColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && HasNonEmptyTag(AssetData, FBlueprintTags::FindInBlueprintsData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintMissingFindInBlueprintsData"), LOCTEXT("BlueprintMissingFibDataName", "Blueprint Missing FiB Data"), LOCTEXT("BlueprintMissingFibDataTooltip", "Show Blueprints without Find-in-Blueprints searchable data."), WarningColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && !HasNonEmptyTag(AssetData, FBlueprintTags::FindInBlueprintsData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintMissingDescription"), LOCTEXT("BlueprintMissingDescriptionName", "Blueprint Missing Description"), LOCTEXT("BlueprintMissingDescriptionTooltip", "Show Blueprints without a Blueprint description tag."), WarningColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && !HasNonEmptyTag(AssetData, FBlueprintTags::BlueprintDescription); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlueprintMissingCategory"), LOCTEXT("BlueprintMissingCategoryName", "Blueprint Missing Category"), LOCTEXT("BlueprintMissingCategoryTooltip", "Show Blueprints without a Blueprint category tag."), WarningColor, [](const FAssetData& AssetData) { return IsBlueprintAsset(AssetData) && !HasNonEmptyTag(AssetData, FBlueprintTags::BlueprintCategory); });
		AddAssetDataFilter(Specs, TEXT("CBTK_WidgetBlueprints"), LOCTEXT("WidgetBlueprintsName", "Widget Blueprints"), LOCTEXT("WidgetBlueprintsTooltip", "Show Widget Blueprint assets."), BlueprintColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("WidgetBlueprint")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_AnimBlueprints"), LOCTEXT("AnimBlueprintsName", "Anim Blueprints"), LOCTEXT("AnimBlueprintsTooltip", "Show Animation Blueprint assets."), BlueprintColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("AnimBlueprint")); });
	}

	void AddTextureFilters(TArray<FFilterSpec>& Specs)
	{
		const UContentBrowserToolkitSettings& Settings = *UContentBrowserToolkitSettings::Get();
		const FLinearColor TextureColor(0.15f, 0.65f, 0.85f);
		const FLinearColor WarningColor(0.9f, 0.25f, 0.2f);

		AddAssetDataFilter(Specs, TEXT("CBTK_TextureAssets"), LOCTEXT("TextureAssetsName", "Texture Assets"), LOCTEXT("TextureAssetsTooltip", "Show texture and render-target assets."), TextureColor, [](const FAssetData& AssetData) { return IsTextureAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_Texture2D"), LOCTEXT("Texture2DName", "Texture2D"), LOCTEXT("Texture2DTooltip", "Show Texture2D assets."), TextureColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("Texture2D")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_TextureCubes"), LOCTEXT("TextureCubesName", "Texture Cubes"), LOCTEXT("TextureCubesTooltip", "Show TextureCube assets."), TextureColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("TextureCube")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_RenderTargets"), LOCTEXT("RenderTargetsName", "Render Targets"), LOCTEXT("RenderTargetsTooltip", "Show texture render target assets."), TextureColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("TextureRenderTarget2D"), TEXT("TextureRenderTargetCube") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_VirtualTextures"), LOCTEXT("VirtualTexturesName", "Virtual Textures"), LOCTEXT("VirtualTexturesTooltip", "Show Runtime Virtual Texture and virtual-volume texture assets."), TextureColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("RuntimeVirtualTexture"), TEXT("SparseVolumeTexture") }); });
		AddAssetDataFilter(
			Specs,
			TEXT("CBTK_LargeTextures"),
			FText::Format(LOCTEXT("LargeTexturesName", "Large Textures >= {0}"), FText::AsNumber(Settings.LargeTextureMaxSize)),
			LOCTEXT("LargeTexturesTooltip", "Show texture assets whose MaxTextureSize tag is above the configured threshold."),
			WarningColor,
			[Threshold = static_cast<int64>(Settings.LargeTextureMaxSize)](const FAssetData& AssetData)
			{
				int64 MaxTextureSize = 0;
				return IsTextureAsset(AssetData) && GetIntTag(AssetData, TEXT("MaxTextureSize"), MaxTextureSize) && MaxTextureSize >= Threshold;
			});
		AddAssetDataFilter(Specs, TEXT("CBTK_InvalidTextureSource"), LOCTEXT("InvalidTextureSourceName", "Invalid Texture Source"), LOCTEXT("InvalidTextureSourceTooltip", "Show textures whose IsSourceValid tag is false."), WarningColor, [](const FAssetData& AssetData) { bool bSourceValid = true; return IsTextureAsset(AssetData) && GetBoolTag(AssetData, TEXT("IsSourceValid"), bSourceValid) && !bSourceValid; });
		AddAssetDataFilter(Specs, TEXT("CBTK_TextureNoMipmaps"), LOCTEXT("TextureNoMipmapsName", "Textures With No Mips"), LOCTEXT("TextureNoMipmapsTooltip", "Show textures whose MipGenSettings tag contains NoMipmaps."), WarningColor, [](const FAssetData& AssetData) { FString MipGenSettings; return IsTextureAsset(AssetData) && GetStringTag(AssetData, TEXT("MipGenSettings"), MipGenSettings) && MipGenSettings.Contains(TEXT("NoMipmaps"), ESearchCase::IgnoreCase); });
		AddAssetDataFilter(Specs, TEXT("CBTK_TexturePowerOfTwoAdjusted"), LOCTEXT("TexturePowerOfTwoAdjustedName", "Power-of-Two Adjusted"), LOCTEXT("TexturePowerOfTwoAdjustedTooltip", "Show textures with a PowerOfTwoMode other than None."), TextureColor, [](const FAssetData& AssetData) { FString PowerOfTwoMode; return IsTextureAsset(AssetData) && GetStringTag(AssetData, TEXT("PowerOfTwoMode"), PowerOfTwoMode) && HasMeaningfulTagValue(PowerOfTwoMode) && !PowerOfTwoMode.Contains(TEXT("None"), ESearchCase::IgnoreCase); });
	}

	void AddMaterialFilters(TArray<FFilterSpec>& Specs)
	{
		const FLinearColor MaterialColor(0.95f, 0.55f, 0.15f);

		AddAssetDataFilter(Specs, TEXT("CBTK_MaterialAssets"), LOCTEXT("MaterialAssetsName", "Material Assets"), LOCTEXT("MaterialAssetsTooltip", "Show materials, material instances, functions, and parameter collections."), MaterialColor, [](const FAssetData& AssetData) { return IsMaterialAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_Materials"), LOCTEXT("MaterialsName", "Materials"), LOCTEXT("MaterialsTooltip", "Show Material assets."), MaterialColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("Material")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_MaterialInstances"), LOCTEXT("MaterialInstancesName", "Material Instances"), LOCTEXT("MaterialInstancesTooltip", "Show Material Instance assets."), MaterialColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("MaterialInstance"), TEXT("MaterialInstanceConstant") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_MaterialFunctions"), LOCTEXT("MaterialFunctionsName", "Material Functions"), LOCTEXT("MaterialFunctionsTooltip", "Show Material Function assets."), MaterialColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("MaterialFunction"), TEXT("MaterialFunctionInstance") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_MaterialParameterCollections"), LOCTEXT("MaterialParameterCollectionsName", "Material Parameter Collections"), LOCTEXT("MaterialParameterCollectionsTooltip", "Show Material Parameter Collection assets."), MaterialColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("MaterialParameterCollection")); });
	}

	void AddMeshAndAnimationFilters(TArray<FFilterSpec>& Specs)
	{
		const UContentBrowserToolkitSettings& Settings = *UContentBrowserToolkitSettings::Get();
		const FLinearColor MeshColor(0.35f, 0.75f, 0.35f);
		const FLinearColor WarningColor(0.9f, 0.25f, 0.2f);

		AddAssetDataFilter(Specs, TEXT("CBTK_StaticMeshes"), LOCTEXT("StaticMeshesName", "Static Meshes"), LOCTEXT("StaticMeshesTooltip", "Show Static Mesh assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("StaticMesh")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_SkeletalMeshes"), LOCTEXT("SkeletalMeshesName", "Skeletal Meshes"), LOCTEXT("SkeletalMeshesTooltip", "Show Skeletal Mesh assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("SkeletalMesh")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_Skeletons"), LOCTEXT("SkeletonsName", "Skeletons"), LOCTEXT("SkeletonsTooltip", "Show Skeleton assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("Skeleton")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_PhysicsAssets"), LOCTEXT("PhysicsAssetsName", "Physics Assets"), LOCTEXT("PhysicsAssetsTooltip", "Show Physics Asset assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("PhysicsAsset")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NaniteEnabled"), LOCTEXT("NaniteEnabledName", "Nanite Enabled"), LOCTEXT("NaniteEnabledTooltip", "Show Static Meshes whose NaniteEnabled tag is true."), MeshColor, [](const FAssetData& AssetData) { bool bNaniteEnabled = false; return IsClass(AssetData, TEXT("StaticMesh")) && GetBoolTag(AssetData, TEXT("NaniteEnabled"), bNaniteEnabled) && bNaniteEnabled; });
		AddAssetDataFilter(Specs, TEXT("CBTK_NaniteDisabled"), LOCTEXT("NaniteDisabledName", "Nanite Disabled"), LOCTEXT("NaniteDisabledTooltip", "Show Static Meshes whose NaniteEnabled tag is false."), WarningColor, [](const FAssetData& AssetData) { bool bNaniteEnabled = false; return IsClass(AssetData, TEXT("StaticMesh")) && GetBoolTag(AssetData, TEXT("NaniteEnabled"), bNaniteEnabled) && !bNaniteEnabled; });
		AddAssetDataFilter(
			Specs,
			TEXT("CBTK_HighTriangleMeshes"),
			FText::Format(LOCTEXT("HighTriangleMeshesName", "High Triangles > {0}"), FText::AsNumber(Settings.HighTriangleThreshold)),
			LOCTEXT("HighTriangleMeshesTooltip", "Show Static Meshes whose Triangles tag is above the configured threshold."),
			WarningColor,
			[Threshold = static_cast<int64>(Settings.HighTriangleThreshold)](const FAssetData& AssetData)
			{
				int64 Triangles = 0;
				return IsClass(AssetData, TEXT("StaticMesh")) && GetIntTag(AssetData, TEXT("Triangles"), Triangles) && Triangles > Threshold;
			});
		AddAssetDataFilter(Specs, TEXT("CBTK_StaticMeshesNoCollision"), LOCTEXT("StaticMeshesNoCollisionName", "Static Meshes No Collision"), LOCTEXT("StaticMeshesNoCollisionTooltip", "Show Static Meshes whose collision primitive and collision section tags are zero."), WarningColor, [](const FAssetData& AssetData) { int64 CollisionPrims = 0; int64 SectionsWithCollision = 0; return IsClass(AssetData, TEXT("StaticMesh")) && GetIntTag(AssetData, TEXT("CollisionPrims"), CollisionPrims) && GetIntTag(AssetData, TEXT("SectionsWithCollision"), SectionsWithCollision) && CollisionPrims == 0 && SectionsWithCollision == 0; });
		AddAssetDataFilter(Specs, TEXT("CBTK_StaticMeshesSingleLOD"), LOCTEXT("StaticMeshesSingleLODName", "Static Meshes Single LOD"), LOCTEXT("StaticMeshesSingleLODTooltip", "Show Static Meshes whose LODs tag is one or zero."), WarningColor, [](const FAssetData& AssetData) { int64 LODs = 0; return IsClass(AssetData, TEXT("StaticMesh")) && GetIntTag(AssetData, TEXT("LODs"), LODs) && LODs <= 1; });
		AddAssetDataFilter(
			Specs,
			TEXT("CBTK_TooManyMaterialSlots"),
			FText::Format(LOCTEXT("TooManyMaterialSlotsName", "Material Slots > {0}"), FText::AsNumber(Settings.TooManyMaterialSlotsThreshold)),
			LOCTEXT("TooManyMaterialSlotsTooltip", "Show Static Meshes whose Materials tag is above the configured threshold."),
			WarningColor,
			[Threshold = static_cast<int64>(Settings.TooManyMaterialSlotsThreshold)](const FAssetData& AssetData)
			{
				int64 Materials = 0;
				return IsClass(AssetData, TEXT("StaticMesh")) && GetIntTag(AssetData, TEXT("Materials"), Materials) && Materials > Threshold;
			});
		AddAssetDataFilter(
			Specs,
			TEXT("CBTK_TooManyUVChannels"),
			FText::Format(LOCTEXT("TooManyUVChannelsName", "UV Channels > {0}"), FText::AsNumber(Settings.TooManyUVChannelsThreshold)),
			LOCTEXT("TooManyUVChannelsTooltip", "Show Static Meshes whose UVChannels tag is above the configured threshold."),
			WarningColor,
			[Threshold = static_cast<int64>(Settings.TooManyUVChannelsThreshold)](const FAssetData& AssetData)
			{
				int64 UVChannels = 0;
				return IsClass(AssetData, TEXT("StaticMesh")) && GetIntTag(AssetData, TEXT("UVChannels"), UVChannels) && UVChannels > Threshold;
			});
		AddAssetDataFilter(Specs, TEXT("CBTK_ComplexCollisionMeshes"), LOCTEXT("ComplexCollisionMeshesName", "Complex Collision Meshes"), LOCTEXT("ComplexCollisionMeshesTooltip", "Show Static Meshes whose CollisionComplexity tag contains UseComplexAsSimple."), WarningColor, [](const FAssetData& AssetData) { FString CollisionComplexity; return IsClass(AssetData, TEXT("StaticMesh")) && GetStringTag(AssetData, TEXT("CollisionComplexity"), CollisionComplexity) && CollisionComplexity.Contains(TEXT("UseComplexAsSimple"), ESearchCase::IgnoreCase); });

		AddAssetDataFilter(Specs, TEXT("CBTK_AnimationAssets"), LOCTEXT("AnimationAssetsName", "Animation Assets"), LOCTEXT("AnimationAssetsTooltip", "Show common animation asset classes."), MeshColor, [](const FAssetData& AssetData) { return IsAnimationAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_AnimSequences"), LOCTEXT("AnimSequencesName", "Anim Sequences"), LOCTEXT("AnimSequencesTooltip", "Show AnimSequence assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("AnimSequence")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_AnimMontages"), LOCTEXT("AnimMontagesName", "Anim Montages"), LOCTEXT("AnimMontagesTooltip", "Show AnimMontage assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("AnimMontage")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_BlendSpaces"), LOCTEXT("BlendSpacesName", "Blend Spaces"), LOCTEXT("BlendSpacesTooltip", "Show BlendSpace and BlendSpace1D assets."), MeshColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("BlendSpace"), TEXT("BlendSpace1D"), TEXT("AimOffsetBlendSpace") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_PoseAssets"), LOCTEXT("PoseAssetsName", "Pose Assets"), LOCTEXT("PoseAssetsTooltip", "Show PoseAsset assets."), MeshColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("PoseAsset")); });
	}

	void AddAudioFilters(TArray<FFilterSpec>& Specs)
	{
		const UContentBrowserToolkitSettings& Settings = *UContentBrowserToolkitSettings::Get();
		const FLinearColor AudioColor(0.1f, 0.7f, 0.55f);
		const FLinearColor WarningColor(0.9f, 0.25f, 0.2f);

		AddAssetDataFilter(Specs, TEXT("CBTK_AudioAssets"), LOCTEXT("AudioAssetsName", "Audio Assets"), LOCTEXT("AudioAssetsTooltip", "Show sound, MetaSound, dialogue, class, and mix assets."), AudioColor, [](const FAssetData& AssetData) { return IsAudioAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_SoundWaves"), LOCTEXT("SoundWavesName", "Sound Waves"), LOCTEXT("SoundWavesTooltip", "Show SoundWave assets."), AudioColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("SoundWave")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_SoundCues"), LOCTEXT("SoundCuesName", "Sound Cues"), LOCTEXT("SoundCuesTooltip", "Show SoundCue assets."), AudioColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("SoundCue")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_MetaSounds"), LOCTEXT("MetaSoundsName", "MetaSounds"), LOCTEXT("MetaSoundsTooltip", "Show MetaSound Source and Patch assets."), AudioColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("MetaSoundSource"), TEXT("MetaSoundPatch") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_DialogueAudio"), LOCTEXT("DialogueAudioName", "Dialogue Audio"), LOCTEXT("DialogueAudioTooltip", "Show DialogueWave and DialogueVoice assets."), AudioColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("DialogueWave"), TEXT("DialogueVoice") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_LoopingSoundWaves"), LOCTEXT("LoopingSoundWavesName", "Looping SoundWaves"), LOCTEXT("LoopingSoundWavesTooltip", "Show SoundWave assets whose bLooping tag is true."), AudioColor, [](const FAssetData& AssetData) { bool bLooping = false; return IsClass(AssetData, TEXT("SoundWave")) && GetBoolTag(AssetData, TEXT("bLooping"), bLooping) && bLooping; });
		AddAssetDataFilter(Specs, TEXT("CBTK_MultichannelSoundWaves"), LOCTEXT("MultichannelSoundWavesName", "Multichannel SoundWaves"), LOCTEXT("MultichannelSoundWavesTooltip", "Show SoundWave assets with NumChannels greater than two."), WarningColor, [](const FAssetData& AssetData) { int64 NumChannels = 0; return IsClass(AssetData, TEXT("SoundWave")) && GetIntTag(AssetData, TEXT("NumChannels"), NumChannels) && NumChannels > 2; });
		AddAssetDataFilter(Specs, TEXT("CBTK_LowSampleRateSoundWaves"), LOCTEXT("LowSampleRateSoundWavesName", "Low Sample Rate SoundWaves"), LOCTEXT("LowSampleRateSoundWavesTooltip", "Show SoundWave assets with SampleRate at or below 22050."), WarningColor, [](const FAssetData& AssetData) { int64 SampleRate = 0; return IsClass(AssetData, TEXT("SoundWave")) && GetIntTag(AssetData, TEXT("SampleRate"), SampleRate) && SampleRate > 0 && SampleRate <= 22050; });
		AddAssetDataFilter(
			Specs,
			TEXT("CBTK_LowAudioQualitySoundWaves"),
			FText::Format(LOCTEXT("LowAudioQualitySoundWavesName", "Audio Quality <= {0}"), FText::AsNumber(Settings.LowAudioQualityThreshold)),
			LOCTEXT("LowAudioQualitySoundWavesTooltip", "Show SoundWave assets whose CompressionQuality tag is at or below the configured threshold."),
			WarningColor,
			[Threshold = static_cast<int64>(Settings.LowAudioQualityThreshold)](const FAssetData& AssetData)
			{
				int64 CompressionQuality = 0;
				return IsClass(AssetData, TEXT("SoundWave")) && GetIntTag(AssetData, TEXT("CompressionQuality"), CompressionQuality) && CompressionQuality > 0 && CompressionQuality <= Threshold;
			});
		AddAssetDataFilter(Specs, TEXT("CBTK_CloudStreamingAudio"), LOCTEXT("CloudStreamingAudioName", "Cloud Streaming Audio"), LOCTEXT("CloudStreamingAudioTooltip", "Show SoundWave assets with bEnableCloudStreaming set."), AudioColor, [](const FAssetData& AssetData) { bool bEnableCloudStreaming = false; return IsClass(AssetData, TEXT("SoundWave")) && GetBoolTag(AssetData, TEXT("bEnableCloudStreaming"), bEnableCloudStreaming) && bEnableCloudStreaming; });
		AddAssetDataFilter(Specs, TEXT("CBTK_MatureAudio"), LOCTEXT("MatureAudioName", "Mature Audio"), LOCTEXT("MatureAudioTooltip", "Show SoundWave assets marked as mature."), WarningColor, [](const FAssetData& AssetData) { bool bMature = false; return IsClass(AssetData, TEXT("SoundWave")) && GetBoolTag(AssetData, TEXT("bMature"), bMature) && bMature; });
	}

	void AddMapFxAndDataFilters(TArray<FFilterSpec>& Specs)
	{
		const FLinearColor FxColor(0.75f, 0.35f, 0.85f);
		const FLinearColor DataColor(0.2f, 0.65f, 0.5f);

		AddAssetDataFilter(Specs, TEXT("CBTK_Maps"), LOCTEXT("MapsName", "Maps"), LOCTEXT("MapsTooltip", "Show World/map assets."), FxColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("World")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NiagaraSystems"), LOCTEXT("NiagaraSystemsName", "Niagara Systems"), LOCTEXT("NiagaraSystemsTooltip", "Show Niagara System assets."), FxColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("NiagaraSystem")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_NiagaraEmitters"), LOCTEXT("NiagaraEmittersName", "Niagara Emitters"), LOCTEXT("NiagaraEmittersTooltip", "Show Niagara Emitter assets."), FxColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("NiagaraEmitter")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_DataLikeAssets"), LOCTEXT("DataLikeAssetsName", "Data-Like Assets"), LOCTEXT("DataLikeAssetsTooltip", "Show common data asset, table, curve, struct, and enum classes."), DataColor, [](const FAssetData& AssetData) { return IsDataLikeAsset(AssetData); });
		AddAssetDataFilter(Specs, TEXT("CBTK_DataAssets"), LOCTEXT("DataAssetsName", "Data Assets"), LOCTEXT("DataAssetsTooltip", "Show DataAsset and PrimaryDataAsset assets."), DataColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("DataAsset"), TEXT("PrimaryDataAsset") }) || ClassContains(AssetData, TEXT("DataAsset")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_DataTables"), LOCTEXT("DataTablesName", "Data Tables"), LOCTEXT("DataTablesTooltip", "Show DataTable assets."), DataColor, [](const FAssetData& AssetData) { return IsClass(AssetData, TEXT("DataTable")); });
		AddAssetDataFilter(Specs, TEXT("CBTK_CurveAssets"), LOCTEXT("CurveAssetsName", "Curve Assets"), LOCTEXT("CurveAssetsTooltip", "Show CurveTable and common curve assets."), DataColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("CurveTable"), TEXT("CurveFloat"), TEXT("CurveVector"), TEXT("CurveLinearColor") }); });
		AddAssetDataFilter(Specs, TEXT("CBTK_UserDefinedTypes"), LOCTEXT("UserDefinedTypesName", "User Defined Types"), LOCTEXT("UserDefinedTypesTooltip", "Show UserDefinedStruct and UserDefinedEnum assets."), DataColor, [](const FAssetData& AssetData) { return IsAnyClass(AssetData, { TEXT("UserDefinedStruct"), TEXT("UserDefinedEnum") }); });
	}

	void AddCachedAuditFilters(TArray<FFilterSpec>& Specs)
	{
		if (!UContentBrowserToolkitSettings::Get()->bEnableCachedAuditFilters)
		{
			return;
		}

		const FLinearColor AuditColor(0.9f, 0.35f, 0.25f);
		const FLinearColor DependencyColor(0.55f, 0.55f, 0.9f);

		AddAssetDataFilter(Specs, TEXT("CBTK_AuditNoReferencers"), LOCTEXT("AuditNoReferencersName", "Audit: No Referencers"), LOCTEXT("AuditNoReferencersTooltip", "Show assets with no package referencers according to the cached AssetRegistry query."), AuditColor, [](const FAssetData& AssetData) { return !FAssetAuditCache::Get().GetAudit(AssetData).bHasReferencers; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditHasReferencers"), LOCTEXT("AuditHasReferencersName", "Audit: Has Referencers"), LOCTEXT("AuditHasReferencersTooltip", "Show assets with at least one package referencer according to the cached AssetRegistry query."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bHasReferencers; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditReferencedByMap"), LOCTEXT("AuditReferencedByMapName", "Audit: Referenced By Map"), LOCTEXT("AuditReferencedByMapTooltip", "Show assets referenced by at least one World package according to cached AssetRegistry queries."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bReferencedByMap; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditHasDependencies"), LOCTEXT("AuditHasDependenciesName", "Audit: Has Dependencies"), LOCTEXT("AuditHasDependenciesTooltip", "Show assets that reference at least one other package according to cached AssetRegistry queries."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bHasDependencies; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditReferencesEngine"), LOCTEXT("AuditReferencesEngineName", "Audit: References Engine"), LOCTEXT("AuditReferencesEngineTooltip", "Show assets with dependencies under /Engine."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bReferencesEngineContent; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditReferencesProject"), LOCTEXT("AuditReferencesProjectName", "Audit: References Project"), LOCTEXT("AuditReferencesProjectTooltip", "Show assets with dependencies under /Game."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bReferencesProjectContent; });
		AddAssetDataFilter(Specs, TEXT("CBTK_AuditReferencesPlugin"), LOCTEXT("AuditReferencesPluginName", "Audit: References Plugin"), LOCTEXT("AuditReferencesPluginTooltip", "Show assets with dependencies under plugin mount points."), DependencyColor, [](const FAssetData& AssetData) { return FAssetAuditCache::Get().GetAudit(AssetData).bReferencesPluginContent; });
	}

	void BuildFilterSpecs(TArray<FFilterSpec>& Specs)
	{
		AddGeneralFilters(Specs);
		AddBlueprintFilters(Specs);
		AddTextureFilters(Specs);
		AddMaterialFilters(Specs);
		AddMeshAndAnimationFilters(Specs);
		AddAudioFilters(Specs);
		AddMapFxAndDataFilters(Specs);
		AddCachedAuditFilters(Specs);
	}
}

class FContentBrowserToolkitPredicateFilter final : public FFrontendFilter
{
public:
	FContentBrowserToolkitPredicateFilter(TSharedPtr<FFrontendFilterCategory> InCategory, ContentBrowserToolkit::FFilterSpec InSpec)
		: FFrontendFilter(MoveTemp(InCategory))
		, Spec(MoveTemp(InSpec))
	{
	}

	virtual FString GetName() const override
	{
		return Spec.Name;
	}

	virtual FText GetDisplayName() const override
	{
		return Spec.DisplayName;
	}

	virtual FText GetToolTipText() const override
	{
		return Spec.ToolTipText;
	}

	virtual FLinearColor GetColor() const override
	{
		return Spec.Color;
	}

	virtual FName GetIconName() const override
	{
		return FName(TEXT("Icons.Filter"));
	}

	virtual bool PassesFilter(FAssetFilterType InItem) const override
	{
		return Spec.Predicate && Spec.Predicate(InItem);
	}

private:
	ContentBrowserToolkit::FFilterSpec Spec;
};

void UContentBrowserToolkitFilterExtension::AddFrontEndFilterExtensions(TSharedPtr<FFrontendFilterCategory> /*DefaultCategory*/, TArray<TSharedRef<FFrontendFilter>>& InOutFilterList) const
{
	TSharedRef<FFrontendFilterCategory> ToolkitCategory = MakeShared<FFrontendFilterCategory>(
		LOCTEXT("ToolkitCategoryName", "Content Browser Toolkit"),
		LOCTEXT("ToolkitCategoryTooltip", "Additional filters for auditing and organizing project content."));

	TArray<ContentBrowserToolkit::FFilterSpec> Specs;
	ContentBrowserToolkit::BuildFilterSpecs(Specs);

	for (ContentBrowserToolkit::FFilterSpec& Spec : Specs)
	{
		InOutFilterList.Add(MakeShared<FContentBrowserToolkitPredicateFilter>(ToolkitCategory, MoveTemp(Spec)));
	}
}

#undef LOCTEXT_NAMESPACE
