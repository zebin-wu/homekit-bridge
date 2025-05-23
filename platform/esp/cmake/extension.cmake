# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

function(__build_component_info components output)
    set(components_json "")
    foreach(name ${components})
        __component_get_target(target ${name})
        __component_get_property(alias ${target} COMPONENT_ALIAS)
        __component_get_property(prefix ${target} __PREFIX)
        __component_get_property(dir ${target} COMPONENT_DIR)
        __component_get_property(type ${target} COMPONENT_TYPE)
        __component_get_property(lib ${target} COMPONENT_LIB)
        __component_get_property(reqs ${target} REQUIRES)
        __component_get_property(include_dirs ${target} INCLUDE_DIRS)
        __component_get_property(priv_reqs ${target} PRIV_REQUIRES)
        __component_get_property(managed_reqs ${target} MANAGED_REQUIRES)
        __component_get_property(managed_priv_reqs ${target} MANAGED_PRIV_REQUIRES)
        if("${type}" STREQUAL "LIBRARY")
            set(file "$<TARGET_LINKER_FILE:${lib}>")

            # The idf_component_register function is converting each source file path defined
            # in SRCS into absolute one. But source files can be also added with cmake's
            # target_sources and have relative paths. This is used for example in log
            # component. Let's make sure all source files have absolute path.
            set(sources "")
            get_target_property(srcs ${lib} SOURCES)
            foreach(src ${srcs})
                get_filename_component(src "${src}" ABSOLUTE BASE_DIR "${dir}")
                list(APPEND sources "${src}")
            endforeach()

        else()
            set(file "")
            set(sources "")
        endif()

        make_json_list("${reqs}" reqs)
        make_json_list("${priv_reqs}" priv_reqs)
        make_json_list("${managed_reqs}" managed_reqs)
        make_json_list("${managed_priv_reqs}" managed_priv_reqs)
        make_json_list("${include_dirs}" include_dirs)
        make_json_list("${sources}" sources)

        string(JOIN "\n" component_json
            "        \"${name}\": {"
            "            \"alias\": \"${alias}\","
            "            \"target\": \"${target}\","
            "            \"prefix\": \"${prefix}\","
            "            \"dir\": \"${dir}\","
            "            \"type\": \"${type}\","
            "            \"lib\": \"${lib}\","
            "            \"reqs\": ${reqs},"
            "            \"priv_reqs\": ${priv_reqs},"
            "            \"managed_reqs\": ${managed_reqs},"
            "            \"managed_priv_reqs\": ${managed_priv_reqs},"
            "            \"file\": \"${file}\","
            "            \"sources\": ${sources},"
            "            \"include_dirs\": ${include_dirs}"
            "        }"
        )
        string(CONFIGURE "${component_json}" component_json)
        if(NOT "${components_json}" STREQUAL "")
            string(APPEND components_json ",\n")
        endif()
        string(APPEND components_json "${component_json}")
    endforeach()
    string(PREPEND components_json "{\n")
    string(APPEND components_json "\n    }")
    set(${output} "${components_json}" PARENT_SCOPE)
endfunction()

function(__all_component_info output)
    set(components_json "")

    idf_build_get_property(build_prefix __PREFIX)
    idf_build_get_property(component_targets __COMPONENT_TARGETS)

    foreach(target ${component_targets})
        __component_get_property(name ${target} COMPONENT_NAME)
        __component_get_property(alias ${target} COMPONENT_ALIAS)
        __component_get_property(prefix ${target} __PREFIX)
        __component_get_property(dir ${target} COMPONENT_DIR)
        __component_get_property(type ${target} COMPONENT_TYPE)
        __component_get_property(lib ${target} COMPONENT_LIB)
        __component_get_property(reqs ${target} REQUIRES)
        __component_get_property(include_dirs ${target} INCLUDE_DIRS)
        __component_get_property(priv_reqs ${target} PRIV_REQUIRES)
        __component_get_property(managed_reqs ${target} MANAGED_REQUIRES)
        __component_get_property(managed_priv_reqs ${target} MANAGED_PRIV_REQUIRES)

        if(prefix STREQUAL build_prefix)
            set(name ${name})
        else()
            set(name ${alias})
        endif()

        make_json_list("${reqs}" reqs)
        make_json_list("${priv_reqs}" priv_reqs)
        make_json_list("${managed_reqs}" managed_reqs)
        make_json_list("${managed_priv_reqs}" managed_priv_reqs)
        make_json_list("${include_dirs}" include_dirs)

        string(JOIN "\n" component_json
            "        \"${name}\": {"
            "            \"alias\": \"${alias}\","
            "            \"target\": \"${target}\","
            "            \"prefix\": \"${prefix}\","
            "            \"dir\": \"${dir}\","
            "            \"lib\": \"${lib}\","
            "            \"reqs\": ${reqs},"
            "            \"priv_reqs\": ${priv_reqs},"
            "            \"managed_reqs\": ${managed_reqs},"
            "            \"managed_priv_reqs\": ${managed_priv_reqs},"
            "            \"include_dirs\": ${include_dirs}"
            "        }"
        )
        string(CONFIGURE "${component_json}" component_json)
        if(NOT "${components_json}" STREQUAL "")
            string(APPEND components_json ",\n")
        endif()
        string(APPEND components_json "${component_json}")
    endforeach()
    string(PREPEND components_json "{\n")
    string(APPEND components_json "\n    }")
    set(${output} "${components_json}" PARENT_SCOPE)
endfunction()

#
# Output the built components to the user. Generates files for invoking esp_idf_monitor
# that doubles as an overview of some of the more important build properties.
#
function(project_info test_components)
    idf_build_get_property(prefix __PREFIX)
    idf_build_get_property(_build_components BUILD_COMPONENTS)
    idf_build_get_property(build_dir BUILD_DIR)
    idf_build_get_property(idf_path IDF_PATH)

    list(SORT _build_components)

    unset(build_components)
    unset(build_component_paths)

    foreach(build_component ${_build_components})
        __component_get_target(component_target "${build_component}")
        __component_get_property(_name ${component_target} COMPONENT_NAME)
        __component_get_property(_prefix ${component_target} __PREFIX)
        __component_get_property(_alias ${component_target} COMPONENT_ALIAS)
        __component_get_property(_dir ${component_target} COMPONENT_DIR)

        if(_alias IN_LIST test_components)
            list(APPEND test_component_paths ${_dir})
        else()
            if(_prefix STREQUAL prefix)
                set(component ${_name})
            else()
                set(component ${_alias})
            endif()
            list(APPEND build_components ${component})
            list(APPEND build_component_paths ${_dir})
        endif()
    endforeach()

    set(PROJECT_NAME ${CMAKE_PROJECT_NAME})
    idf_build_get_property(PROJECT_VER PROJECT_VER)
    idf_build_get_property(PROJECT_PATH PROJECT_DIR)
    idf_build_get_property(BUILD_DIR BUILD_DIR)
    idf_build_get_property(SDKCONFIG SDKCONFIG)
    idf_build_get_property(SDKCONFIG_DEFAULTS SDKCONFIG_DEFAULTS)
    idf_build_get_property(PROJECT_EXECUTABLE EXECUTABLE)
    set(PROJECT_BIN ${CMAKE_PROJECT_NAME}.bin)
    idf_build_get_property(IDF_VER IDF_VER)
    idf_build_get_property(common_component_reqs __COMPONENT_REQUIRES_COMMON)

    idf_build_get_property(sdkconfig_cmake SDKCONFIG_CMAKE)
    include(${sdkconfig_cmake})
    idf_build_get_property(COMPONENT_KCONFIGS KCONFIGS)
    idf_build_get_property(COMPONENT_KCONFIGS_PROJBUILD KCONFIG_PROJBUILDS)
    idf_build_get_property(debug_prefix_map_gdbinit DEBUG_PREFIX_MAP_GDBINIT)

    if(CONFIG_APP_BUILD_TYPE_RAM)
        set(PROJECT_BUILD_TYPE ram_app)
    else()
        set(PROJECT_BUILD_TYPE flash_app)
    endif()

    # Write project description JSON file
    idf_build_get_property(build_dir BUILD_DIR)
    make_json_list("${build_components};${test_components}" build_components_json)
    make_json_list("${build_component_paths};${test_component_paths}" build_component_paths_json)
    make_json_list("${common_component_reqs}" common_component_reqs_json)

    __build_component_info("${build_components};${test_components}" build_component_info_json)
    __all_component_info(all_component_info_json)

    # The configure_file function doesn't process generator expressions, which are needed
    # e.g. to get component target library(TARGET_LINKER_FILE), so the project_description
    # file is created in two steps. The first step, with configure_file, creates a temporary
    # file with cmake's variables substituted and unprocessed generator expressions. The second
    # step, with file(GENERATE), processes the temporary file and substitute generator expression
    # into the final project_description.json file.
    configure_file("${idf_path}/tools/cmake/project_description.json.in"
        "${build_dir}/project_description.json.templ")
    file(READ "${build_dir}/project_description.json.templ" project_description_json_templ)
    file(REMOVE "${build_dir}/project_description.json.templ")
    file(GENERATE OUTPUT "${build_dir}/project_description.json"
         CONTENT "${project_description_json_templ}")

    # Generate component dependency graph
    depgraph_generate("${build_dir}/component_deps.dot")

    # We now have the following component-related variables:
    #
    # build_components is the list of components to include in the build.
    # build_component_paths is the paths to all of these components, obtained from the component dependencies file.
    #
    # Print the list of found components and test components
    string(REPLACE ";" " " build_components "${build_components}")
    string(REPLACE ";" " " build_component_paths "${build_component_paths}")
    message(STATUS "Components: ${build_components}")
    message(STATUS "Component paths: ${build_component_paths}")

    if(test_components)
        string(REPLACE ";" " " test_components "${test_components}")
        string(REPLACE ";" " " test_component_paths "${test_component_paths}")
        message(STATUS "Test components: ${test_components}")
        message(STATUS "Test component paths: ${test_component_paths}")
    endif()
endfunction()

#
# Add idf commands
#
function(add_idf_commands)
    set(mapfile "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
    set(idf_target "${IDF_TARGET}")
    string(TOUPPER ${idf_target} idf_target)
    # Add cross-reference table to the map file
    target_link_options(${TARGET} PRIVATE "-Wl,--cref")
    # Add this symbol as a hint for esp_idf_size to guess the target name
    target_link_options(${TARGET} PRIVATE "-Wl,--defsym=IDF_TARGET_${idf_target}=0")
    # Enable map file output
    target_link_options(${TARGET} PRIVATE "-Wl,--Map=${mapfile}")
    # Check if linker supports --no-warn-rwx-segments
    execute_process(COMMAND ${CMAKE_LINKER} "--no-warn-rwx-segments" "--version"
        RESULT_VARIABLE result
        OUTPUT_QUIET
        ERROR_QUIET)
    if(${result} EQUAL 0)
        # Do not print RWX segment warnings
        target_link_options(${TARGET} PRIVATE "-Wl,--no-warn-rwx-segments")
    endif()
    if(CONFIG_COMPILER_ORPHAN_SECTIONS_WARNING)
        # Print warnings if orphan sections are found
        target_link_options(${TARGET} PRIVATE "-Wl,--orphan-handling=warn")
    endif()
    unset(idf_target)

    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES
        "${mapfile}" "${project_elf_src}")

    idf_build_get_property(idf_path IDF_PATH)
    idf_build_get_property(python PYTHON)

    set(idf_size ${python} -m esp_idf_size)

    # Add size targets, depend on map file, run esp_idf_size
    # OUTPUT_JSON is passed for compatibility reasons, SIZE_OUTPUT_FORMAT
    # environment variable is recommended and has higher priority
    add_custom_target(size
        COMMAND ${CMAKE_COMMAND}
        -D "IDF_SIZE_TOOL=${idf_size}"
        -D "MAP_FILE=${mapfile}"
        -D "OUTPUT_JSON=${OUTPUT_JSON}"
        -P "${idf_path}/tools/cmake/run_size_tool.cmake"
        DEPENDS ${mapfile}
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(size-files
        COMMAND ${CMAKE_COMMAND}
        -D "IDF_SIZE_TOOL=${idf_size}"
        -D "IDF_SIZE_MODE=--files"
        -D "MAP_FILE=${mapfile}"
        -D "OUTPUT_JSON=${OUTPUT_JSON}"
        -P "${idf_path}/tools/cmake/run_size_tool.cmake"
        DEPENDS ${mapfile}
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(size-components
        COMMAND ${CMAKE_COMMAND}
        -D "IDF_SIZE_TOOL=${idf_size}"
        -D "IDF_SIZE_MODE=--archives"
        -D "MAP_FILE=${mapfile}"
        -D "OUTPUT_JSON=${OUTPUT_JSON}"
        -P "${idf_path}/tools/cmake/run_size_tool.cmake"
        DEPENDS ${mapfile}
        USES_TERMINAL
        VERBATIM
    )

    unset(idf_size)

    # Add DFU build and flash targets
    __add_dfu_targets()

    # Add uf2 related targets
    idf_build_get_property(idf_path IDF_PATH)
    idf_build_get_property(python PYTHON)

    set(UF2_ARGS --json "${CMAKE_CURRENT_BINARY_DIR}/flasher_args.json")
    set(UF2_CMD ${python} "${idf_path}/tools/mkuf2.py" write --chip ${chip_model})

    add_custom_target(uf2
        COMMAND ${CMAKE_COMMAND}
        -D "IDF_PATH=${idf_path}"
        -D "UF2_CMD=${UF2_CMD}"
        -D "UF2_ARGS=${UF2_ARGS};-o;${CMAKE_CURRENT_BINARY_DIR}/uf2.bin"
        -P "${idf_path}/tools/cmake/run_uf2_cmds.cmake"
        USES_TERMINAL
        VERBATIM
        )

    add_custom_target(uf2-app
        COMMAND ${CMAKE_COMMAND}
        -D "IDF_PATH=${idf_path}"
        -D "UF2_CMD=${UF2_CMD}"
        -D "UF2_ARGS=${UF2_ARGS};-o;${CMAKE_CURRENT_BINARY_DIR}/uf2-app.bin;--bin;app"
        -P "${idf_path}/tools/cmake/run_uf2_cmds.cmake"
        USES_TERMINAL
        VERBATIM
        )

endfunction(add_idf_commands)

#
# Sets initial list of build specifications (compile options, definitions, etc.) common across
# all library targets built under the ESP-IDF build system. These build specifications are added
# privately using the directory-level CMake commands (add_compile_options, include_directories, etc.)
# during component registration.
#
function(init_compiler_options)
    # Use generator expression so that users can append/override flags even after call to
    # idf_build_process
    idf_build_get_property(include_directories INCLUDE_DIRECTORIES GENERATOR_EXPRESSION)
    idf_build_get_property(compile_options COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(compile_definitions COMPILE_DEFINITIONS GENERATOR_EXPRESSION)
    idf_build_get_property(c_compile_options C_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(cxx_compile_options CXX_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(asm_compile_options ASM_COMPILE_OPTIONS GENERATOR_EXPRESSION)
    idf_build_get_property(common_reqs ___COMPONENT_REQUIRES_COMMON)

    include_directories("${include_directories}")
    add_compile_options("${compile_options}")
    add_compile_definitions("${compile_definitions}")
    add_c_compile_options("${c_compile_options}")
    add_cxx_compile_options("${cxx_compile_options}")
    add_asm_compile_options("${asm_compile_options}")
endfunction(init_compiler_options)
