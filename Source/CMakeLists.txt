cmake_minimum_required(VERSION 3.24)
 
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SKIP_RPATH 1)   
project(Uptooda)	

set(IU_ENABLE_MEGANZ ON CACHE BOOL "Enable support of mega.nz")
set(IU_USE_OPENSSL ON CACHE BOOL "Use OpenSSL library for secured connections")
set(IU_ENABLE_SFTP ON CACHE BOOL "Use LibSSH2 for SFTP connections")
set(IU_ENABLE_MEDIAINFO ON CACHE BOOL "Use MediaInfo")

if(WIN32)
	set(IU_ENABLE_WEBVIEW2 ON CACHE BOOL "Enable support of Microsoft Edge WebView2")
    set(IU_ENABLE_SERVERS_CHECKER ON CACHE BOOL "Enable servers checker tool")
    set(IU_ENABLE_NETWORK_DEBUGGER ON CACHE BOOL "Enable network debugger tool")
endif()
set(IU_ENABLE_FFMPEG ON CACHE BOOL "Use FFmpeg libraries")
set(IU_FFMPEG_STANDALONE OFF CACHE BOOL "Don't Use FFmpeg Conan package")
#set(IU_LIBHEIF_WITH_DAV1D ON CACHE BOOL "Build libheif with libdav1d")
set(IU_BUILD_WTL_GUI ON CACHE BOOL "Enable build of WTL GUI")
set(IU_BUILD_QIMAGEUPLOADER OFF CACHE BOOL "Enable build of Qt GUI")
set(IU_STATIC_RUNTIME ON CACHE BOOL "Use static runtime")
set(IU_ENABLE_ASAN OFF CACHE BOOL "Enable address sanitizer")
#set(IU_ARCH "" CACHE STRING "Architecture (x86, x86_64, armv8)")
#set(IU_HOST_PROFILE "" CACHE STRING "Conan host profile")
#set(IU_BUILD_PROFILE "" CACHE STRING "Conan build profile")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

if (MSVC)
	SET(NUGET_PACKAGES_REFERENCES "CrashRpt.CPP_2.0.20;")
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/Uptooda.sln.startup.json")
        configure_file(
            "${CMAKE_SOURCE_DIR}/Uptooda.sln.startup.json"
            "${CMAKE_BINARY_DIR}/Uptooda.sln.startup.json"
            COPYONLY
        )
    endif()
endif()

include(cmake/common.cmake)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

if (MSVC)   
#    add_definitions("/MP")
    add_compile_options(/wd4091) # disable warning C4091: 'typedef ': ignored on left of '*' when no variable is declared
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

if (IU_STATIC_RUNTIME)#AND IU_PROFILE_HOST STREQUAL "" 
	add_definitions(-DIU_STATIC_RUNTIME)
	if(MSVC)
		if(POLICY CMP0091)
			cmake_policy(SET CMP0091 NEW)
 		endif()
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
		
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MT")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MT")
		
		# Replace `/MD` with `/MT` flag to use static VS runtime, 
		# otherwise conan_cmake_autodetect() will fail to detect the runtime.
		# Maybe it will be fixed in future and conan-cmake will respect
		# `CMAKE_MSVC_RUNTIME_LIBRARY` option.
		# https://github.com/conan-io/cmake-conan/issues/174
		set(CompilerFlags
			CMAKE_CXX_FLAGS
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_MINSIZEREL
			CMAKE_CXX_FLAGS_RELWITHDEBINFO 
			CMAKE_C_FLAGS
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_MINSIZEREL
			CMAKE_C_FLAGS_RELWITHDEBINFO 
		)
		foreach(CompilerFlag ${CompilerFlags})
			string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
		endforeach()
	else()
		SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
	endif()
endif()

if (IU_ENABLE_FFMPEG)
	add_definitions(-DIU_ENABLE_FFMPEG)
	if (IU_FFMPEG_STANDALONE)
		add_definitions(-DIU_FFMPEG_STANDALONE)
	endif()
endif()

if(IU_ENABLE_MEDIAINFO)
	add_definitions(-DIU_ENABLE_MEDIAINFO)
endif()

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DGOOGLE_GLOG_DLL_DECL= -DCURL_WIN32_UTF8_FILENAMES -DGDIPVER=0x0100 -D_WTL_NO_CSTRING -D_ATL_NO_OLD_NAMES)
    if (IU_USE_OPENSSL)
        add_definitions(-DUSE_OPENSSL)
    endif()
	if (IU_ENABLE_WEBVIEW2)
        add_definitions(-DIU_ENABLE_WEBVIEW2)
    endif()
    if (IU_ENABLE_SERVERS_CHECKER)
        add_definitions(-DIU_ENABLE_SERVERS_CHECKER)
    endif()
    if (IU_ENABLE_NETWORK_DEBUGGER)
        add_definitions(-DIU_ENABLE_NETWORK_DEBUGGER)
    endif()
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
    #add_compile_options(/bigobj)
    if (IU_ENABLE_ASAN)
        add_definitions(-D_DISABLE_VECTOR_ANNOTATION -D_DISABLE_STRING_ANNOTATION)
        add_compile_options(-fsanitize=address /bigobj)
    endif()
endif(MSVC)

add_definitions(-DTIXML_USE_STL -DSCRAT_USE_EXCEPTIONS -D_SQ64 -DBOOST_BIND_NO_PLACEHOLDERS -DBOOST_ALL_NO_LIB -DNOMINMAX)
#-DBOOST_ASIO_DISABLE_BUFFER_DEBUGGING
SET(ADDITIONAL_OPTIONS "")

if(IU_ENABLE_MEGANZ)
    add_definitions(-DIU_ENABLE_MEGANZ)
endif()


if (IU_ENABLE_WEBVIEW2)
    string(APPEND NUGET_PACKAGES_REFERENCES "Microsoft.Web.WebView2_1.0.2592.51;Microsoft.Windows.ImplementationLibrary_1.0.240122.1;")
endif()

	
if (IU_ENABLE_MEGANZ)
	list(APPEND COMMON_DEPS 
	    megaio/3.5.2@zenden2k/stable 
	    cryptopp/8.7.0  
	    c-ares/1.17.2
		libuv/1.42.0
    )
	if (NOT IU_ENABLE_MEDIAINFO)
		list(APPEND ADDITIONAL_OPTIONS 
			megaio:UseMediainfo=False
		)
	endif()
	#list(APPEND ADDITIONAL_OPTIONS 
	#		megaio:UseCryptopp=False
	#)
	
	#if (WIN32 AND IU_ARCH STREQUAL "armv8")
	#	list(APPEND ADDITIONAL_OPTIONS 
	#		megaio:UseOpenSsl=False
	#		megaio:UseCurl=True
	#		megaio:UseCurlSchannel=True
	#		megaio:UseMediainfo=False
	#	)
	#endif()
endif()

if (IU_ENABLE_SFTP) 
	add_definitions(-DIU_ENABLE_SFTP)					
endif()

	
#conan_cmake_configure(REQUIRES "${COMMON_DEPS}"
#						GENERATORS cmake_find_package_multi 
#						OPTIONS 
#						boost:shared=False
#						boost:without_context=True
#						boost:without_contract=True
# 						boost:without_coroutine=True
# 						boost:without_fiber=True
# 						boost:without_graph=True
# 						boost:without_graph_parallel=True
# 						boost:without_json=True
# 						boost:without_iostreams=True
# 						boost:without_log=True
# 						boost:without_math=True
# 						boost:without_mpi=True
# 						boost:without_nowide=True
# 						boost:without_python=True
# 						boost:without_random=True
# 						boost:without_regex=True
# 						boost:without_serialization=True
# 						boost:without_stacktrace=True
# 						boost:without_test=True
# 						boost:without_timer=True 
# 						boost:without_type_erasure=True
# 						boost:without_wave=True
# #						glog:with_gflags=False
# 						${ADDITIONAL_OPTIONS}
#						)

SET(msvc_link_setting "")

#if (NOT DEFINED CMAKE_CONFIGURATION_TYPES)
#   SET(CONFIGURATION_TYPES "Release")
#else()
#	SET(CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
#endif()


#	conan_cmake_install(PATH_OR_REFERENCE .
#                        BUILD missing                    
#						PROFILE_HOST ${IU_HOST_PROFILE} #"vs2022_arm64"
#						PROFILE_BUILD ${IU_BUILD_PROFILE} #vs2022_arm64
#	)


set(CMAKE_MODULE_PATH ${CONAN_CMAKE_MODULE_PATH} ${CMAKE_MODULE_PATH})

find_package(Boost COMPONENTS program_options REQUIRED)
if (IU_USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
endif()	
find_package(CURL REQUIRED)
find_package(PCRE REQUIRED)
find_package(uriparser REQUIRED)
find_package(ZLIB REQUIRED)
find_package(uriparser REQUIRED)
find_package(base64 REQUIRED)
find_package(jsoncpp REQUIRED)
#find_package(Libssh2 CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(minizip REQUIRED)
find_package(glog REQUIRED)
find_package(tinyxml2 REQUIRED)
if (IU_ENABLE_MEGANZ)
#	find_package(cryptopp CONFIG REQUIRED)
	#find_package(c-ares CONFIG REQUIRED)
	#find_package(libuv CONFIG REQUIRED)
endif()
find_package(WebP REQUIRED)
if (IU_ENABLE_MEDIAINFO)
	find_package(MediaInfoLib REQUIRED) 
endif()	
if(IU_ENABLE_MEGANZ)
	find_package(megaio REQUIRED) 
endif()	
find_package(gumbo-parser REQUIRED) 
find_package(GTest REQUIRED)
find_package(squirrel COMPONENTS sqstdlib REQUIRED) 
if(WIN32)
	find_package(base-classes REQUIRED)
    find_package(libheif REQUIRED)
endif()	

if (IU_ENABLE_FFMPEG AND NOT IU_FFMPEG_STANDALONE)
	find_package(ffmpeg REQUIRED)
endif()
  
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include)
include_directories(${CMAKE_SOURCE_DIR}/)
include_directories(${PROJECT_BINARY_DIR}/include)

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/Libs/)
	include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/win32/)
	include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include/WTL/)
endif()

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
 
if(WIN32)
	include_directories(${CMAKE_SOURCE_DIR}/../Contrib/Include)
    link_directories(${CMAKE_SOURCE_DIR}/../Contrib/Lib/)
else()
    include_directories(${CONAN_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
endif()

set(COMMON_LIBS_LIST "")

#if (IU_ENABLE_SFTP)
#	list(APPEND COMMON_LIBS_LIST Libssh2::Libssh2)
#endif()

#if (IU_ENABLE_MEGANZ)
#	list(APPEND COMMON_LIBS_LIST libuv::libuv)
#endif()
    
#if (IU_USE_OPENSSL)
#	list(APPEND COMMON_LIBS_LIST OpenSSL::OpenSSL)
#endif()

if(WIN32)
	list(APPEND COMMON_LIBS_LIST Oleacc Userenv Psapi Iphlpapi Winhttp Crypt32 Wldap32 Urlmon)
	
#	if (IU_ENABLE_WEBVIEW2)

		#list(APPEND COMMON_LIBS_LIST WebView2Guid.lib)
#		list(APPEND COMMON_LIBS_LIST WebView2LoaderStatic.lib version.lib)
#	endif()	
else()
    list(APPEND COMMON_LIBS_LIST dl pthread rt gflags)
endif()

add_subdirectory(Core)	
if(IU_BUILD_WTL_GUI OR IU_BUILD_QIMAGEUPLOADER)
    add_subdirectory(Video)
endif()
add_subdirectory(History)	

if(WIN32 AND IU_BUILD_WTL_GUI)
    add_subdirectory(Gui)
    add_subdirectory(ScreenCapture)
endif()

add_subdirectory(CLI)

if(IU_BUILD_QIMAGEUPLOADER)
	add_subdirectory(qimageuploader)	
endif()	
#enable_testing() 
add_subdirectory(Tests)	
add_subdirectory(AndroidApp)
