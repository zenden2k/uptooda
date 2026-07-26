set(SRC_LIST    
    VideoGrabber.cpp
        AbstractFrameGrabber.cpp
    AbstractVideoFrame.cpp
)

set(HEADER_LIST 
    VideoGrabber.h
    FrameGrabberException.h
    AbstractFrameGrabber.h
    AbstractVideoFrame.h
)
    
if(WIN32)
    SET(OPTIONS WIN32)
    list(APPEND SRC_LIST 
        DirectshowFrameGrabber.cpp
        DirectshowFrameGrabber2.cpp
        DirectshowVideoFrame.cpp
        NoDirectVobSub.cpp
        GrabFilter.cpp
        DirectShowUtil.cpp
        ../Core/3rdpart/dxerr.cpp
        MediaFoundationFrameGrabber.cpp
    )
    list(APPEND HEADER_LIST 
        DirectshowFrameGrabber.h
        DirectshowFrameGrabber2.h
        DirectshowVideoFrame.h
        NoDirectVobSub.h
        GrabFilter.h
        DirectShowUtil.h
        ../Core/3rdpart/dxerr.h
        MediaFoundationFrameGrabber.h
    )
endif()

if (IU_ENABLE_FFMPEG)
	list(APPEND SRC_LIST AvcodecFrameGrabber.cpp)
    list(APPEND HEADER_LIST AvcodecFrameGrabber.h)
endif()	

source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "Sources" FILES ${SRC_LIST} ${HEADER_LIST})

add_library(iu_video STATIC ${SRC_LIST} ${HEADER_LIST})

target_include_directories(iu_video PRIVATE ${CMAKE_SOURCE_DIR}/../Contrib/Include/DX/)

#if (WIN32)
#    set(LIBS_LIST base-classes::base-classes)
#endif()

if (IU_ENABLE_FFMPEG)
    if(NOT IU_FFMPEG_STANDALONE)
        target_link_libraries(iu_video PRIVATE ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil ffmpeg::swscale)
    else()
	    target_link_libraries(iu_video PUBLIC avcodec avformat avutil swscale)
    endif()	
endif()	

if (WIN32)
    target_link_libraries(iu_video PUBLIC base-classes::base-classes Winmm.lib Mfreadwrite.lib Mfplat.lib mfuuid.lib Propsys.lib)
endif()

target_link_libraries(iu_video PUBLIC
    iu_core
)

target_link_libraries(iu_video PRIVATE
    ${COMMON_LIBS_LIST}
)

if(MSVC)
#    target_compile_options(iu_video PRIVATE "/MP")
endif()

set_target_properties(iu_video PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/)
