# directory
set(COMMON_DIR "${TOP_DIR}/common")
set(RBTREE_DIR "${TOP_DIR}/ext/rbtree")
set(COMMON_SRC_DIR "${COMMON_DIR}/src")
set(COMMON_INC_DIR  "${COMMON_DIR}/include" "${RBTREE_DIR}")

# collect common sources
set(COMMON_SRCS "${COMMON_SRC_DIR}/lapi.c"
                "${RBTREE_DIR}/rbtree.c")
