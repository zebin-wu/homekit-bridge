# Generate lua binray from lua script.
#
# gen_lua_binary(BIN SCRIPT LUAC [DEBUG])
function(gen_lua_binary bin script luac)
    set(options DEBUG)
    cmake_parse_arguments(arg "${options}" "" "" "${ARGN}")
    if(NOT arg_DEBUG)
        set(LUAC_FLAGS ${LUAC_FLAGS} -s)
    endif()
    add_custom_command(OUTPUT ${bin}
        COMMAND ${luac} ${LUAC_FLAGS} -o ${bin} ${script}
        COMMAND echo "Generated ${bin}"
        DEPENDS ${luac} ${script}
        COMMENT "Generating ${bin}"
    )
endfunction(gen_lua_binary)

# Genrate lua binraies in a directory.
#
# gen_lua_binary_from_dir(TARGET DEST_DIR LUAC [DEBUG]
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
        file(GLOB_RECURSE scripts ${src_dir}/*.lua)
        foreach(script ${scripts})
            # get the relative path of the script
            file(RELATIVE_PATH rel_path ${src_dir} ${script})
            set(bin ${dest_dir}/${rel_path}c)
            set(bins ${bins} ${bin})
            get_filename_component(dir ${bin} DIRECTORY)
            make_directory(${dir})
            gen_lua_binary(${bin} ${script} ${luac}
                ${GEN_LUA_LIBRARY_OPTIONS}
            )
        endforeach()
    endforeach()
    add_custom_target(${target}
        ALL
        DEPENDS ${bins}
    )
endfunction(gen_lua_binary_from_dir)

# Compile luac.
#
# compile_luac(BIN SRC_DIR BUILD_DIR)
function(compile_luac bin src_dir build_dir)
    add_custom_command(OUTPUT ${bin}
        COMMAND cmake -H${src_dir} -B${build_dir} -G Ninja
        COMMAND cmake --build ${build_dir} -j10
        DEPENDS ${src_dir}/CMakeLists.txt
        COMMENT "Compiling luac"
    )
    add_custom_target(luac ALL DEPENDS ${bin})
endfunction(compile_luac)

# Get host platform.
#
# get_host_platform(OUTPUT)
macro(get_host_platform output)
    execute_process(
        COMMAND uname
        OUTPUT_VARIABLE ${output}
    )
    string(REPLACE "\n" "" ${output} ${${output}})
    string(TOLOWER ${${output}} ${output})
endmacro(get_host_platform)
