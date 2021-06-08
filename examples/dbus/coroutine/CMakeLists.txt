set(dbustest_SRCS
    main.cpp
)

add_executable(dbustest-coro ${dbustest_SRCS})
target_link_libraries(dbustest-coro
    QCoro
    examples-dbus-common
    Threads::Threads
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)
