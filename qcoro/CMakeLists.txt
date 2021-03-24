add_library(qcoro)
target_include_directories(qcoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)
