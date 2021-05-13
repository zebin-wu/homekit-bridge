# directory
set(BRIDGE_DIR ${TOP_DIR}/bridge)
set(BRIDGE_SRC_DIR ${BRIDGE_DIR}/src)
set(BRIDGE_INC_DIR ${BRIDGE_DIR}/include)
set(BRIDGE_SCRIPTS_DIR ${BRIDGE_DIR}/scripts)

# collect bridge sources
set(BRIDGE_SRCS
    ${BRIDGE_SRC_DIR}/app.c
    ${BRIDGE_SRC_DIR}/db.c
    ${BRIDGE_SRC_DIR}/lloglib.c
    ${BRIDGE_SRC_DIR}/lhaplib.c
    ${BRIDGE_SRC_DIR}/lpallib.c
    ${BRIDGE_SRC_DIR}/lc.c
)

# collect bridge headers
set(BRIDGE_HEADERS
    ${BRIDGE_INC_DIR}/app.h
    ${BRIDGE_SRC_DIR}/app_int.h
    ${BRIDGE_SRC_DIR}/db.h
    ${BRIDGE_SRC_DIR}/lc.h
    ${BRIDGE_SRC_DIR}/lhaplib.h
    ${BRIDGE_SRC_DIR}/lloglib.h
    ${BRIDGE_SRC_DIR}/lpallib.h
)
