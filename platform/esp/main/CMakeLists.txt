set(APP_SRCS    "app_main.c"
                "app_wifi.c")

idf_component_register(SRCS "${APP_SRCS}"
                    INCLUDE_DIRS ".")
