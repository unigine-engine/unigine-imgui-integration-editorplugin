##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 3.19)
project(ImGuiSamplesProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")

##==============================================================================
## Variables.
##==============================================================================
# Variables listed below are used to construct proper names for target binaries in accordance with UNIGINE Plugin Naming Convention:
#   - Windows: <VENDOR_NAME><target>_plugin_<precision>_x64.* for Windows binaries
#   - Linux: lib<VENDOR_NAME><target>_plugin_<precision>_x64.so for Linux binaries
# (https://developer.unigine.com/en/docs/latest/code/cpp/plugin?rlang=cpp#naming)

# DOUBLE_PRECISION - double (TRUE) or single (FALSE) precision for the target.
# VENDOR_NAME - name of a company or person offering the plugin.
# target - name of the plugin.
set(vendor_name Unigine)
set(target ImGuiSamples)

set(binary_name ${vendor_name}${target}_editorplugin)
if (UNIGINE_DOUBLE)
  string(APPEND binary_name "_double")
endif()
string(APPEND binary_name "_x64")


##==============================================================================
## Environment.
##==============================================================================
set(UNIGINE_SDK_PATH ${PROJECT_SOURCE_DIR}/../../../../)
set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin/)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib/)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include/)
set(OUTPUT_PLUGIN_DIR ${UNIGINE_BIN_DIR}plugins/${vendor_name}/ImGuiSamples)

##==============================================================================
## Dependencies.
##==============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake/modules)
find_package(Engine REQUIRED MODULE QUIET)
find_package(Editor REQUIRED MODULE QUIET)

# UNIGINE_QTROOT is Unigine related global variable that points to the effective
# Qt's distributive.
set(QT5_ROOT "$ENV{QTROOT}")
if (QT5_ROOT AND EXISTS "${QT5_ROOT}")
  list(APPEND CMAKE_PREFIX_PATH ${QT5_ROOT})
endif()

# The following construct is required as Editor's Debug build
# uses Release binaries of QT5 framework
set(temp_map_imported ${CMAKE_MAP_IMPORTED_CONFIG_DEBUG})
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELEASE)

# Find additional required packages
find_package(Qt5 5.12.3 CONFIG EXACT COMPONENTS Core Gui Widgets)

# Apply imported_location for qt executable
get_target_property(MOC_EXECUTABLE Qt5::moc IMPORTED_LOCATION)
set_target_properties(Qt5::moc PROPERTIES IMPORTED_LOCATION_RELEASE ${MOC_EXECUTABLE})
get_target_property(RCC_EXECUTABLE Qt5::rcc IMPORTED_LOCATION)
set_target_properties(Qt5::rcc PROPERTIES IMPORTED_LOCATION_RELEASE ${RCC_EXECUTABLE})
get_target_property(UIC_EXECUTABLE Qt5::uic IMPORTED_LOCATION)
set_target_properties(Qt5::uic PROPERTIES IMPORTED_LOCATION_RELEASE ${UIC_EXECUTABLE})

# Restore the actual value
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG ${temp_map_imported})
unset(temp_map_imported)

if (NOT Qt5_FOUND)
  message(FATAL_ERROR "The required Qt version is not found on your computer, "
  "please follow this link to see the instructions on resolving this issue. "
  "https://developer.unigine.com/en/docs/latest/editor2/extensions/environment")
endif()
##==============================================================================
## Target.
##==============================================================================
add_library(${target} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiSamplesPlugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiSamplesPlugin.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiBackend.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiBackend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/SamplesWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SamplesWindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ISample.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleComponents.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleComponents.h
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleEditorImmediate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleEditorImmediate.h
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleSplineEditor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/SampleSplineEditor.h
  )

if(UNIX)
  set_target_properties(${target} PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
  )
endif()

target_include_directories(${target} SYSTEM
  PRIVATE
  ${UNIGINE_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries(${target}
  PRIVATE
  Unigine::Engine
  Unigine::Editor
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  )

##==============================================================================
## Target compiling options and properties.
##==============================================================================
target_compile_definitions(${target}
  PRIVATE
  $<$<BOOL:${UNIX}>:_LINUX>
  QT_NO_DEBUG
  )

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(UNIGINE_COMPILER_IS_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(UNIGINE_COMPILER_IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(UNIGINE_COMPILER_IS_GNU TRUE)
endif()

if (UNIGINE_COMPILER_IS_MSVC)
  target_compile_definitions(${target}
    PRIVATE
    _CRT_SECURE_NO_DEPRECATE
  )
  include(ProcessorCount)
  ProcessorCount(proc_count)
  target_compile_options(${target}
    PRIVATE
    /TP				# Specifies all source files are C++.
    /FS				# Forces writes to the program database (PDB) file to be serialized through MSPDBSRV.EXE.
    /fp:fast		# Specifies floating-point behavior.
    /bigobj			# Increases the number of addressable sections in an .obj file.
    /EHsc			# Specifies the kind of exception handling used by the compiler.
    /MP${proc_count}	# Build with Multiple Processes.
  )
  unset(proc_count)
  target_link_options(${target} INTERFACE "/FIXED:NO")

elseif(UNIGINE_COMPILER_IS_GNU OR UNIGINE_COMPILER_IS_CLANG)

  target_compile_options(${target}
    PRIVATE
    -m64
    -march=athlon64
    -msse4.2
    # Optimizations.
    -ffast-math
    -fno-strict-aliasing
    $<$<NOT:$<CONFIG:Debug>>:-fomit-frame-pointer>
    # Warnings.
    $<$<CONFIG:Debug>:-Wall>
    $<$<CONFIG:Debug>:-Wno-unknown-pragmas>
    $<$<CONFIG:Debug>:-Wno-unused-parameter>
  )
endif ()

set_target_properties(${target}
  PROPERTIES
  OUTPUT_NAME ${binary_name}$<$<CONFIG:Debug>:d>
  LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PLUGIN_DIR}
  LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PLUGIN_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PLUGIN_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PLUGIN_DIR}
  CXX_VISIBILITY_PRESET hidden
  )
