if(NOT OUTPUT)
    message(FATAL_ERROR "OUTPUT must be specified")
endif()

if(NOT INPUT)
    message(FATAL_ERROR "INPUT must be specified")
endif()

string(REGEX REPLACE "[/.]" "_" filename ${INPUT})
file(WRITE ${OUTPUT} "")

file(APPEND ${OUTPUT} "// Auto generated. Don't edit it manually!\n\n")
file(APPEND ${OUTPUT} "static const char ${filename}[] = {\n")

set(offset 0)
while(1)
    file(READ ${INPUT} hex
        OFFSET ${offset}
        LIMIT 1
        HEX
    )
    if("${hex}" STREQUAL "")
        break()
    endif()
    math(EXPR offset "${offset} + 1")
    file(APPEND ${OUTPUT} "0x${hex}, ")
endwhile()
file(APPEND ${OUTPUT} "};\n")
file(APPEND ${OUTPUT} "#define ${filename}_len ${offset}\n")
