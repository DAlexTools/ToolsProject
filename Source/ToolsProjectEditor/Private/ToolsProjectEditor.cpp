#include "ToolsProjectEditor.h"

DEFINE_LOG_CATEGORY(ToolsProjectEditor);

#define LOCTEXT_NAMESPACE "FToolsProjectEditor"


static FAutoConsoleCommand ImportTexturesCmd(
	TEXT("Import.Textures"),
	TEXT("Import textures from folder"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		
	}));

void FToolsProjectEditor::StartupModule()
{
	UE_LOG(ToolsProjectEditor, Warning, TEXT("ToolsProjectEditor module has been loaded"));
}

void FToolsProjectEditor::ShutdownModule()
{
	UE_LOG(ToolsProjectEditor, Warning, TEXT("ToolsProjectEditor module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FToolsProjectEditor, ToolsProjectEditor)