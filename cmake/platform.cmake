# directory
set(PLATFORM_DIR ${TOP_DIR}/platform)
set(PLATFORM_INC_DIR ${PLATFORM_DIR}/include)
set(PLATFORM_LINUX_DIR ${PLATFORM_DIR}/linux)
set(PLATFORM_LINUX_SRC_DIR ${PLATFORM_LINUX_DIR}/src)

# collect platform linux sources
set(PLATFORM_LINUX_SRCS
    ${PLATFORM_LINUX_SRC_DIR}/pal/board.c
    ${PLATFORM_LINUX_SRC_DIR}/pal/memory.c
    ${PLATFORM_LINUX_SRC_DIR}/main.c
)

# collect platform headers
set(PLATFORM_HEADERS
    ${PLATFORM_INC_DIR}/pal/board.h
    ${PLATFORM_INC_DIR}/pal/memory.h
)
