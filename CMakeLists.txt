# This is for using libuwifi as a component in ESP-IDF

idf_component_register(SRCS core/wlan_parser.c core/wlan_util.c
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "include/uwifi"
                       REQUIRES "")
