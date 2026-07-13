# ToolsProject

ToolsProject is an Unreal Engine 5.5 editor tooling workspace built around standalone Slate-based plugins. The project focuses on everyday editor productivity: creating C++ classes faster, managing Data Assets in bulk, and running targeted content validation directly inside the Unreal Editor.

## Overview

| Area | Plugin | What it adds |
| --- | --- | --- |
| C++ workflow | `CppTemplateGenerator` | A configurable `Tools` menu for creating native C++ classes from approved parent templates. |
| Content management | `DataAssetManager` | A dedicated Data Asset browser with filtering, bulk operations, validation, diffing, reference inspection, and editor utilities. |
| Scene organization | `OutlinerToolkit` | Scene Outliner columns, filters, actor batch actions, and a world audit panel. |
| Text editing | `UNotepad` | A dockable in-editor notepad for text, code, JSON, and CSV files. |
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
| `OutlinerToolkit` | Editor plugin | Extends Scene Outliner with extra columns, filters, context actions, and audit tooling. |
| `UNotepad` | Editor plugin | Adds a tabbed source/text editor directly inside the Unreal Editor. |
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

### OutlinerToolkit

`OutlinerToolkit` extends the Unreal Scene Outliner with practical production controls. It adds custom columns, custom filters, actor context menu actions, and a dedicated audit window for finding common level organization and performance issues.

**Entry points**

- Menu: `Tools -> Outliner Toolkit -> Outliner Toolkit Audit`
- Settings: `Tools -> Outliner Toolkit -> Outliner Toolkit Settings`
- Actor context menu: `Outliner Toolkit`
- Window: `Outliner Toolkit Audit` Nomad tab
- Integrates with: Scene Outliner, Level Editor, Project Settings

**Scene Outliner columns**

| Column | What it controls or displays |
| --- | --- |
| `HiddenInGame` | Toggle Hidden In Game for scene components on an actor. |
| `ActorLock` | Show or change actor lock state from the outliner. |
| `SimulatePhysics` | Toggle physics simulation for supported primitive components. |
| `SetMobility` | Set component mobility to Static, Stationary, or Movable. |
| `Tick` | Toggle actor tick when the actor supports ticking. |
| `CastShadows` | Toggle shadow casting on primitive components. |
| `Nanite` | Toggle Nanite on static mesh assets used by an actor. |
| `NaniteKeepTrianglePercent` | Edit Nanite Keep Triangle Percent for static mesh assets used by an actor. |
| `CollisionPreset` | Set collision preset across primitive components. |
| `GenerateOverlapEvents` | Toggle overlap event generation. |
| `CustomDepth` | Toggle Custom Depth rendering. |
| `CustomDepthStencil` | Edit Custom Depth stencil value. |
| `ActorTags` | View, edit, and sort actor tags. |

**Scene Outliner filters**

- Movable actors.
- Hidden in game actors.
- Actors casting shadows.
- Actors generating overlap events.
- Actors using Custom Depth.
- Actors with non-zero Custom Stencil values.
- Untagged actors and actors with tags.
- Movable actors that cast shadows.
- Performance-risk actors with tick, physics, overlap events, or movable shadows.
- Invalid physics mobility.
- Too many material slots.
- Too many components.
- Non-uniform or negative scale.
- Editor-only actors.
- Actors without an Outliner folder.
- Invalid static mesh assignments.
- Collision enabled and no-collision actors.
- Invalid material slots.
- Tick enabled actors.
- Physics enabled actors.

**Actor context tools**

- Group selected actors into a new folder-backed group.
- Ungroup selected actor groups while keeping members selected.
- Copy and paste common settings such as tick, Hidden In Game, and mobility.
- Copy and paste rendering settings such as shadows and Custom Depth.
- Copy and paste collision, physics, tags, and folder settings.
- Add label prefixes or suffixes to selected actors.
- Bulk enable or disable tick, Hidden In Game, shadows, Custom Depth, overlap events, and physics simulation.

**Audit panel**

- Audit selected actors, current level, visible actors, or the whole world.
- Filter audit results by severity, category, issue type, actor name, and ignored state.
- Display actor-level issue summaries and a detailed issue panel.
- Select, focus, and open actors from audit results.
- Open static mesh assets referenced by audited actors.
- Copy selected actor path or issue text to the clipboard.
- Ignore and restore ignored audit issues persistently.
- Apply supported fixes, such as moving actors into an audit folder or making physics-simulating components Movable.

**Audit criteria**

| Criterion | Detects |
| --- | --- |
| `TickEnabled` | Actors with enabled actor tick. |
| `NoFolder` | Actors that are not assigned to an Outliner folder. |
| `EditorOnly` | Actors marked as editor-only. |
| `TooManyComponents` | Actors with more components than the configured threshold. |
| `BadActorScale` | Actors with non-uniform or negative actor scale. |
| `BadComponentScale` | Scene components with non-uniform or negative relative scale. |
| `PhysicsEnabled` | Primitive components simulating physics. |
| `InvalidPhysicsMobility` | Physics-simulating primitive components that are not Movable. |
| `OverlapEvents` | Primitive components generating overlap events. |
| `MovableShadows` | Movable primitive components that cast shadows. |
| `InvalidStaticMesh` | Static mesh components without an assigned static mesh. |
| `InvalidMaterials` | Static mesh material slots that resolve to no material. |
| `TooManyMaterials` | Actors whose static mesh components exceed the material slot threshold. |

**Settings**

- Material slot count threshold.
- Component count threshold.
- Scale tolerance for uniform-scale checks.
- Per-criterion severity overrides.
- Persistent ignored audit issue keys.

### UNotepad

`UNotepad` is a dockable editor notepad for quick source and data-file editing without leaving Unreal Editor. It supports multiple document modes, tabs, split document groups, editor commands, and file operations backed by dedicated services.

**Entry points**

- Menu: `Tools -> UNotepad`
- Toolbar: `UNotepad` button in the Level Editor toolbar
- Content Browser: `Open in UNotepad` for supported source files
- Window: `UNotepad` Nomad tab
- Settings: `Project Settings -> Plugins -> UNotepad`

**Document workflow**

- Create untitled documents.
- Open text files from disk.
- Open supported source files from the Content Browser.
- Save and Save As documents.
- Close documents with dirty-state prompts.
- Keep multiple documents open in tabs.
- Split the workspace into vertical or horizontal document groups.
- Move documents between adjacent groups.
- Track dirty state against saved content.
- Maintain undo and redo history for document content.

**Document modes**

| Mode | Purpose |
| --- | --- |
| `Text` | General plain-text editing. |
| `Code` | Source-code editing with code-oriented actions. |
| `Json` | JSON editing, validation, and pretty formatting. |
| `Csv` | CSV editing, validation, parsing, and normalized formatting. |

**Editing tools**

- Toggle line numbers.
- Toggle whitespace display.
- Search forward and backward.
- Replace current match or replace all matches.
- Optional case-sensitive search.
- Go to line.
- Toggle line comments for a selection.
- Duplicate the current line or selection.
- Move the current line up or down.
- Trim trailing whitespace.
- Convert tabs to spaces.
- Convert leading spaces to tabs.
- Ensure final newline.
- Normalize line endings to `LF` or `CRLF`.
- Open the matching header/source file pair.

**Project integration**

- Build a Solution Explorer tree from configured source file extensions.
- Refresh the source tree from the UI.
- Double-click source tree files to open them in UNotepad.
- Compile the current Unreal project code from the notepad toolbar/menu.
- Open UNotepad settings from the tool.

**Settings**

- Editor font size.
- Tab size.
- Show line numbers by default.
- Show whitespace by default.
- Show Solution Explorer by default.
- Supported source file extensions.

Default source extensions:

- `h`, `hh`, `hpp`, `hxx`
- `inl`, `ipp`
- `cpp`, `cc`, `cxx`
- `cs`

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
| `OutlinerToolkit` | `OutlinerToolkitTests` |
| `UNotepad` | `UNotepadTests` |
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

Other available plugin test filters:

- `Automation RunTests CppTemplateGenerator`
- `Automation RunTests DataAssetManager`
- `Automation RunTests OutlinerToolkit`
- `Automation RunTests UNotepad`
- `Automation RunTests ValidatorX`

## Repository Layout

```text
ToolsProject/
  Plugins/
    CppTemplateGenerator/
    DataAssetManager/
    OutlinerToolkit/
    UNotepad/
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
