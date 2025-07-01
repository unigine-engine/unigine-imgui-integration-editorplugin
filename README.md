# ImGuiSamples Plugin

The ***ImGuiSamples Plugin*** demonstrates how to integrate the [Dear ImGui](https://github.com/ocornut/imgui) library with **UnigineEditor**, and how to use the [Editor's API](https://developer.unigine.com/en/docs/latest/api/editor/index.html) to build advanced custom tools (create new buttons, menus, widgets and more) for various development tasks. The sample is also available on the [Add-On Store](https://store.unigine.com/add-on/1efb3c7d-5082-6e82-8692-7342af84d483/description) as **ImGuiSamples Plugin**.

![ImGuiSamples Plugin](https://developer.unigine.com/en/docs/latest/editor2/imgui_samples_plugin/anim_sm.gif)

The plugin adds a custom window to the UnigineEditor with three tabs, each showcasing a different sample:

### <ins>Spline Editor</ins>

This sample demonstrates how to use the Editor's Undo/Redo system via the API, work with hotkeys, and implement custom visualization, using spline editing.

When **Edit Mode** is enabled, the spline editor becomes active. Available actions (remappable via **Windows → Settings → Hotkeys**):

- **LMB** - Select a point
- **Shift + LMB** - Add/remove a point or append to the spline
- **Del** - Remove selected point
- **F** - Focus camera on the selected point
- **Ctrl + Z** - Undo
- **Ctrl + Y** - Redo.

### <ins>Editor (Immediate Mode)</ins>

This sample demonstrates how to use the Editor and Engine widgets in immediate mode (ImGui-like style) using the `EditorImmediate` class. It's simple to use, but has a number of limitations. Best for prototypes and simple tools.

In the sample, four connected points form a polygon at the center of the world. Click **Edit Mode** to:

- Display manipulators for each point
- Move points using the manipulators
- Use the ***F*** key to focus the camera on the polygon.

The current position of each point `(X, Y, Z)` is updated each frame in the widget at the top-left corner of the viewport.

### <ins>Components</ins>

This sample shows how to work with C++ components inside the Editor.

- Click *Initialize the ComponentSystem* to register and activate the `NodeRotation` component (a property written in C++).
- Assign the component to any node. It starts rotating immediately.
- Control rotation speed using three sliders for **X, Y, Z** axes.

## How to Run the Sample

### Prerequisites

- **UNIGINE SDK Browser** (latest version)
- **UNIGINE SDK Community** or **Engineering** edition (**Sim** upgrade supported)
- **Visual Studio 2022** (recommended)

### Step-by-Step Guide

To get started with the **ImGuiSamples Plugin**:

1. **Clone or download** the sample.

2. **Open SDK Browser** and make sure you have the latest version.

3. **Add the sample project to SDK Browser**:
   - Go to the *My Projects* tab.
   - Click *Add Existing*, select the `.project` file from the cloned folder (matching your OS - `*-win-*`/`*-lin-*`, edition, precision), and click *Import Project*.

     ![Add Project](https://developer.unigine.com/en/docs/latest/sdk/api_samples/third_party/photon/add_project.png)

> [!NOTE]
> If you're using **UNIGINE SDK *Sim***, select the ***Engineering*** `*-eng-sim-*.project` file when importing the sample. After import, you can upgrade the project to the **Sim** version directly in SDK Browser - just click *Upgrade*, choose the SDK **Sim** version, and adjust any additional settings you want to use in the configuration window that opens.

4. **Repair the project**:
   - After importing, you'll see a **Repair** warning - this is expected, as only essential files are stored in the Git repository. SDK Browser will restore the rest.

![Repair Project](https://developer.unigine.com/en/docs/latest/sdk/api_samples/third_party/repair_project.png)

   - Click *Repair* and then *Configure Project*.

4. **Install required dependencies**:
   - The sample uses the **Qt 5.12.3** framework. Make sure you have **Qt 5.12.3** installed and that the corresponding **QT environment variable** is correctly set. For this sample, Qt will be located using the `QTROOT` environment variable.
You can set it, for example, as follows: click *Start*, type *Environment Variables*, and open *Edit the system environment variables*. In the *System Environment Variables* dialog, click *New* (or *Edit*, if it already exists) and set:
       - **Variable name**: `QTROOT`
       - **Value**: full Qt path (e.g., `C:\Qt\5.12.3\msvc2017_64`)
         ![QT Setup](https://developer.unigine.com/en/docs/latest/editor2/imgui_samples_plugin/qt_setup.png)
       - Restart your IDE or system to apply changes.

5. **Open the project in your IDE**:
   - Launch the recommended Visual Studio 2022 (other C++ IDE with CMake support can be used as well).
   - Open the folder: `source/plugins/Unigine/ImGuiSamples`.
   - Confirm `CMakeLists.txt` is highlighted in **bold** (ready to build).

> [!WARNING]
   > By default, the project uses **single precision (float)**. To enable **double precision**, modify `CMakeLists.txt`:
   >```diff
   > - set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")
   > + set(UNIGINE_DOUBLE True CACHE BOOL "Double coords")
   >```
   > Make sure this setting matches the `.project` file you selected (e.g., `*-double.project` for double precision builds).

7. **Build and start the Editor**:
   - Open Editor on your project's card in SDK Browser.
   - Plugin loads automatically from: `your_project/bin/plugins/Unigine/ImGuiSamples`.
>
   >[!TIP]
   > To use this plugin in **another UNIGINE project**, copy the entire `ImGuiSamples` folder into the same path in the target project: `your_other_project/bin/plugins/Unigine/ImGuiSamples`.
   > Make sure both projects use the same SDK version, build config, and precision.

>[!NOTE]
> If you are building the ***Debug*** version of the plugin, make sure to launch the ***Debug*** version of the Editor. Likewise, for a ***Release*** build, use the ***Release*** version of the Editor. Mismatched build configurations (e.g., *Debug* plugin in *Release* Editor) can cause crashes or prevent the plugin from loading.

If the plugin fails to run:
- Double-check all setup steps above to ensure nothing was skipped.
- Check that `UNIGINE_DOUBLE` in `CMakeLists.txt` matches the current build type (double/float).
- Ensure the correct `.project` file is used for your platform and SDK edition.
- Verify your SDK version is not older than the project's specified version.
- If CMake issues occur in Visual Studio, right-click the project and select: **Delete Cache and Reconfigure** and then **Build** again.