include(CMakeParseArguments)

add_library(qcoro_test STATIC testobject.cpp)
target_link_libraries(qcoro_test PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)


function(qcoro_add_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})

    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} QCoro qcoro_test Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test
                         ${TEST_LINK_LIBRARIES} Threads::Threads)
    add_test(NAME test-${_name} COMMAND test-${_name})
endfunction()

function(qcoro_add_dbus_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    set(test_SRCS ${_name}.cpp testdbusserver.cpp)
    if (${QT_VERSION_MAJOR} EQUAL 5)
        qt5_add_dbus_interface(test_SRCS cz.dvratil.qcorodbustest.xml qcorodbustestinterface)
    else()
        qt_add_dbus_interface(test_SRCS cz.dvratil.qcorodbustest.xml qcorodbustestinterface)
    endif()
    add_executable(test-${_name} ${test_SRCS})
    target_link_libraries(test-${_name} QCoro qcoro_test ${TEST_LINK_LIBRARIES})
    target_include_directories(test-${_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME test-${_name} COMMAND dbus-launch $<TARGET_FILE:test-${_name}>)
endfunction()



qcoro_add_test(qtimer)
qcoro_add_test(qnetworkreply LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
qcoro_add_dbus_test(qdbuspendingcall LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::DBus)
qcoro_add_dbus_test(qdbuspendingreply LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::DBus)
if (QT_HAS_COMPAT_ABI)
    qcoro_add_test(qfuture LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Concurrent)
endif()
qcoro_add_test(qcoroprocess)
qcoro_add_test(qcorolocalsocket LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
qcoro_add_test(qcoroabstractsocket LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
qcoro_add_test(qcoronetworkreply LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
qcoro_add_test(qcorotcpserver LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
qcoro_add_test(qcorosignal)
qcoro_add_test(task)

# Tests for test utilities
qcoro_add_test(testhttpserver LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Network)
