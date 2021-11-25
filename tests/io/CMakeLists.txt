function(qcoro_add_io_test _name)
    set(options)
    set(oneValueArgs)
    set(multiValueAgs LINK_LIBRARIES SOURCES)
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueAgs}" ${ARGN})
    add_executable(test-${_name} ${_name}.cpp ${TEST_SOURCES})
    target_link_libraries(test-${_name} qcoro_test
                          Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test
                          ${TEST_LINK_LIBRARIES}
                          Threads::Threads)
    add_test(NAME test-io-${_name} COMMAND test-${_name})
endfunction()

set(IO_SOURCE_DIR ${CMAKE_SOURCE_DIR}/qcoro/io)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    qcoro_add_io_test(iouringengine
        SOURCES ${IO_SOURCE_DIR}/ioengine.cpp ${IO_SOURCE_DIR}/ioengine_iouring.cpp
        LINK_LIBRARIES Liburing::Liburing
    )
endif()

