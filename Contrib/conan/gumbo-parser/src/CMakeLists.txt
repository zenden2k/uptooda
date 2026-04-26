cmake_minimum_required(VERSION 4.0)							

project(gumbo-parser)

set(SRC_LIST 
		src/attribute.c
		src/char_ref.c
		src/error.c
		src/parser.c
		src/string_buffer.c
		src/string_piece.c
		src/tag.c
		src/tokenizer.c
		src/utf8.c
		src/util.c
		src/vector.c
)

set(HEADER_FILES  
		src/attribute.h
		src/char_ref.h
		src/error.h
		src/gumbo.h
		src/insertion_mode.h
		src/parser.h
		src/string_buffer.h
		src/string_piece.h
		src/tokenizer.h
		src/tokenizer_states.h
		src/token_type.h
		src/utf8.h
		src/util.h
		src/vector.h
)

if(WIN32)
	include_directories(visualc/include/)
endif()

add_library(gumbo-parser STATIC ${SRC_LIST})
			 
#			 set_property(TARGET gumbo-parser PROPERTY
#		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")