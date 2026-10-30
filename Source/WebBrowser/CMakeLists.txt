set(SRC_LIST
        WebViewWindow.cpp
        WebViewWindow.h
        WTLBrowserView.cpp
        WTLBrowserView.h
        Browser.h
)

if (IU_ENABLE_WEBVIEW2)
    list(APPEND SRC_LIST
            Webview2BrowserView.cpp
            Webview2BrowserView.h
    )
endif()


source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "Sources" FILES ${SRC_LIST})

add_library(iu_webbrowser STATIC ${SRC_LIST})

target_link_libraries(iu_webbrowser PUBLIC
        glog::glog
)

configure_webview2_target(iu_webbrowser)
configure_nuget_packages(iu_webbrowser)

if(MSVC)
    target_compile_options(iu_webbrowser PRIVATE /permissive)
endif()

set_target_properties(iu_webbrowser PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/)
