# generate lua binray from lua script
function(gen_lua_binary bin script luac)
    add_custom_command(OUTPUT ${bin}
        COMMAND ${luac} -s -o ${bin} ${script}
        COMMAND echo "Generated ${bin}"
        DEPENDS ${luac} ${script}
        COMMENT "Generating ${bin}"
    )
endfunction(gen_lua_binary)

# genrate lua binraies in a directory
function(gen_lua_binary_from_dir dest_dir src_dir luac target)
    # get all lua scripts
    file(GLOB_RECURSE scripts ${src_dir}/*.lua)
    foreach(script ${scripts})
        # get the relative path of the script
        file(RELATIVE_PATH rel_path ${src_dir} ${script})
        set(bin ${dest_dir}/${rel_path}c)
        set(bins ${bins} ${bin})
        get_filename_component(dir ${bin} DIRECTORY)
        make_directory(${dir})
        gen_lua_binary(${bin} ${script} ${luac})
    endforeach()
    add_custom_target(${target}
        ALL
        DEPENDS ${bins}
    )
endfunction(gen_lua_binary_from_dir)

# compile luac
function(compile_luac bin src_dir build_dir)
    add_custom_command(OUTPUT ${bin}
        COMMAND cmake -H${src_dir} -B${build_dir} -G Ninja
        COMMAND cmake --build ${build_dir} -j10
        DEPENDS ${src_dir}/CMakeLists.txt
        COMMENT "Compiling luac"
    )
    add_custom_target(luac ALL DEPENDS ${bin})
endfunction(compile_luac)

# get host platform
macro(get_host_platform output)
    execute_process(
        COMMAND uname
        OUTPUT_VARIABLE ${output}
    )
    string(REPLACE "\n" "" ${output} ${${output}})
    string(TOLOWER ${${output}} ${output})
endmacro(get_host_platform)
