cmake_minimum_required(VERSION 4.0)

project(qimageuploader)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS "Gui/ui/")

add_definitions(-DIU_QT)

if (WIN32)
    set(RESOURCE_LIST ../res/qimageuploader.exe.manifest qimageuploader.rc)
    set(OPTIONS WIN32)
endif()

set(SOURCE_FILES
        main.cpp
        Gui/MainWindow.cpp
        Gui/FrameGrabberDlg.cpp
        Gui/ResultsWindow.cpp
        Gui/LogWindow.cpp
        Gui/LoginDialog.cpp
        Gui/AboutDialog.cpp
        ../ScreenCapture/ScreenCaptureQt.cpp
        Gui/RegionSelect.cpp
        Gui/models/uploadtreemodel.cpp
        Gui/controls/MyListWidget.cpp
        QtUploadErrorHandler.cpp
        QtDefaultLogger.cpp
        QtScriptDialogProvider.cpp
        QtScriptFunctionsImpl.cpp
        Gui/controls/ServerSelectorWidget.cpp
        Gui/controls/ProgressRingWidget.cpp
        Gui/controls/UploadTreeView.cpp
        Gui/controls/UploadTreeView.h
        ../Core/Settings/QtGuiSettings.cpp
        ../Core/Settings/CommonGuiSettings.cpp
        ../Core/AbstractServerIconCache.cpp
        ../Core/AbstractServerIconCache.h
        ../Core/QtServerIconCache.cpp
        ../Core/QtServerIconCache.h
        ../Video/QtImage.cpp
        Gui/ui/MainWindow.ui
        Gui/ui/FrameGrabberDlg.ui
        Gui/ui/ResultsWindow.ui
        Gui/ui/LogWindow.ui
        Gui/ui/LoginDialog.ui
        Gui/ui/AboutDialog.ui
        qimageuploader.qrc
)

if(WIN32)
    list(APPEND SOURCE_FILES
            ../Func/GdiPlusInitializer.cpp
    )
endif()

add_executable(qimageuploader ${OPTIONS} ${SOURCE_FILES} ${RESOURCE_LIST})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32 dwmapi gdiplus)
    configure_webview2_target(${PROJECT_NAME})
    configure_nuget_packages(${PROJECT_NAME})
endif()

target_link_libraries(qimageuploader PRIVATE iu_core iu_video iu_image ${COMMON_LIBS_LIST})

if(WIN32)
    add_custom_command(TARGET qimageuploader
            POST_BUILD
            COMMAND python "${CMAKE_SOURCE_DIR}/../Utils/Version/set_binary_version.py"
            "${CMAKE_SOURCE_DIR}/versioninfo.h"
            $<TARGET_FILE:qimageuploader>
            $<CONFIG>
    )
endif()