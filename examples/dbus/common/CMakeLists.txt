set(common_SRCS
    dbusserver.cpp
)

add_library(examples-dbus-common STATIC ${common_SRCS})
target_include_directories(examples-dbus-common
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
target_link_libraries(examples-dbus-common
    PUBLIC
    Qt5::Core
    PRIVATE
    Qt5::DBus
)
