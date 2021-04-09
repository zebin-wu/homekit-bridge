# directory
set(BRIDGE_DIR "${TOP_DIR}/bridge")
set(BRIDGE_SRC_DIR "${BRIDGE_DIR}/src")
set(BRIDGE_INC_DIR "${BRIDGE_DIR}/include")
set(BRIDGE_LUA_DIR "${BRIDGE_DIR}/lua")

# collect bridge sources
set(BRIDGE_SRCS "${BRIDGE_SRC_DIR}/App.c"
                "${BRIDGE_SRC_DIR}/lhaplib.c"
                "${BRIDGE_SRC_DIR}/DB.c")
