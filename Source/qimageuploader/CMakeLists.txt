cmake_minimum_required(VERSION 4.0)

project(qimageuploader)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Widgets Qml Quick QuickControls2 QuickWidgets)
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
        Gui/MainWindowNew.cpp
        Gui/FrameGrabberController.cpp
        Gui/FrameGrabberController.h
        Gui/ImageViewerWindow.cpp
        Gui/ImageViewerWindow.h
        Gui/IImageViewerSource.h
        Gui/FrameGrabberDlg.cpp
        Gui/ResultsWindow.cpp
        Gui/LogWindow.cpp
        Gui/LoginDialog.cpp
        Gui/AboutDialog.cpp
        ../ScreenCapture/ScreenCaptureQt.cpp
        Gui/RegionSelect.cpp
        Gui/models/uploadtreemodel.cpp
        Gui/models/QmlUploadSessionModel.cpp
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

qt_add_executable(qimageuploader MANUAL_FINALIZATION ${OPTIONS} ${SOURCE_FILES} ${RESOURCE_LIST})

set_source_files_properties(Gui/qml/MainWindowNew.qml PROPERTIES QT_RESOURCE_ALIAS MainWindowNew.qml)
set_source_files_properties(Gui/qml/ServerSelector.qml PROPERTIES QT_RESOURCE_ALIAS ServerSelector.qml)
set_source_files_properties(Gui/qml/ActionButton.qml PROPERTIES QT_RESOURCE_ALIAS ActionButton.qml)
set_source_files_properties(Gui/qml/AppMenuItem.qml PROPERTIES QT_RESOURCE_ALIAS AppMenuItem.qml)
set_source_files_properties(Gui/qml/AppMenuSeparator.qml PROPERTIES QT_RESOURCE_ALIAS AppMenuSeparator.qml)
set_source_files_properties(Gui/qml/MenuBackground.qml PROPERTIES QT_RESOURCE_ALIAS MenuBackground.qml)
set_source_files_properties(Gui/qml/EmptyTab.qml PROPERTIES QT_RESOURCE_ALIAS EmptyTab.qml)
set_source_files_properties(Gui/qml/UploadSessionCard.qml PROPERTIES QT_RESOURCE_ALIAS UploadSessionCard.qml)
set_source_files_properties(Gui/qml/UploadTaskCard.qml PROPERTIES QT_RESOURCE_ALIAS UploadTaskCard.qml)
set_source_files_properties(Gui/qml/ImageViewerWindow.qml PROPERTIES QT_RESOURCE_ALIAS ImageViewerWindow.qml)
set_source_files_properties(Gui/qml/FrameGrabberDialog.qml PROPERTIES QT_RESOURCE_ALIAS FrameGrabberDialog.qml)
set_source_files_properties(Gui/qml/ThumbsView.qml PROPERTIES QT_RESOURCE_ALIAS ThumbsView.qml)
set_source_files_properties(Gui/qml/UploadParametersPage.qml PROPERTIES QT_RESOURCE_ALIAS UploadParametersPage.qml)

qt_add_qml_module(qimageuploader
        URI Uptooda.Ui
        VERSION 1.0
        RESOURCE_PREFIX /qt/qml
        QML_FILES
        Gui/qml/MainWindowNew.qml
        Gui/qml/ServerSelector.qml
        Gui/qml/ActionButton.qml
        Gui/qml/AppMenuItem.qml
        Gui/qml/AppMenuSeparator.qml
        Gui/qml/MenuBackground.qml
        Gui/qml/EmptyTab.qml
        Gui/qml/UploadSessionCard.qml
        Gui/qml/UploadTaskCard.qml
        Gui/qml/ImageViewerWindow.qml
        Gui/qml/FrameGrabberDialog.qml
        Gui/qml/ThumbsView.qml
        Gui/qml/UploadParametersPage.qml
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::QuickWidgets)
qt_import_qml_plugins(${PROJECT_NAME})

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

qt_finalize_executable(${PROJECT_NAME})
