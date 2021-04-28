# generate lua binray from lua script
function(gen_lua_binary bin script)
    add_custom_command(OUTPUT ${bin}
        COMMAND ${luac_BINARY_DIR}/luac -o ${bin} ${script}
        DEPENDS luac ${script}
        COMMENT "Generating ${bin}"
    )
endfunction()

# genrate lua binraies in a directory
function(gen_lua_binary_from_dir dest_dir src_dir)
    # get all lua scripts
    file(GLOB_RECURSE scripts ${src_dir}/*.lua)
    foreach(script ${scripts})
        # get the relative path of the script
        file(RELATIVE_PATH rel_path ${src_dir} ${script})
        set(bin ${dest_dir}/${rel_path}.out)
        set(bins ${bins} ${bin})
        get_filename_component(dir ${bin} DIRECTORY)
        make_directory(${dir})
        gen_lua_binary(${bin} ${script})
    endforeach()
    add_custom_target(lua_binary ALL DEPENDS ${bins})
endfunction()
