include(CMakeParseArguments)

add_library(qcoro_test STATIC testobject.cpp testloop.cpp)
target_link_libraries(qcoro_test PUBLIC
    QCoro${QT_VERSION_MAJOR}Core
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
    target_link_libraries(qcoro_test_dbus PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::DBus
    )
endif()

function(qcoro_add_test _name)
    set(options SKIP_ADD_TEST)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(
        test-${_name}
        qcoro_test
        QCoro${QT_VERSION_MAJOR}Core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        ${TEST_LINK_LIBRARIES}
        Threads::Threads
    )
    if (NOT TEST_SKIP_ADD_TEST)
        add_test(NAME test-${_name} COMMAND test-${_name})
    endif()
endfunction()

function(qcoro_add_network_test _name)
    qcoro_add_test(${_name} ${ARGN})
    target_link_libraries(
        test-${_name}
        QCoro${QT_VERSION_MAJOR}Network
        Qt${QT_VERSION_MAJOR}::Network
    )
    if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
        target_link_libraries(test-${_name} atomic)
    endif()
endfunction()

function(qcoro_add_dbus_test _name)
    qcoro_add_test(${_name} ${ARGN} SKIP_ADD_TEST)
    add_dependencies(test-${_name} testdbusserver qcoro_test_dbus)
    target_link_libraries(
        test-${_name}
        QCoro${QT_VERSION_MAJOR}DBus
        qcoro_test_dbus
    )
    target_include_directories(test-${_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(test-${_name} PRIVATE TESTDBUSSERVER_EXECUTABLE=\"$<TARGET_FILE:testdbusserver>\")
    if (APPLE)
        # On MacOS dbus-launch doesn't work, so we rely on the session dbus running
        add_test(NAME test-${_name} COMMAND test-${_name})
    else()
        add_test(NAME test-${_name} COMMAND dbus-launch $<TARGET_FILE:test-${_name}>)
    endif()
endfunction()



qcoro_add_test(qtimer)
qcoro_add_test(qcoroprocess)
qcoro_add_test(qcorosignal)
qcoro_add_test(qcorothread)
qcoro_add_test(task)
qcoro_add_test(testconstraints)
qcoro_add_test(qfuture LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Concurrent)

if (QCORO_WITH_QTDBUS)
    qcoro_add_dbus_test(qdbuspendingcall)
    qcoro_add_dbus_test(qdbuspendingreply)
endif()

if (QCORO_WITH_QTNETWORK)
    qcoro_add_network_test(qcorolocalsocket)
    qcoro_add_network_test(qcoroabstractsocket)
    qcoro_add_network_test(qcoronetworkreply)
    qcoro_add_network_test(qcorotcpserver)

    # Tests for test utilities
    qcoro_add_network_test(testhttpserver)
endif()
