add_library(iu_mediainfo STATIC
        MediaInfoHelper.cpp
        MediaInfoHelper.h
)

source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "Sources" FILES
        MediaInfoHelper.cpp
        MediaInfoHelper.h
)

target_link_libraries(iu_mediainfo PRIVATE iu_core mediainfo)

set_target_properties(iu_mediainfo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib/)
