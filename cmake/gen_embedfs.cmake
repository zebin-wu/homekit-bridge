# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

if(NOT OUTPUT)
    message(FATAL_ERROR "OUTPUT must be specified")
endif()

if (NOT ROOT_DIR)
    message(FATAL_ERROR "ROOT_DIR must be specified")
endif()

if (NOT EMBEDFS_ROOT_NAME)
    message(FATAL_ERROR "EMBEDFS_ROOT_NAME must be specified")
endif()

file(WRITE ${OUTPUT} "")

function(append_line str)
    file(APPEND ${OUTPUT} "${str}\n")
endfunction()

function(gen_embedfs_dir dir parent indent)
    append_line("${indent}.name = \"${dir}\",")
    append_line("${indent}.files = (const embedfs_file * const[]) {")
    file(GLOB files RELATIVE ${ROOT_DIR} ${parent}/${dir}/*)
    set(count 0)
    foreach(file ${files})
        if(NOT IS_DIRECTORY ${ROOT_DIR}/${file})
            math(EXPR count "${count} + 1")
            append_line("${indent}    & (const embedfs_file) {")
            get_filename_component(filename ${file} NAME)
            append_line("${indent}        .name = \"${filename}\",")
            string(REGEX REPLACE "[/.]" "_" filename ${file})
            append_line("${indent}        .data = ${filename},")
            append_line("${indent}        .len = ${filename}_len,")
            append_line("${indent}    },")
        endif()
    endforeach()
    append_line("${indent}},")
    append_line("${indent}.file_count = ${count},")

    append_line("${indent}.children = (const embedfs_dir * const[]) {")
    file(GLOB files RELATIVE ${parent}/${dir} ${parent}/${dir}/*)
    set(count 0)
    foreach(file ${files})
        if(IS_DIRECTORY ${parent}/${dir}/${file})
            math(EXPR count "${count} + 1")
            append_line("${indent}    & (const embedfs_dir) {")
            gen_embedfs_dir(${file} ${parent}/${dir} "${indent}        ")
            append_line("${indent}    },")
        endif()
    endforeach()
    append_line("${indent}},")
    append_line("${indent}.child_count = ${count},")
endfunction()

append_line("// Auto generated. Don't edit it manually!")
append_line("")
append_line("#include <embedfs.h>")

file(GLOB_RECURSE files RELATIVE ${ROOT_DIR} ${ROOT_DIR}/*)
foreach(file ${files})
    if(NOT IS_DIRECTORY ${ROOT_DIR}/${file})
        append_line("#include \"${file}.h\"")
    endif()
endforeach()

append_line("")
append_line("const embedfs_dir ${EMBEDFS_ROOT_NAME} = {")
gen_embedfs_dir("" ${ROOT_DIR} "    ")
append_line("};")
