set(dbustest_SRCS
    main.cpp
)

add_executable(dbustest-coro ${dbustest_SRCS})
target_link_libraries(dbustest-coro
    qcoro
    examples-dbus-common
    Threads::Threads
    Qt5::Core
    Qt5::DBus
)
