include(CMakeParseArguments)

add_library(qcoro_test STATIC testobject.cpp)
target_link_libraries(qcoro_test PUBLIC
    QCoroCore
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

if (QCORO_WITH_QTDBUS)
    add_executable(testdbusserver EXCLUDE_FROM_ALL testdbusserver.cpp)
    target_link_libraries(testdbusserver Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::DBus)

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
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} QCoroCore qcoro_test
                          Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test
                          ${TEST_LINK_LIBRARIES}
                          Threads::Threads)
    add_test(NAME test-${_name} COMMAND test-${_name})
endfunction()

function(qcoro_add_network_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} QCoroNetwork qcoro_test
                          Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test
                          Qt${QT_VERSION_MAJOR}::Network
                          Threads::Threads)
    add_test(NAME test-${_name} COMMAND test-${_name})
endfunction()

function(qcoro_add_dbus_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    set(test_SRCS ${_name}.cpp)
    add_executable(test-${_name} ${test_SRCS})
    add_dependencies(test-${_name} testdbusserver qcoro_test_dbus)
    target_link_libraries(test-${_name} QCoroDBus qcoro_test qcoro_test_dbus ${TEST_LINK_LIBRARIES})
    target_include_directories(test-${_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(test-${_name} PRIVATE TESTDBUSSERVER_EXECUTABLE=\"$<TARGET_FILE:testdbusserver>\")
    add_test(NAME test-${_name} COMMAND dbus-launch $<TARGET_FILE:test-${_name}>)
endfunction()



qcoro_add_test(qtimer)
qcoro_add_test(qcoroprocess)
qcoro_add_test(qcorosignal)
qcoro_add_test(task)
qcoro_add_test(testconstraints)
if (QCORO_QT_HAS_COMPAT_ABI)
    qcoro_add_test(qfuture LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Concurrent)
endif()

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
