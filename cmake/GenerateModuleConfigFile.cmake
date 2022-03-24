include(CMakePackageConfigHelpers)

function(generate_cmake_module_config_file)
    function(process_dependencies)
        set(oneValueArgs PREFIX OUTPUT)
        set(multiValueArgs LIBRARIES)
        cmake_parse_arguments(deps "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        set(_deps)
        set(_deps_private FALSE)
        foreach(dep ${deps_LIBRARIES})
            if ("${dep}" MATCHES "PUBLIC|INTERFACE")
                set(_deps_private FALSE)
                continue()
            elseif ("${dep}" STREQUAL "PRIVATE")
                set(_deps_private TRUE)
                continue()
            endif()

            if (NOT _deps_private)
                set(_deps "${_deps}find_dependency(${deps_PREFIX}${dep})\n")
            endif()
        endforeach()

        set(${deps_OUTPUT} ${_deps} PARENT_SCOPE)
    endfunction()

    set(options)
    set(oneValueArgs TARGET_NAME NAME)
    set(multiValueArgs QT_DEPENDENCIES QCORO_DEPENDENCIES)
    cmake_parse_arguments(cmc "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    process_dependencies(
        PREFIX "Qt${QT_VERSION_MAJOR}"
        LIBRARIES ${cmc_QT_DEPENDENCIES}
        OUTPUT QT_DEPENDENCIES
    )

    process_dependencies(
        PREFIX "QCoro${QT_VERSION_MAJOR}"
        LIBRARIES ${cmc_QCORO_DEPENDENCIES}
        OUTPUT QCORO_DEPENDENCIES
    )

    set(MODULE_NAME "${cmc_NAME}")
    configure_package_config_file(
        "${qcoro_SOURCE_DIR}/cmake/QCoroModuleConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${cmc_TARGET_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${cmc_TARGET_NAME}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${cmc_TARGET_NAME}ConfigVersion.cmake"
        VERSION ${qcoro_VERSION}
        COMPATIBILITY SameMajorVersion
    )
endfunction()
