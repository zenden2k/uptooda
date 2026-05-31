
set(SRC_LIST Network/NetworkClient.cpp
	Network/NetworkClientFactory.cpp
	Network/NetworkDebugger.cpp
    Upload/DefaultUploadEngine.cpp
    Upload/FileUploadTask.cpp
    Upload/ScriptUploadEngine.cpp
    Upload/UploadEngine.cpp
    Upload/Uploader.cpp
    Upload/UploadTask.cpp
    Upload/AuthTask.cpp
    Upload/UrlShorteningTask.cpp
    Upload/TestConnectionTask.cpp
    Upload/FolderTask.cpp
    Upload/SearchByImageUrlTask.cpp
    Upload/SearchByImageFileTask.cpp
    Upload/TempImageUploadTask.cpp
    Utils/CoreUtils.cpp
    Utils/CryptoUtils.cpp
    Utils/SimpleXml.cpp
    Utils/StringUtils.cpp
    Utils/ConsoleUtils.cpp
    Utils/MimeTypeHelper.cpp
    3rdpart/pcreplusplus.cpp
    OutputGenerator/OutputGeneratorFactory.cpp
    OutputGenerator/AbstractOutputGenerator.cpp
    OutputGenerator/AbstractCodeGenerator.cpp
    OutputGenerator/BBCodeGenerator.cpp
    OutputGenerator/HTMLGenerator.cpp
    OutputGenerator/MarkdownGenerator.cpp
    OutputGenerator/PlainTextGenerator.cpp
    OutputGenerator/JSONGenerator.cpp
    OutputGenerator/XmlTemplateList.cpp
    OutputGenerator/XmlTemplateGenerator.cpp
    UploadEngineList.cpp
    SettingsManager.cpp
        AppRuntimeInfo.cpp
    Scripting/API/Functions.cpp
    Scripting/API/RegularExpression.cpp
    Scripting/API/ScriptAPI.cpp
    Scripting/API/WebServer.cpp
    Logging.cpp
	DownloadTask.cpp
    Upload/ConsoleUploadErrorHandler.cpp
    Upload/FileQueueUploader.cpp
    Upload/FileQueueUploaderPrivate.cpp
    Upload/FolderList.cpp
    Upload/ServerProfile.cpp
    Upload/ServerProfileGroup.cpp
    Upload/ServerSync.cpp
    Upload/UploadEngineManager.cpp
    Upload/UploadManager.cpp
    Upload/UploadSession.cpp
    Upload/AdvancedUploadEngine.cpp
    Upload/Parameters/AbstractParameter.cpp
    Upload/Parameters/TextParameter.cpp
    Upload/Parameters/ChoiceParameter.cpp
    Upload/Parameters/BooleanParameter.cpp
    Upload/Parameters/FileNameParameter.cpp
    Upload/Parameters/ParameterFactory.cpp
    ServiceLocator.cpp
    Settings/BasicSettings.cpp
    Logging/MyLogSink.cpp
    Logging/ConsoleLogger.cpp
    Scripting/ScriptsManager.cpp
    Network/CurlShare.cpp
    ThreadSync.cpp
    Scripting/Script.cpp
    Scripting/API/UploadTaskWrappers.cpp
    Scripting/API/GumboBingings/GumboDocument.cpp
    TempFileDeleter.cpp
    Utils/DesktopUtils.cpp
    HistoryManager.cpp
    CoreFunctions.cpp
    3rdpart/GumboQuery/GQDocument.cpp
    3rdpart/GumboQuery/Node.cpp
    3rdpart/GumboQuery/Object.cpp
    3rdpart/GumboQuery/GQ_Parser.cpp
    3rdpart/GumboQuery/QueryUtil.cpp
    3rdpart/GumboQuery/Selection.cpp
    3rdpart/GumboQuery/Selector.cpp
    3rdpart/xdgmime/xdgmime.c
    3rdpart/xdgmime/xdgmimeglob.c
    3rdpart/xdgmime/xdgmimeint.c
    3rdpart/xdgmime/xdgmimemagic.c
    3rdpart/xdgmime/xdgmimealias.c
    3rdpart/xdgmime/xdgmimeparent.c
    3rdpart/xdgmime/xdgmimecache.c
    3rdpart/xdgmime/xdgmimeicon.c
    Scripting/API/Process.cpp
    Upload/Filters/UrlShorteningFilter.cpp
    Upload/Filters/ImageSearchFilter.cpp
    Upload/Filters/UserFilter.cpp
    LocalFileCache.cpp
    Utils/SystemUtils.cpp
    Settings/EncodedPassword.cpp
    Scripting/API/WebBrowserPrivateBase.cpp
    Utils/TextUtils.cpp
    Scripting/UploadFilterScript.cpp
    3rdpart/htmlentities.cpp
    Video/VideoUtils.cpp
    Images/AbstractImage.cpp
    FileTypeCheckTask.cpp
    Settings/AutoSettings.cpp
)

set(HEADER_LIST
    BasicConstants.h
    Network/NetworkClient.h
	Network/NetworkClientFactory.h
    Network/NetworkDebugger.h
    Upload/DefaultUploadEngine.h
    Upload/FileUploadTask.h
    Upload/ScriptUploadEngine.h
    Upload/UploadEngine.h
    Upload/FolderItem.h
    Upload/Uploader.h
    Upload/UploadTask.h
    Upload/AuthTask.h
    Upload/TestConnectionTask.h
    Upload/UrlShorteningTask.h
    Upload/FolderTask.h
    Upload/SearchByImageUrlTask.h
    Upload/SearchByImageFileTask.h
    Upload/TempImageUploadTask.h
    Upload/Parameters/AbstractParameter.h
    Upload/Parameters/TextParameter.h
    Upload/Parameters/ChoiceParameter.h
    Upload/Parameters/BooleanParameter.h
    Upload/Parameters/FileNameParameter.h
    Upload/Parameters/ParameterFactory.h
    Utils/CoreUtils.h
    Utils/CryptoUtils.h
    Utils/SimpleXml.h
    Utils/StringUtils.h
    Utils/ConsoleUtils.h
    Utils/IOException.h
    Utils/MimeTypeHelper.h
    3rdpart/pcreplusplus.h
    3rdpart/utf8.h
    3rdpart/UriParser.h
    3rdpart/xdgmime/xdgmime.h
    3rdpart/xdgmime/xdgmimealias.h
    3rdpart/xdgmime/xdgmimecache.h
    3rdpart/xdgmime/xdgmimeglob.h
    3rdpart/xdgmime/xdgmimeicon.h
    3rdpart/xdgmime/xdgmimeint.h
    3rdpart/xdgmime/xdgmimemagic.h
    3rdpart/xdgmime/xdgmimeparent.h 
    OutputGenerator/OutputGeneratorFactory.h
    OutputGenerator/AbstractOutputGenerator.h
    OutputGenerator/AbstractCodeGenerator.h
    OutputGenerator/BBCodeGenerator.h
    OutputGenerator/HTMLGenerator.h
    OutputGenerator/MarkdownGenerator.h
    OutputGenerator/PlainTextGenerator.h
    OutputGenerator/JSONGenerator.h
    OutputGenerator/XmlTemplateList.h
    OutputGenerator/XmlTemplateGenerator.h
    UploadEngineList.h
    SettingsManager.h
        AppRuntimeInfo.h
    Scripting/API/Functions.h
    Scripting/API/RegularExpression.h
    Scripting/API/ScriptAPI.h
    Scripting/API/WebServer.h
    Logging.h
    Upload/ConsoleUploadErrorHandler.h
    Upload/FileQueueUploader.h
    Upload/FileQueueUploaderPrivate.h
    Upload/FolderList.h
    Upload/ServerProfile.h
    Upload/ServerProfileGroup.h
    Upload/ServerSync.h
    Upload/UploadEngineManager.h
    Upload/UploadManager.h
    Upload/UploadSession.h
    Upload/AdvancedUploadEngine.h
    ServiceLocator.h
    Settings/BasicSettings.h
    Logging/MyLogSink.h
    Logging/ConsoleLogger.h
    Scripting/ScriptsManager.h
    Network/CurlShare.h
    ThreadSync.h
    Scripting/Script.h
    Scripting/API/UploadTaskWrappers.h
    Scripting/API/GumboBingings/GumboDocument.h
    TempFileDeleter.h
    Utils/DesktopUtils.h
    HistoryManager.h
    CoreFunctions.h
    3rdpart/GumboQuery/Document.h
    3rdpart/GumboQuery/Node.h
    3rdpart/GumboQuery/Object.h
    3rdpart/GumboQuery/Parser.h
    3rdpart/GumboQuery/QueryUtil.h
    3rdpart/GumboQuery/Selection.h
    3rdpart/GumboQuery/Selector.h
    Scripting/API/Process.h
    Upload/Filters/UrlShorteningFilter.h
    Upload/Filters/ImageSearchFilter.h
    Upload/Filters/UserFilter.h
    LocalFileCache.h
    Utils/SystemUtils.h
    Settings/EncodedPassword.h
    Scripting/API/WebBrowserPrivateBase.h
    Utils/GlobalMutex.h
    Utils/TextUtils.h
    Scripting/UploadFilterScript.h
    3rdpart/htmlentities.h
    Video/VideoUtils.h
    i18n/Translator.h
    FileTypeCheckTask.h
    IFileList.h
    Settings/AutoSettings.h
)
    
if(WIN32)
    list(APPEND SRC_LIST 
        Utils/Utils_win.cpp
        Utils/CryptoUtils_win.cpp
        Utils/SystemUtils_win.cpp
        Utils/IOUtf8_win.cpp
        Utils/GlobalMutex_win.cpp
        DefaultProxyProvider.cpp
        3rdpart/CodePages.cpp 
        Scripting/API/HtmlDocument.cpp
        Scripting/API/COMUtils.cpp
        Scripting/API/HtmlDocumentPrivate_win.cpp
        Scripting/API/HtmlElement.cpp
        Scripting/API/HtmlElementPrivate_win.cpp
        Scripting/API/WebBrowser.cpp
        Scripting/API/WebBrowserPrivate_win.cpp
        ../Func/UpdatePackage.cpp
        ../Func/IuCommonFunctions.cpp
        ../3rdpart/Registry.cpp
        ../3rdpart/Unzipper.cpp
        ../3rdpart/MemberFunctionCallback.cpp
        3rdpart/xdgmime/ports/fnmatch.cpp
        ../Gui/Dialogs/WebViewWindow.cpp
        ../Gui/Controls/WTLBrowserView.cpp
        ../Func/WinUtils.cpp
        ../Gui/GuiTools.cpp
        ../Gui/Helpers/DPIHelper.cpp
        Images/Utils.cpp
        Images/GdiplusImageReader.cpp
        Images/WebpImageReader.cpp
        Images/HeifImageReader.cpp
        Images/ImageLoader.cpp
        Images/GdiPlusImage.cpp
        Images/ColorQuantizer.cpp
        Images/OctreeColorQuantizer.cpp
        ../Func/GdiPlusInitializer.cpp
    )
    
    if (IU_ENABLE_WEBVIEW2)
        list(APPEND SRC_LIST  ../Gui/Controls/Webview2BrowserView.cpp)
    endif()

    list(APPEND HEADER_LIST
        COMInitializer.h
        DefaultProxyProvider.h 
        3rdpart/CodePages.h
        3rdpart/xdgmime/ports/fnmatch.h
        Scripting/API/HtmlDocument.h
        Scripting/API/COMUtils.h
        Scripting/API/HtmlDocumentPrivate_win.h
        Scripting/API/HtmlElement.h
        Scripting/API/HtmlElementPrivate_win.h
        Scripting/API/WebBrowser.h
        Scripting/API/WebBrowserPrivate_win.h
        ../Func/UpdatePackage.h
        ../Func/IuCommonFunctions.h
        ../3rdpart/Registry.h
        ../3rdpart/Unzipper.h
        ../3rdpart/MemberFunctionCallback.h
        ../Gui/Dialogs/WebViewWindow.h
        ../Gui/Controls/WTLBrowserView.h
        ../Func/WinUtils.h
        ../Gui/GuiTools.h
        ../Gui/Helpers/DPIHelper.cpp
        Images/Utils.h
        Images/GdiplusImageReader.h
        Images/WebpImageReader.h
        Images/HeifImageReader.h
        Images/AbstractImageReader.h
        Images/ImageLoader.h
        Images/ColorQuantizer.h
        Images/OctreeColorQuantizer.h
        Utils/IOUtf8_win.h
        Utils/Win32Exception.h
    )
else()
    list(APPEND SRC_LIST  
        Utils/Utils_unix.cpp 
        Utils/SystemUtils_unix.cpp
        Utils/CryptoUtils_unix.cpp
        Utils/GlobalMutex_unix.cpp
    )
endif()

if(IU_ENABLE_MEGANZ)
    list(APPEND SRC_LIST Upload/MegaNzUploadEngine.cpp)
    list(APPEND HEADER_LIST Upload/MegaNzUploadEngine.h)
endif()

if(WIN32)   
	include_directories(${CMAKE_SOURCE_DIR}/Core/3rdpart/xdgmime/ports)
endif()

source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "Sources" FILES ${SRC_LIST} ${HEADER_LIST})

add_library(iu_core STATIC ${SRC_LIST} ${HEADER_LIST})

target_link_libraries(iu_core PUBLIC
    CURL::libcurl
    boost::boost 
    uriparser::uriparser 
    ZLIB::ZLIB 
    JsonCpp::JsonCpp
    glog::glog 
    pcre::pcre
    squirrel::squirrel 
    squirrel::sqstdlib 
    gumbo-parser::gumbo-parser 
    aklomp::base64
    tinyxml2::tinyxml2
    ${COMMON_LIBS_LIST}
)

if (IU_ENABLE_MEGANZ)
    target_link_libraries(iu_core PUBLIC megaio::megaio
    SQLite::SQLite3
    #cryptopp::cryptopp-static 
    #c-ares::c-ares
    )
endif()

if (IU_USE_OPENSSL)
    target_link_libraries(iu_core PUBLIC openssl::openssl)
endif()

if (IU_ENABLE_SFTP)
    #target_link_libraries(iu_core PUBLIC Libssh2::Libssh2)
	#list(APPEND COMMON_LIBS_LIST )
endif()

if (WIN32)
    target_link_libraries(iu_core PUBLIC minizip::minizip WebP::webpdemux WebP::webpdecoder libheif::heif gdiplus winmm urlmon Dwmapi)
	configure_webview2_target(iu_core)
	configure_nuget_packages(iu_core)
endif()



#if(MSVC)
#    target_compile_options(iu_core PRIVATE "/MP")
#endif()

set_target_properties(iu_core PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/)
