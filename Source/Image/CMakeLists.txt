set(SRC_LIST
    ThumbnailMaker.cpp
)

set(HEADER_LIST
    ThumbnailMaker.h
)

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Sources" FILES ${SRC_LIST} ${HEADER_LIST})

add_library(iu_image STATIC ${SRC_LIST} ${HEADER_LIST})

target_include_directories(iu_image PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(iu_image PUBLIC
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)
