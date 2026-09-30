cmake_minimum_required(VERSION 3.21)

# The version declared here is the single authoritative application version.
# It is compiled into the binary (About dialog, --version) via plainrun_config.h.
project(plainrun
    VERSION 0.1.0
    DESCRIPTION "A simple native running log for Linux"
    HOMEPAGE_URL "https://github.com/pingskills/plainrun"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

option(PLAINRUN_BUILD_TESTS "Build the automated test suite" ON)
option(PLAINRUN_BUILD_DEVTOOLS "Build development-only tools (never installed)" OFF)

set(PLAINRUN_APP_ID "io.github.pingskills.plainrun")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)
qt_standard_project_setup(REQUIRES 6.5)
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW) # qmldir per QML subdirectory
endif()

configure_file(src/plainrun_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/plainrun_config.h @ONLY)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# plainrun_core: all non-visual logic (Qt Core + Qt SQL only). Tested directly.
# ---------------------------------------------------------------------------
qt_add_library(plainrun_core STATIC
    src/core/run.h
    src/core/runformat.h src/core/runformat.cpp
    src/core/validation.h src/core/validation.cpp
    src/core/periods.h src/core/periods.cpp
    src/core/statistics.h src/core/statistics.cpp
    src/core/personalbests.h src/core/personalbests.cpp
    src/core/csv.h src/core/csv.cpp
    src/core/logging.h src/core/logging.cpp
    src/database/database.h src/database/database.cpp
    src/database/migrations.h src/database/migrations.cpp
    src/database/runrepository.h src/database/runrepository.cpp
    src/database/backup.h src/database/backup.cpp
    src/models/runlistmodel.h src/models/runlistmodel.cpp
    src/services/datalocation.h src/services/datalocation.cpp
    src/services/appcontroller.h src/services/appcontroller.cpp
    src/services/cli.h src/services/cli.cpp
    src/omarchy/omarchycolors.h src/omarchy/omarchycolors.cpp
)
target_include_directories(plainrun_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(plainrun_core PUBLIC Qt6::Core Qt6::Sql)

# ---------------------------------------------------------------------------
# plainrun_ui: the QML module and GUI-only C++ (theme). Static QML module so
# that the UI smoke test can load exactly the same QML as the application.
# ---------------------------------------------------------------------------
qt_add_library(plainrun_ui STATIC
    src/ui/theme.h src/ui/theme.cpp
    src/ui/uisetup.h src/ui/uisetup.cpp
)
qt_add_qml_module(plainrun_ui
    URI PlainRun
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/PlainRun
    QML_FILES
        qml/Main.qml
        qml/components/AccentButton.qml
        qml/components/BarChart.qml
        qml/components/ConfirmDialog.qml
        qml/components/HeaderCell.qml
        qml/components/MessageDialog.qml
        qml/components/PeriodSelector.qml
        qml/components/RunRow.qml
        qml/components/SectionLabel.qml
        qml/components/SummaryCell.qml
        qml/views/AboutDialog.qml
        qml/views/HistoryView.qml
        qml/views/RunEditor.qml
        qml/views/SidePanel.qml
        qml/views/StartupErrorView.qml
        qml/views/SummaryBar.qml
    RESOURCES
        resources/icons/png/256/io.github.pingskills.plainrun.png
)
target_link_libraries(plainrun_ui PUBLIC
    plainrun_core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

# ---------------------------------------------------------------------------
# The application executable
# ---------------------------------------------------------------------------
qt_add_executable(plainrun src/app/main.cpp)
target_link_libraries(plainrun PRIVATE plainrun_core plainrun_ui plainrun_uiplugin)
set_target_properties(plainrun PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)

# ---------------------------------------------------------------------------
# Installation (respects CMAKE_INSTALL_PREFIX; no paths are hardcoded in code)
# ---------------------------------------------------------------------------
install(TARGETS plainrun RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/${PLAINRUN_APP_ID}.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/${PLAINRUN_APP_ID}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES resources/icons/${PLAINRUN_APP_ID}.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
foreach(size 16 32 48 64 128 256)
    install(FILES resources/icons/png/${size}/${PLAINRUN_APP_ID}.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps)
endforeach()
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME})

# ---------------------------------------------------------------------------
# Tests and development tools
# ---------------------------------------------------------------------------
if(PLAINRUN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PLAINRUN_BUILD_DEVTOOLS)
    qt_add_executable(plainrun-devseed tools/devseed.cpp)
    target_link_libraries(plainrun-devseed PRIVATE plainrun_core)
    qt_add_executable(plainrun-uishot tools/uishot.cpp)
    target_link_libraries(plainrun-uishot PRIVATE plainrun_core plainrun_ui plainrun_uiplugin)
endif()
