include(CMakeParseArguments)

add_library(qcoro_test STATIC testobject.cpp)
target_link_libraries(qcoro_test PUBLIC qcoro Qt5::Core Qt5::Test)

function(qcoro_add_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})

    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} qcoro qcoro_test Qt5::Core Qt5::Test
                         ${TEST_LINK_LIBRARIES} Threads::Threads)
    add_test(NAME test-${_name} COMMAND test-${_name})
endfunction()

function(qcoro_add_dbus_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})

    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} qcoro qcoro_test ${TEST_LINK_LIBRARIES})
    add_test(NAME test-${_name} COMMAND dbus-launch $<TARGET_FILE:test-${_name}>)
endfunction()



qcoro_add_test(qtimer)
qcoro_add_test(qnetworkreply LINK_LIBRARIES Qt5::Network)
qcoro_add_dbus_test(qdbuspendingcall LINK_LIBRARIES Qt5::DBus)
qcoro_add_test(qfuture LINK_LIBRARIES Qt5::Concurrent)
qcoro_add_test(qcoroprocess)
qcoro_add_test(qcorolocalsocket LINK_LIBRARIES Qt5::Network)
qcoro_add_test(qcoroabstractsocket LINK_LIBRARIES Qt5::Network)
qcoro_add_test(qcoronetworkreply LINK_LIBRARIES Qt5::Network)

# Tests for test utilities
qcoro_add_test(testhttpserver LINK_LIBRARIES Qt5::Network)
