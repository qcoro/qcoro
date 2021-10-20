function(qcoro_add_io_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp)
    target_link_libraries(test-${_name} QCoroIO qcoro_test
                          Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test
                          ${TEST_LINK_LIBRARIES}
                          Threads::Threads)
    add_test(NAME test-io-${_name} COMMAND test-${_name})
endfunction()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    qcoro_add_io_test(iouringengine LINK_LIBRARIES qcoroio_iouring)
endif()

