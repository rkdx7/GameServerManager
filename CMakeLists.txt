cmake_minimum_required(VERSION 3.16)
project(app-qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(app-qt WIN32
    main.cpp
    LoginWindow.cpp         LoginWindow.h
    MainWindow.cpp          MainWindow.h
    SideBar.cpp             SideBar.h
    MinecraftPage.cpp       MinecraftPage.h
    CS2Page.cpp             CS2Page.h
    DockerSettingsPage.cpp  DockerSettingsPage.h
    ServerDashboard.cpp     ServerDashboard.h
    PluginManagerWidget.cpp PluginManagerWidget.h
    DockerManager.cpp       DockerManager.h
    SshTunnel.cpp           SshTunnel.h
    GenericGamePage.cpp     GenericGamePage.h
    ImagePickerDialog.cpp   ImagePickerDialog.h
    GameConsoleWidget.cpp   GameConsoleWidget.h
    GamePageConfig.h
    ServerInstance.h
    VMInstance.h
    VMStorage.cpp           VMStorage.h
    CloudProviderBase.h
    ScalewayProvider.cpp    ScalewayProvider.h
    AddCustomVMDialog.cpp   AddCustomVMDialog.h
    CreateVMDialog.cpp      CreateVMDialog.h
    DeploymentTargetSelector.cpp  DeploymentTargetSelector.h
    VMAdminPage.cpp         VMAdminPage.h
    InfraPage.cpp           InfraPage.h
)

target_link_libraries(app-qt PRIVATE Qt6::Widgets Qt6::Network)
