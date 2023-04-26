add_subdirectory(testlibs)

include(CMakeParseArguments)

function(_enable_supressions _name)
    function(_enable_supressions_file _filename)
        if (EXISTS "${_filename}")
            set_tests_properties(test-${_name} PROPERTIES
                ENVIRONMENT LSAN_OPTIONS=suppressions=${_filename}
            )
        endif()
    endfunction()

    _enable_supressions_file("${CMAKE_CURRENT_SOURCE_DIR}/${_name}-qt${QT_VERSION_MAJOR}-lsan.supp")
endfunction()

function(qcoro_add_test _name)
    set(options SKIP_ADD_TEST)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(
        test-${_name}
        PRIVATE
        qcoro_testlib
        QCoro${QT_VERSION_MAJOR}Core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        ${TEST_LINK_LIBRARIES}
        Threads::Threads
    )
    if (NOT TEST_SKIP_ADD_TEST)
        add_test(NAME test-${_name} COMMAND test-${_name})
        _enable_supressions(${_name})
    endif()

    target_code_coverage(test-${_name} AUTO ALL EXCLUDE "${CMAKE_BINARY_DIR}" "tests/testlibs/*")
endfunction()

function(qcoro_add_network_test _name)
    qcoro_add_test(${_name} ${ARGN})
    target_link_libraries(
        test-${_name}
        PRIVATE
        QCoro${QT_VERSION_MAJOR}Network
        Qt${QT_VERSION_MAJOR}::Network
    )
    if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
        target_link_libraries(test-${_name} PRIVATE atomic)
    endif()
endfunction()

function(qcoro_add_dbus_test _name)
    qcoro_add_test(${_name} ${ARGN} SKIP_ADD_TEST)
    add_dependencies(test-${_name} testdbusserver qcoro_test_dbus)
    target_link_libraries(
        test-${_name}
        PRIVATE
        QCoro${QT_VERSION_MAJOR}DBus
        qcoro_test_dbus
    )
    target_compile_definitions(test-${_name} PRIVATE TESTDBUSSERVER_EXECUTABLE=\"$<TARGET_FILE:testdbusserver>\")
    if (APPLE)
        # On MacOS dbus-launch doesn't work, so we rely on the session dbus running
        add_test(NAME test-${_name} COMMAND test-${_name})
    else()
        add_test(NAME test-${_name} COMMAND dbus-launch $<TARGET_FILE:test-${_name}>)
    endif()
    _enable_supressions(${_name})
endfunction()

function(qcoro_add_websockets_test _name)
    qcoro_add_network_test(${_name} ${ARGN})
    target_link_libraries(
        test-${_name}
        PRIVATE
        QCoro${QT_VERSION_MAJOR}WebSockets
        Qt${QT_VERSION_MAJOR}::WebSockets
        qcoro_test_ws
    )
endfunction()

function(qcoro_add_qml_test _name)
    qcoro_add_network_test(${_name} ${ARGN})
    target_link_libraries(
        test-${_name}
        PRIVATE
        QCoro${QT_VERSION_MAJOR}Qml
        Qt${QT_VERSION_MAJOR}::Qml
    )
endfunction()

function(qcoro_add_quick_test _name)
    qcoro_add_test(${_name} ${ARGN})
    target_link_libraries(
        test-${_name}
        PRIVATE
        QCoro${QT_VERSION_MAJOR}Quick
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::QuickPrivate
    )
    get_test_property(test-${_name} ENVIRONMENT _env)
    list(APPEND _env QT_QPA_PLATFORM=offscreen)
    set_tests_properties(test-${_name} PROPERTIES ENVIRONMENT "${_env}")
endfunction()

qcoro_add_test(qtimer)
qcoro_add_test(qcoroprocess)
qcoro_add_test(qcorosignal)
qcoro_add_test(qcorothread)
qcoro_add_test(qcorotask)
qcoro_add_test(testconstraints)
qcoro_add_test(qfuture LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Concurrent)
qcoro_add_test(qcorogenerator)
qcoro_add_test(qcoroasyncgenerator)

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

if (QCORO_WITH_QTWEBSOCKETS)
    qcoro_add_websockets_test(qcorowebsocket)
    qcoro_add_websockets_test(qcorowebsocketserver)
endif()

if (QCORO_WITH_QML)
    qcoro_add_qml_test(qcoroqmltask)
endif()

if (QCORO_WITH_QTQUICK)
    qcoro_add_quick_test(qcoroimageprovider)
endif()

