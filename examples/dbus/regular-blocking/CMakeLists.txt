set(dbustest_SRCS
    main.cpp
)

add_executable(dbustest ${dbustest_SRCS})
set_target_properties(dbustest PROPERTIES AUTOMOC ON)
target_link_libraries(dbustest
    examples-dbus-common
    Threads::Threads
    Qt5::Core
    Qt5::DBus
)
