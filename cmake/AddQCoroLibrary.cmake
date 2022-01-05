# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

include(GenerateHeaders)

function(add_qcoro_library)
    function(prefix_libraries)
        set(oneValueArgs PREFIX OUTPUT)
        set(multiValueArgs LIBRARIES)
        cmake_parse_arguments(prf "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        set(_libs)
        foreach(libname ${prf_LIBRARIES})
            if ("${libname}" MATCHES "PUBLIC|PRIVATE|INTERFACE")
                list(APPEND _libs "${libname}")
            else()
                list(APPEND _libs "${prf_PREFIX}::${libname}")
            endif()
        endforeach()

        set(${prf_OUTPUT} ${_libs} PARENT_SCOPE)
    endfunction()

    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES CAMELCASE_HEADERS HEADERS QCORO_LINK_LIBRARIES QT_LINK_LIBRARIES)

    cmake_parse_arguments(LIB "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target_name "${QCORO_TARGET_PREFIX}${LIB_NAME}")
    string(TOLOWER "${target_name}" target_name_lowercase)
    string(TOLOWER "${LIB_INCLUDEDIR}" includedir_lowercase)

    prefix_libraries(
        PREFIX ${QCORO_TARGET_PREFIX}
        LIBRARIES ${LIB_QCORO_LINK_LIBRARIES}
        OUTPUT qcoro_LIBS
    )

    prefix_libraries(
        PREFIX Qt${QT_VERSION_MAJOR}
        LIBRARIES ${LIB_QT_LINK_LIBRARIES}
        OUTPUT qt_LIBS
    )

    add_library(${target_name})
    if (LIB_NAME)
        add_library(${QCORO_TARGET_PREFIX}::${LIB_NAME} ALIAS ${target_name})
    endif()
    if (LIB_SOURCES)
        target_sources(${target_name} PRIVATE ${LIB_SOURCES})
    endif()

    target_include_directories(
        ${target_name}
        INTERFACE $<BUILD_INTERFACE:${QCORO_SOURCE_DIR}>;$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PUBLIC $<INSTALL_INTERFACE:${QCORO_INSTALL_INCLUDEDIR}>
        PUBLIC $<INSTALL_INTERFACE:${QCORO_INSTALL_INCLUDEDIR_CAMELCASE}>
    )

    if (LIB_NAME)
        # Link against QCoro{5,6} top-level target
        target_link_libraries(${target_name} PUBLIC ${QCORO_TARGET_PREFIX})
    endif()
    target_link_libraries(${target_name} ${qcoro_LIBS})
    target_link_libraries(${target_name} ${qt_LIBS})

    set_target_properties(
        ${target_name}
        PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS 1
        VERSION ${qcoro_VERSION}
        SOVERSION ${qcoro_SOVERSION}
    )

    generate_headers(
        camelcase_HEADERS
        HEADER_NAMES ${LIB_CAMELCASE_HEADERS}
        OUTPUT_DIR QCoro
        ORIGINAL_HEADERS_VAR source_HEADERS
    )

    install(
        FILES ${source_HEADERS} ${LIB_HEADERS}
        DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/${includedir_lowercase}
        COMPONENT Devel
    )
    install(
        FILES ${camelcase_HEADERS}
        DESTINATION ${QCORO_INSTALL_INCLUDEDIR_CAMELCASE}/${LIB_INCLUDEDIR}
        COMPONENT Devel
    )
    install(
        TARGETS ${target_name}
        EXPORT ${QCORO_TARGETS}
    )
endfunction()
