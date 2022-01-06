# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

include(GenerateHeaders)
include(CMakePackageConfigHelpers)

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

    set(params INTERFACE)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES CAMELCASE_HEADERS HEADERS QCORO_LINK_LIBRARIES QT_LINK_LIBRARIES)

    cmake_parse_arguments(LIB "${params}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target_name "${QCORO_TARGET_PREFIX}${LIB_NAME}")
    string(TOLOWER "${target_name}" target_name_lowercase)
    set(target_interface)
    set(target_include_interface "PUBLIC")
    if (LIB_INTERFACE)
        set(target_interface "INTERFACE")
        set(target_include_interface "INTERFACE")
    endif()

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

    # TODO: How is it done in Qt?
    # We want to export target QCoro5::Network but so far we are exporting
    # QCoro5::QCoro5Network :shrug:
    add_library(${target_name} ${target_interface})
    add_library(${QCORO_TARGET_PREFIX}::${LIB_NAME} ALIAS ${target_name})
    if (LIB_SOURCES)
        target_sources(${target_name} PRIVATE ${LIB_SOURCES})
    endif()

    target_include_directories(
        ${target_name}
        ${target_include_interface} $<BUILD_INTERFACE:${QCORO_SOURCE_DIR}>
        ${target_include_interface} $<BUILD_INTERFACE:${QCORO_SOURCE_DIR}/qcoro>
        ${target_include_interface} $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${target_include_interface} $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        ${target_include_interface} $<INSTALL_INTERFACE:${QCORO_INSTALL_INCLUDEDIR}>
        ${target_include_interface} $<INSTALL_INTERFACE:${QCORO_INSTALL_INCLUDEDIR}/qcoro>
        ${target_include_interface} $<INSTALL_INTERFACE:${QCORO_INSTALL_INCLUDEDIR}/QCoro>
    )

    target_link_libraries(${target_name} ${qcoro_LIBS})
    target_link_libraries(${target_name} ${qt_LIBS})

    set_target_properties(
        ${target_name}
        PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS 1
        VERSION ${qcoro_VERSION}
        SOVERSION ${qcoro_SOVERSION}
        EXPORT_NAME ${LIB_NAME}
    )

    generate_headers(
        camelcase_HEADERS
        HEADER_NAMES ${LIB_CAMELCASE_HEADERS}
        OUTPUT_DIR QCoro
        ORIGINAL_HEADERS_VAR source_HEADERS
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/QCoro${LIB_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${target_name}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${target_name}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${target_name}ConfigVersion.cmake"
        VERSION ${qcoro_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        TARGETS ${target_name}
        EXPORT ${target_name}Targets
    )
    install(
        FILES ${source_HEADERS} ${LIB_HEADERS}
        DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/qcoro/
        COMPONENT Devel
    )
    install(
        FILES ${camelcase_HEADERS}
        DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/QCoro/
        COMPONENT Devel
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${target_name}Config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/${target_name}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${target_name}"
        COMPONENT Devel
    )
    install(
        EXPORT ${target_name}Targets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${target_name}"
        FILE "${target_name}Targets.cmake"
        NAMESPACE ${QCORO_TARGET_PREFIX}::
        COMPONENT Devel
    )
endfunction()
