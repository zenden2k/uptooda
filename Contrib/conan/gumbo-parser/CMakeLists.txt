cmake_minimum_required(VERSION 4.0)							

project(gumbo C)

file(GLOB GUMBO_SRC gumbo-parser/src/*.c)

add_library(gumbo ${GUMBO_SRC})

target_include_directories(gumbo PUBLIC gumbo-parser/src)
if(MSVC)
    target_include_directories(gumbo PRIVATE gumbo-parser/visualc/include)
endif()
			 
install(FILES gumbo-parser/src/gumbo.h gumbo-parser/src/tag_enum.h
        DESTINATION include)
install(FILES gumbo-parser/COPYING DESTINATION licenses)
install(TARGETS gumbo
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)