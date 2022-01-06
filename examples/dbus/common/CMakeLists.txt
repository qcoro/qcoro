
# Build the dbus-server as a stand-alone executable to workaround QTBUG-92107
add_executable(dbusserver dbusserver.cpp)
set_target_properties(dbusserver
    PROPERTIES COMPILE_DEFINITIONS STANDALONE
)
target_link_libraries(dbusserver
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

#-----------------------------------------------------#

set(common_SRCS
    dbusserver.cpp
)

add_library(examples-dbus-common STATIC ${common_SRCS})
set_target_properties(examples-dbus-common PROPERTIES COMPILE_DEFINITIONS SERVER_EXEC_PATH=\"$<TARGET_FILE:dbusserver>\")
target_include_directories(examples-dbus-common
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)
target_link_libraries(examples-dbus-common
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    PRIVATE
    Qt${QT_VERSION_MAJOR}::DBus
)
