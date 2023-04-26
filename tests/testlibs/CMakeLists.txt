add_library(qcoro_testlib
    STATIC
    testobject.cpp
    testloop.cpp
)
target_include_directories(qcoro_testlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qcoro_testlib PUBLIC
    QCoro${QT_VERSION_MAJOR}Core
    QCoro${QT_VERSION_MAJOR}Test
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

if (QCORO_WITH_QTDBUS)
    add_executable(testdbusserver EXCLUDE_FROM_ALL testdbusserver.cpp)
    target_link_libraries(testdbusserver
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::DBus
    )

    set(qcoro_test_dbus_SRCS)
    if (${QT_VERSION_MAJOR} EQUAL 5)
        qt5_add_dbus_interface(qcoro_test_dbus_SRCS cz.dvratil.qcorodbustest.xml qcorodbustestinterface)
    else()
        qt_add_dbus_interface(qcoro_test_dbus_SRCS cz.dvratil.qcorodbustest.xml qcorodbustestinterface)
    endif()

    add_library(qcoro_test_dbus OBJECT ${qcoro_test_dbus_SRCS})
    target_include_directories(qcoro_test_dbus PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(qcoro_test_dbus PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::DBus
    )
endif()

if (QCORO_WITH_QTWEBSOCKETS)
    set(qcoro_test_ws_SRCS testwsserver.cpp)
    add_library(qcoro_test_ws OBJECT ${qcoro_test_ws_SRCS})
    target_include_directories(qcoro_test_ws PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(qcoro_test_ws PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::WebSockets
    )
endif()
