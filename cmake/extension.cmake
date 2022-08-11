# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# Generate lua binray from lua script.
#
# out: absolute path of the output file
# in: input file path relative to ${dir}
# dir: the path in the debug information generated by luac will be relative to this directory
#
# gen_lua_binary(<out> <in> <dir> <luac> [DEBUG])
function(gen_lua_binary out in dir luac)
    set(options DEBUG)
    cmake_parse_arguments(arg "${options}" "" "" "${ARGN}")
    if(NOT arg_DEBUG)
        set(LUAC_FLAGS ${LUAC_FLAGS} -s)
    endif()
    add_custom_command(OUTPUT ${out}
        COMMAND cd ${dir}
        COMMAND ${luac} ${LUAC_FLAGS} -o ${out} ${in}
        DEPENDS ${luac} ${dir}/${in}
        COMMENT "Generating ${out}"
    )
endfunction(gen_lua_binary)

# Genrate lua binraies in a directory.
#
# gen_lua_binary_from_dir(<target> <dest_dir> <luac> [DEBUG]
#                         [SRC_DIRS dir1 [dir2...]])
function(gen_lua_binary_from_dir target dest_dir luac)
    set(options DEBUG)
    set(multi SRC_DIRS)
    cmake_parse_arguments(arg "${options}" "" "${multi}" "${ARGN}")
    if(arg_DEBUG)
        set(GEN_LUA_LIBRARY_OPTIONS DEBUG)
    endif()
    foreach(src_dir ${arg_SRC_DIRS})
        # get all lua scripts
        file(GLOB_RECURSE scripts RELATIVE ${src_dir} ${src_dir}/*.lua)
        foreach(script ${scripts})
            set(bin ${dest_dir}/${script}c)
            set(bins ${bins} ${bin})
            get_filename_component(dir ${bin} DIRECTORY)
            make_directory(${dir})
            gen_lua_binary(${bin} ${script} ${src_dir} ${luac}
                ${GEN_LUA_LIBRARY_OPTIONS}
            )
        endforeach()
    endforeach()
    add_custom_target(${target}
        ALL
        DEPENDS ${bins}
    )
endfunction(gen_lua_binary_from_dir)

# Find luac.
#
# find_luac(<output>)
function(find_luac output)
    if (NOT LUA_DIR)
        include (${TOP_DIR}/cmake/lua.cmake)
    endif ()
    find_program(MAKE NAMES "make")
    if(NOT MAKE)
        message(FATAL_ERROR "make not found")    
    endif()
    set(luac ${LUA_SRC_DIR}/luac)
    add_custom_command(OUTPUT ${luac}
        COMMAND ${MAKE} -s -C ${LUA_DIR} 1> /dev/null
        DEPENDS ${LUA_SRCS} ${LUAC_SRCS} ${LUA_HEADERS}
        COMMENT "Compiling luac"
    )
    add_custom_target(luac ALL DEPENDS ${luac})
    set(${output} ${luac} PARENT_SCOPE)
endfunction(find_luac)

# Check code style when building <target>.
#
# add_cstyle_check(<target> <top_dir> [SRCS src1 [src2...]])
function(target_check_cstyle target)
    find_program(CPPLINT NAMES "cpplint")
    if(NOT CPPLINT)
        message(FATAL_ERROR "cpplint not found")    
    endif()

    set(multi FILES)
    cmake_parse_arguments(arg "" "" "${multi}" "${ARGN}")

    set(cstyle_dir ${CMAKE_BINARY_DIR}/cstyle)
    foreach(file ${arg_FILES})
        # get the relative path of the script
        file(RELATIVE_PATH rel_path ${TOP_DIR} ${file})
        set(output ${cstyle_dir}/${rel_path}.cs)
        set(outputs ${outputs} ${output})
        get_filename_component(dir ${output} DIRECTORY)
        make_directory(${dir})
        add_custom_command(OUTPUT ${output}
            COMMENT "Checking ${rel_path}"
            COMMAND ${CPPLINT}
                --quiet
                --linelength=120
                --filter=-readability/casting,-build/include,-runtime/arrays,-runtime/int
                ${rel_path}
            COMMAND touch ${output}
            WORKING_DIRECTORY ${TOP_DIR}
            DEPENDS ${file}
        )
    endforeach()
    add_custom_target(${target}_cstyle
        ALL
        DEPENDS ${outputs}
    )
    add_dependencies(${target} ${target}_cstyle)
endfunction(target_check_cstyle)

#
# Add embedfs to a target.
#
# target_add_embedfs(<target> <dir> <root_name>)
function (target_add_embedfs target dir root_name)
    set(dest_dir ${CMAKE_BINARY_DIR}/${target}_${root_name})
    set(output ${dest_dir}/${target}_${root_name}.c)

    file(GLOB_RECURSE files RELATIVE ${dir} ${dir}/*)
    foreach(file ${files})
        set(header ${dest_dir}/${file}.h)
        set(headers ${headers} ${header})
        add_custom_command(OUTPUT ${header}
            COMMAND cd ${dir}
            COMMAND ${CMAKE_COMMAND}
            -D OUTPUT=${header}
            -D INPUT=${file}
            -P ${TOP_DIR}/cmake/bin2hex.cmake
            DEPENDS ${dir}/${file}
            COMMENT "Generating ${header}"
        )
    endforeach()

    add_custom_command(OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND}
            -D OUTPUT=${output}
            -D ROOT_DIR=${dir}
            -D DEST_DIR=${dest_dir}
            -D EMBEDFS_ROOT_NAME=${root_name}
            -P ${TOP_DIR}/cmake/gen_embedfs.cmake
        DEPENDS ${headers} ${TOP_DIR}/cmake/gen_embedfs.cmake
    )
    target_sources(${target}
        PRIVATE ${output}
    )
    target_include_directories(${target}
        PRIVATE ${dest_dir}
    )
endfunction(target_add_embedfs)

#
# Add lua binary embedfs to a target.
#
# target_add_lua_binary_embedfs(<target> <root_name> <luac> [DEBUG]
#                               [SRC_DIRS dir1 [dir2...]])
function(target_add_lua_binary_embedfs target root_name luac)
    set(options DEBUG)
    set(multi SRC_DIRS)
    cmake_parse_arguments(arg "${options}" "" "${multi}" "${ARGN}")
    if(arg_DEBUG)
        set(GEN_LUA_LIBRARY_OPTIONS DEBUG)
    endif()

    set(dest_dir ${CMAKE_BINARY_DIR}/${target}_${root_name})
    set(output ${dest_dir}/${target}_${root_name}.c)
    set(binary_dir ${CMAKE_BINARY_DIR}/${target}_${root_name}_bin)

    gen_lua_binary_from_dir(${target}_${root_name}_bin
        ${binary_dir}
        ${luac}
        ${GEN_LUA_LIBRARY_OPTIONS}
        SRC_DIRS ${arg_SRC_DIRS}
    )

    foreach(src_dir ${arg_SRC_DIRS})
        file(GLOB_RECURSE bins RELATIVE ${src_dir} ${src_dir}/*.lua)
        foreach(bin ${bins})
            set(bin ${bin}c)
            set(header ${dest_dir}/${bin}.h)
            set(headers ${headers} ${header})
            string(REGEX REPLACE "[/.]" "_" filename ${bin})
            add_custom_command(OUTPUT ${header}
                COMMAND cd ${binary_dir}
                COMMAND ${CMAKE_COMMAND}
                    -D OUTPUT=${header}
                    -D INPUT=${bin}
                    -P ${TOP_DIR}/cmake/bin2hex.cmake
                DEPENDS ${binary_dir}/${bin}
                COMMENT "Generating ${header}"
            )
        endforeach()
    endforeach()

    add_custom_command(OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND}
            -D OUTPUT=${output}
            -D ROOT_DIR=${binary_dir}
            -D DEST_DIR=${dest_dir}
            -D EMBEDFS_ROOT_NAME=${root_name}
            -P ${TOP_DIR}/cmake/gen_embedfs.cmake
        DEPENDS ${headers} ${TOP_DIR}/cmake/gen_embedfs.cmake
        COMMENT "Generating ${output}"
    )
    target_sources(${target}
        PRIVATE ${output}
    )
endfunction(target_add_lua_binary_embedfs)
