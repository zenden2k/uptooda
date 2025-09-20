cmake_minimum_required(VERSION 3.1.0)							

#set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/c_flag_overrides.cmake)
#set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cxx_flag_overrides.cmake)

if (CMAKE_VERSION VERSION_LESS "3.1")
#    include(CheckCXXCompilerFlag)
#    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
#    if(COMPILER_SUPPORTS_CXX11)
#        message("Compiler supports C++11.")
#        
#    elseif(COMPILER_SUPPORTS_CXX0X)
#        message("Compiler supports c++0x.")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#    else()
#        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
#    endif()
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    message("Stadart set to 17")
else ()
    set (CMAKE_CXX_STANDARD 17)
endif ()


#use_cxx11()
set(CMAKE_SKIP_RPATH 1)   
project(ImageUploader)	

set(IU_DISABLE_MEGANZ OFF CACHE BOOL "Disable support of mega.nz")
set(IU_USE_OPENSSL OFF CACHE BOOL "Use OpenSSL library for secured connections")

if (MSVC)
    set(IU_STATIC_RUNTIME ON CACHE BOOL "Use static runtime")
    add_definitions("/MP")
    add_compile_options(/wd4091) # disable warning C4091: 'typedef ': ignored on left of '*' when no variable is declared
endif()

if (MSVC AND IU_STATIC_RUNTIME) 
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
endif()

#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.01")
if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DCURL_STATICLIB -DPCRE_STATIC -DGOOGLE_GLOG_DLL_DECL=)
    if (IU_USE_OPENSSL)
        add_definitions(-DUSE_OPENSSL)
    endif()
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_IE=0x0603 -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -D_ATL_XP_TARGETING)
endif(MSVC)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

if (NOT WIN32) 
    find_package(Boost COMPONENTS filesystem system locale thread program_options REQUIRED)
    #find_package(OpenSSL REQUIRED)
    message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
    add_definitions(-DUSE_OPENSSL -DUSE_FREEIMAGE)
endif()

include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include)
include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/squirrel/)
include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/sqplus/)
include_directories(${CMAKE_SOURCE_DIR}/)

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/Libs/)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Source/boost/)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/WTL/)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/gumbo/)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/pcre/)
endif()

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_definitions(-DTIXML_USE_STL -DENABLE_SYNC -DSCRAT_USE_EXCEPTIONS -D_SQ64 -DBOOST_BIND_NO_PLACEHOLDERS)

if(IU_DISABLE_MEGANZ)
    add_definitions(-DIU_DISABLE_MEGANZ)
endif()

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include)
    link_directories(${CMAKE_SOURCE_DIR}/../Contrib/Lib/ ${CMAKE_SOURCE_DIR}/../Contrib/Source/boost/stage/lib/)
else()
    include_directories(${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
endif()

# libiucore depends on these libraries
set(COMMON_LIBS_LIST squirrel sqstdlib pcre gumbo uriparser jsoncpp sqlite3)
if (NOT IU_DISABLE_MEGANZ)
    list(APPEND COMMON_LIBS_LIST mega cryptopp cares)
    if (IU_USE_OPENSSL)
        list(APPEND COMMON_LIBS_LIST mega_openssl)
    endif()
endif()

if(WIN32)
    if (NOT IU_DISABLE_MEGANZ)
        list(APPEND COMMON_LIBS_LIST libuv)
    endif()
    
    if (IU_USE_OPENSSL)
        list(APPEND COMMON_LIBS_LIST ssleay32 libeay32 libcurl_openssl)
	else()
		list(APPEND COMMON_LIBS_LIST ssleay32 libeay32 libcurl)
    endif()

    list(APPEND COMMON_LIBS_LIST Oleacc glog minizip Userenv Psapi Iphlpapi Winhttp base64 libwebpdecoder libwebpdemux zlib Crypt32 Wldap32)
else()
    list(APPEND COMMON_LIBS_LIST dl z base64 curl ssl crypto glog pthread uv_a rt sqlite3 freeimage gflags ${Boost_LIBRARIES})
    SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()
# ${OPENSSL_LIBRARIES}

add_subdirectory(Core)	
add_subdirectory(CLI)	
if(WIN32)
    add_subdirectory(Gui)
    add_subdirectory(ServerListTool)
endif()

#add_subdirectory(qimageuploader)	
#add_subdirectory(Tests)	
