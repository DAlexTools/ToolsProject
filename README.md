# ToolsProject

ToolsProject is an Unreal Engine 5.5 editor tooling workspace built around standalone Slate-based plugins. The project focuses on everyday editor productivity: creating C++ classes faster, managing Data Assets in bulk, and running targeted content validation directly inside the Unreal Editor.

## Overview

| Area | Plugin | What it adds |
| --- | --- | --- |
| C++ workflow | `CppTemplateGenerator` | A configurable `Tools` menu for creating native C++ classes from approved parent templates. |
| Content management | `DataAssetManager` | A dedicated Data Asset browser with filtering, bulk operations, validation, diffing, reference inspection, and editor utilities. |
| Content validation | `ValidatorX` | A Data Validation dashboard with switchable Blueprint and Material validators. |

## Requirements

- Unreal Engine `5.5`
- Windows editor target: `ToolsProjectEditor`
- Visual Studio toolchain compatible with Unreal Engine 5.5

## Project Modules

| Module | Type | Purpose |
| --- | --- | --- |
| `ToolsProject` | Runtime | Base game module for the Unreal project. |
| `ToolsProjectEditor` | Editor | Editor-side module used by the project. |
| `CppTemplateGenerator` | Editor plugin | Adds C++ template creation tools to the Unreal Editor. |
| `DataAssetManager` | Editor plugin | Adds a full Data Asset management window and supporting services. |
| `ValidatorX` | Editor plugin | Adds configurable validation tooling on top of Unreal's Data Validation system. |

## Plugins

### CppTemplateGenerator

`CppTemplateGenerator` adds a curated C++ class creation workflow to the Unreal Editor. Instead of manually choosing from the full class picker every time, the plugin exposes a configurable set of native parent classes as direct menu entries.

**Entry points**

- Menu: `Tools -> Programming -> New C++ Template...`
- Settings: `Project Settings -> Plugins -> C++ Template Generator`

**What it can do**

- Create new native C++ classes through Unreal's standard `Add Code to Project` dialog.
- Expose frequently used parent classes as direct menu actions.
- Allow the template list to be configured from plugin settings.
- Filter out unsupported template entries before they appear in the menu.
- Ignore duplicate, non-native, deprecated, or invalid classes.
- Support Actor and Actor Component based templates.

**Default templates**

- `AActor`
- `UActorComponent`
- `APawn`
- `ACharacter`
- `AGameModeBase`
- `AHUD`

This plugin is useful when a project has a preferred set of base classes and the team wants a faster, cleaner way to create new gameplay C++ types.

### DataAssetManager

`DataAssetManager` is a dedicated editor window for browsing, inspecting, validating, and maintaining Data Assets. It is designed for projects with many Data Assets where the Content Browser alone is not enough for repeated cleanup and review workflows.

**Entry points**

- Menu: `Tools -> Data Asset Manager`
- Window: standalone Nomad tab inside the Unreal Editor
- Settings: `Project Settings -> Plugins -> DataAssetManager`

**Browsing and filtering**

- Scan configured asset directories. The default scan root is `/Game`.
- Browse Data Assets from a dedicated tree/list UI.
- Search by asset name and related asset information.
- Filter by Data Asset type.
- Filter assets located inside plugin content paths.
- Show only modified assets.
- Show only assets with validation issues.
- Toggle table columns such as source control state, validation state, name, type, disk size, and path.

**Asset operations**

- Create a new Data Asset in the selected folder.
- Open selected Data Assets in the editor.
- Save selected Data Assets.
- Save all dirty Data Assets.
- Rename assets inline.
- Duplicate selected assets.
- Move selected assets.
- Delete selected assets.
- Sync selected assets in the Content Browser.
- Open selected assets in the Property Matrix.
- Reset editable properties back to the class default object.

**Clipboard and navigation tools**

- Copy selected asset references.
- Copy selected disk paths.
- Open Reference Viewer.
- Open Size Map.
- Open Asset Audit.
- Open the output log, message log, plugin settings, and source control dialog from the tool UI.

**Validation**

- Validate selected Data Assets.
- Validate all loaded/scanned Data Assets.
- Cache validation state for fast table feedback.
- Display validation status as a dedicated list column.
- Filter the list to invalid assets only.

**Reference inspection**

- Inspect dependencies and referencers for selected Data Assets.
- Show unresolved package references.
- Open or sync inspected assets from the reference inspector.

**Data Asset diff**

- Compare two selected Data Assets of the same class.
- Display editable top-level property differences.
- Copy property values from left to right or right to left.
- Use the diff tool as a focused review workflow before applying bulk changes.

**Settings and customization**

- Configure scan roots.
- Exclude asset types from scans.
- Configure documentation URL.
- Configure random integer and float clamp ranges used by editor utility actions.
- Configure root UI color used by the tool.

### ValidatorX

`ValidatorX` is an editor validation plugin built on top of Unreal's Data Validation system. It provides a Slate dashboard where validators can be searched, enabled, disabled, and managed without digging through engine settings.

**Entry points**

- Menu: `Tools -> DataValidation -> Open ValidatorX`
- Window: `ValidatorX` Nomad tab
- Integrates with: `UEditorValidatorSubsystem`

**What it can do**

- Register ValidatorX validators with Unreal's editor validation subsystem.
- Display all ValidatorX validators in a searchable table.
- Show validator type, display name, state, and enabled checkbox.
- Enable or disable individual validators.
- Enable all validators at once.
- Disable all validators at once.
- Persist validator enabled state to config.
- Open Blueprint graph locations from validation messages when a validator reports a node-specific issue.

**Blueprint validators**

| Validator | Detects |
| --- | --- |
| `BranchConditionValidator` | Branch nodes with an unconnected condition pin that will use the default boolean value. |
| `EmptyBranchValidator` | Branch nodes with disconnected execution outputs. |
| `DeadBranchValidator` | Branch execution paths that do not lead to meaningful logic. |
| `DebugCallValidator` | Debug-only Blueprint calls such as print/debug helper nodes. |
| `DefaultAssignmentValidator` | Suspicious default-value assignment patterns in Blueprint graphs. |
| `EmptyFunctionValidator` | User functions that have no meaningful graph body. |
| `EmptyMacroValidator` | Macro graphs that have no meaningful graph body. |
| `UnusedFunctionValidator` | User functions that are not referenced. |
| `UnusedMacroValidator` | Macro graphs that are not referenced. |
| `UnusedNodeValidator` | Nodes that are not connected to the graph workflow. |
| `GlobalVariableNeverUsedValidator` | Blueprint member variables that are never used. |
| `LocalVariableNeverUsedValidator` | Local variables that are never used. |
| `LocalGlobalNameConflictValidator` | Local variables that conflict with Blueprint member variable names. |
| `CircularDependencyValidator` | Circular graph/function dependency patterns. |
| `LongFunctionValidator` | Blueprint functions or graphs that exceed the configured complexity threshold. |
| `TickUsageValidator` | Connected `Event Tick` usage. |
| `UnhandledCastFailureValidator` | Dynamic casts where the failure path is not handled. |
| `UnboundEventDispatcherValidator` | Event dispatchers that are declared but never bound. |

**Material validators**

| Validator | Detects |
| --- | --- |
| `MaterialPositionOffsetValidator` | Materials using position offset features such as World Position Offset. |
| `TextureSampleCountMaterialValidator` | Materials that exceed the expected texture sample count. |
| `TranslucentMaterialValidator` | Materials using translucent blend modes. |
| `TwoSidedMaterialValidator` | Materials with two-sided rendering enabled. |

ValidatorX is intended for editor-time quality gates, project cleanup, and review workflows where content issues should be visible before they become runtime problems.

## Testing

Each plugin has a dedicated automation test module:

| Plugin | Test module |
| --- | --- |
| `CppTemplateGenerator` | `CppTemplateGeneratorTests` |
| `DataAssetManager` | `DataAssetManagerTests` |
| `ValidatorX` | `ValidatorXTests` |

Example commands:

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" ToolsProjectEditor Win64 Development -Project="C:\Users\admin\Documents\GitProjects\ToolsProject\ToolsProject.uproject" -WaitMutex -NoHotReloadFromIDE
```

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\admin\Documents\GitProjects\ToolsProject\ToolsProject.uproject" -ExecCmds="Automation RunTests DataAssetManager; Quit" -unattended -nop4 -nosplash
```

```powershell
& "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\admin\Documents\GitProjects\ToolsProject\ToolsProject.uproject" -ExecCmds="Automation RunTests ValidatorX; Quit" -unattended -nop4 -nosplash
```

## Repository Layout

```text
ToolsProject/
  Plugins/
    CppTemplateGenerator/
    DataAssetManager/
    ValidatorX/
  Source/
    ToolsProject/
    ToolsProjectEditor/
  ToolsProject.uproject
```

## Status

The project is in active development. The plugins are usable editor tools, but APIs, UI details, and validation rules may continue to change as the toolset grows.

## Contributing

Contributions are welcome. Keep changes focused, include tests for behavior changes where practical, and verify editor automation tests for the plugin you modify.

## License

MIT License.
