cmake_minimum_required(VERSION 3.16)
project(app-qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(app-qt WIN32
    src/main.cpp

    # Bundled resources (game banner artwork)
    resources/games.qrc

    # Application shell
    src/app/MainWindow.cpp          src/app/MainWindow.h
    src/app/SideBar.cpp             src/app/SideBar.h

    # Pages (stacked widget)
    src/pages/MinecraftPage.cpp     src/pages/MinecraftPage.h
    src/pages/CS2Page.cpp           src/pages/CS2Page.h
    src/pages/GenericGamePage.cpp   src/pages/GenericGamePage.h
    src/pages/DockerSettingsPage.cpp src/pages/DockerSettingsPage.h
    src/pages/VMAdminPage.cpp       src/pages/VMAdminPage.h
    src/pages/InfraPage.cpp         src/pages/InfraPage.h

    # Reusable widgets
    src/widgets/ServerDashboard.cpp src/widgets/ServerDashboard.h
    src/widgets/PluginManagerWidget.cpp src/widgets/PluginManagerWidget.h
    src/widgets/GameConsoleWidget.cpp src/widgets/GameConsoleWidget.h
    src/widgets/GameLogsWidget.cpp  src/widgets/GameLogsWidget.h
    src/widgets/ConfigEditorWidget.cpp src/widgets/ConfigEditorWidget.h
    src/widgets/DeploymentTargetSelector.cpp src/widgets/DeploymentTargetSelector.h
    src/widgets/GameBanner.h

    # Dialogs
    src/dialogs/ImagePickerDialog.cpp src/dialogs/ImagePickerDialog.h
    src/dialogs/AddCustomVMDialog.cpp src/dialogs/AddCustomVMDialog.h
    src/dialogs/CreateVMDialog.cpp   src/dialogs/CreateVMDialog.h

    # Docker / remote execution
    src/docker/DockerManager.cpp    src/docker/DockerManager.h
    src/docker/SshTunnel.cpp        src/docker/SshTunnel.h

    # Cloud providers
    src/cloud/CloudProviderBase.h
    src/cloud/ScalewayProvider.cpp  src/cloud/ScalewayProvider.h
    src/cloud/VMStorage.cpp         src/cloud/VMStorage.h

    # Models / config (header-only)
    src/models/GamePageConfig.h
    src/models/ServerInstance.h
    src/models/VMInstance.h
)

# Quoted includes are by filename only, so expose every source dir on the
# include path. This keeps existing `#include "Foo.h"` working after the move.
target_include_directories(app-qt PRIVATE
    src
    src/app
    src/pages
    src/widgets
    src/dialogs
    src/docker
    src/cloud
    src/models
)

target_link_libraries(app-qt PRIVATE Qt6::Widgets Qt6::Network)
