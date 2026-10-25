
add_definitions(-DSQTEST_STATIC -DIU_NOWEBBROWSER -DIU_TESTS)

set(SRC_LIST 
    Tests.cpp
    TestHelpers.cpp
    sqtest.cpp
   ../Core/Utils/Tests/CoreUtilsTest.cpp
   ../Core/Utils/Tests/CryptoUtilsTest.cpp
   ../Core/Utils/Tests/StringUtilsTest.cpp
   ../Core/Utils/Tests/TextUtilsTest.cpp
   ../Core/Utils/Tests/SimpleXmlTest.cpp
   ../Core/Upload/Tests/UploadEngineListTest.cpp
   ../Core/Upload/Tests/UploadEngineDataTest.cpp
   ../Core/Upload/Tests/ScriptUploadEngineTest.cpp
   ../Core/Upload/Tests/DefaultUploadEngineTest.cpp
   ../Core/3rdpart/GumboQuery/Tests/GumboTest.cpp
   ../Core/DownloadTaskTest.cpp
   ../Core/Network/Tests/NetworkClientTest.cpp
   ../Core/Network/Tests/CurlTest.cpp
   ../CLI/ConsoleScriptFunctionsImpl.cpp
   ../Core/Settings/CliSettings.cpp
   ../Core/Network/Tests/NetworkClientMock.h
   ../Func/IuCommonFunctions.cpp
)
if(WIN32)
    list(APPEND SRC_LIST 
        ../Func/Tests/CmdLineTest.cpp 
        ../Func/CmdLine.cpp
        ../Func/Tests/WinUtilsTest.cpp 
        ../Core/Images/Tests/UtilsTest.cpp
        ../Core/Images/Tests/ImageConverterTest.cpp
        ../Core/Images/Tests/HeifImageReaderTest.cpp
        ../Core/Images/ImageConverterPrivate_gdiplus.cpp
        ../Core/Images/ImageConverterPrivateBase.cpp
        ../Core/Images/ImageConverter.cpp
        ../Core/Images/Thumbnail.cpp
        ../Core/3rdpart/parser.cpp
        ../Func/MyUtils.cpp
        ../Core/FileDownloader.cpp
        ../Core/FileDownloaderTest.cpp
        ../Video/Tests/VideoGrabberTest.cpp
    )
   
endif()

add_executable(Tests ${SRC_LIST})

if (MSVC)
    configure_webview2_target(Tests)
    configure_nuget_packages(Tests)
endif()

set_target_properties(Tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Tests/)
if(WIN32 AND IU_ENABLE_FFMPEG AND IU_FFMPEG_STANDALONE)
   set_target_properties(Tests PROPERTIES LINK_FLAGS "/DELAYLOAD:avcodec-59.dll /DELAYLOAD:avformat-59.dll /DELAYLOAD:avutil-57.dll /DELAYLOAD:swscale-6.dll")
endif()

if(WIN32)
    list(APPEND LIBS_LIST urlmon)
	#if (IU_ENABLE_FFMPEG)
	#	list(APPEND LIBS_LIST avformat avutil swscale avcodec)
	#endif()
 #   target_link_libraries(Tests)
else()
    #list(APPEND LIBS_LIST icuuc icui18n gtest)
endif()

target_link_libraries(Tests iu_core iu_video GTest::gtest GTest::gmock ${COMMON_LIBS_LIST}) 

if (NOT CMAKE_CROSSCOMPILING)
	include(GoogleTest)
    #gtest_discover_tests(Tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Tests/TestData") 
	#gtest_discover_tests(Tests EXTRA_ARGS --dir "${CMAKE_SOURCE_DIR}/Tests/TestData" --sqdir "${CMAKE_SOURCE_DIR}/Core/Scripting/API/Tests/")
endif()
