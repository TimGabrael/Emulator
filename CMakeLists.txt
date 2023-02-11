cmake_minimum_required(VERSION 3.7)
project(EmulatorProj)


file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.c ${CMAKE_CURRENT_LIST_DIR}/src/*.h)

function(embed_resource resource_file_name source_file_name variable_name)
    file(READ ${resource_file_name} hex_content HEX)
    string(REPEAT "[0-9a-f]" 32 column_pattern)
    string(REGEX REPLACE "(${column_pattern})" "\\1\n" content "${hex_content}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")
    string(REGEX REPLACE ", $" "" content "${content}")
    set(array_definition "static const unsigned char ${variable_name}[] =\n{\n${content}\n};")
    set(source "// Auto generated file.\n${array_definition}\n")
    file(WRITE "${source_file_name}" "${source}")
endfunction()

embed_resource("data/ps1-bios/SCPH1001.BIN" "data/PS1_BIOS.h" "PS1_BIOS")

if(WIN32)
	set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/SDL2/include)

	add_executable(emu WIN32 ${SRC_FILES})

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		# 64 bits
		set(SDL2_LIBRARIES ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x64/SDL2.lib ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x64/SDL2main.lib)
		add_custom_command(
        TARGET emu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/SDL2/lib/x64/SDL2.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/SDL2.dll)

	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    # 32 bits
		set(SDL2_LIBRARIES ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x86/SDL2.lib ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x86/SDL2main.lib)
		add_custom_command(
        TARGET emu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/SDL2/lib/x86/SDL2.dll
                ${CMAKE_CURRENT_LIST_DIR}/bin/SDL2.dll)

	endif()
	


elseif(EMSCRIPTEN)

	set(CMAKE_C_FLAGS "-s ASYNCIFY=1 -s USE_SDL=2 -O2 -s FORCE_FILESYSTEM=1 -s \"EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']\"")
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	add_executable(emu ${SRC_FILES})
	
	target_include_directories(emu PRIVATE D:/SDKs/emsdk/upstream/emscripten/cache/sysroot/include ${SDL2_INCLUDE_DIRS})


else()

	find_package(SDL2 REQUIRED)
	add_executable(emu ${SRC_FILES})


endif()

set_target_properties(emu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

target_include_directories(emu PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(emu PRIVATE ${SDL2_LIBRARIES})