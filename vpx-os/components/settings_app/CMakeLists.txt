set(PROJ_SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE PROJ_SRCS_C   ${PROJ_SRC}/*.c)
file(GLOB_RECURSE PROJ_SRCS_CPP ${PROJ_SRC}/*.cpp)

idf_component_register(
    SRCS  ${PROJ_SRCS_C} ${PROJ_SRCS_CPP}
    INCLUDE_DIRS ${PROJ_SRC}
    PRIV_REQUIRES espressif__brookesia_service_helper espressif__brookesia_service_manager waveshare__esp32_s3_touch_amoled_2_06
    # WHOLE_ARCHIVE forces the linker to include all object files even when
    # nothing directly references ClockApp. Without it, the plugin registration
    # macro is stripped and the app never appears in the launcher.
    WHOLE_ARCHIVE
)

set_source_files_properties(
    ${PROJ_SRCS_CPP}
    PROPERTIES
        COMPILE_FLAGS "-Wno-missing-field-initializers"
)
