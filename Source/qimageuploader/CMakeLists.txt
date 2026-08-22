cmake_minimum_required(VERSION 4.0)

project(qimageuploader)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Widgets)
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
        Gui/FileThumbnailCache.cpp
        Gui/FileThumbnailCache.h
        Gui/AppStyle.cpp
        Gui/AppStyle.h
        Gui/styles/Application.qss
        Gui/MediaDlg.cpp
        Gui/MediaDlg.h
        Gui/QtImageGenerator.cpp
        Gui/QtImageGenerator.h
        Gui/ImageViewerWindow.cpp
        Gui/ImageViewerWindow.h
        Gui/VirtualFileDrop.h
        Gui/IImageViewerSource.h
        Gui/ResultsWindow.cpp
        Gui/LogWindow.cpp
        Gui/LoginDialog.cpp
        Gui/AboutDialog.cpp
        ../ScreenCapture/ScreenCaptureQt.cpp
        Gui/RegionSelect.cpp
        Gui/models/ThumbnailListModel.cpp
        Gui/models/ThumbnailListModel.h
        Gui/controls/ThumbnailListView.cpp
        Gui/controls/ThumbnailListView.h
        QtUploadErrorHandler.cpp
        QtDefaultLogger.cpp
        QtScriptDialogProvider.cpp
        QtScriptFunctionsImpl.cpp
        Gui/controls/ServerSelectorWidget.cpp
        Gui/controls/ProgressRingWidget.cpp
        Gui/controls/MainWindowTabsWidget.cpp
        Gui/controls/MainWindowTabsWidget.h
        Gui/controls/UploadSessionListWidget.cpp
        Gui/controls/UploadSessionListWidget.h
        ../Core/Settings/QtGuiSettings.cpp
        ../Core/Settings/CommonGuiSettings.cpp
        ../Core/AbstractServerIconCache.cpp
        ../Core/AbstractServerIconCache.h
        ../Core/QtServerIconCache.cpp
        ../Core/QtServerIconCache.h
        ../Video/QtImage.cpp
        Gui/ui/MainWindow.ui
        Gui/ui/MediaDlg.ui
        Gui/ui/ResultsWindow.ui
        Gui/ui/LogWindow.ui
        Gui/ui/LoginDialog.ui
        Gui/ui/AboutDialog.ui
        qimageuploader.qrc
)

if(WIN32)
    list(APPEND SOURCE_FILES
            ../Func/GdiPlusInitializer.cpp
            Gui/WindowsVirtualFileDrop.cpp
    )
else()
    list(APPEND SOURCE_FILES Gui/NullVirtualFileDrop.cpp)
endif()

add_executable(qimageuploader ${OPTIONS} ${SOURCE_FILES} ${RESOURCE_LIST})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gdi32 dwmapi gdiplus ole32)
    configure_webview2_target(${PROJECT_NAME})
    configure_nuget_packages(${PROJECT_NAME})
endif()

target_link_libraries(qimageuploader PRIVATE iu_core iu_video ${COMMON_LIBS_LIST})

if(WIN32 AND IU_ENABLE_MEDIAINFO)
    target_link_libraries(qimageuploader PRIVATE iu_mediainfo)
endif()

if(WIN32)
    add_custom_command(TARGET qimageuploader
            POST_BUILD
            COMMAND python "${CMAKE_SOURCE_DIR}/../Utils/Version/set_binary_version.py"
            "${CMAKE_SOURCE_DIR}/versioninfo.h"
            $<TARGET_FILE:qimageuploader>
            $<CONFIG>
    )
endif()
