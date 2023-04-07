macro(qcoro_find_qt)
    set(options)
    set(oneValueArgs QT_VERSION FOUND_VER_VAR)
    set(multiValueArgs COMPONENTS QT5_COMPONENTS QT6_COMPONENTS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARGS_QT_VERSION)
        find_package(Qt6Core QUIET)
        if (Qt6Core_FOUND)
            set(ARGS_QT_VERSION 6)
        else()
            set(ARGS_QT_VERSION 5)
        endif()
    endif()
    
    list(APPEND REQUIRED_QT_COMPONENTS "${ARGS_QT${ARGS_QT_VERSION}_COMPONENTS}")
    list(FILTER REQUIRED_QT_COMPONENTS EXCLUDE REGEX "Private$$")

    find_package(Qt${ARGS_QT_VERSION} REQUIRED COMPONENTS ${REQUIRED_QT_COMPONENTS})

    set(${ARGS_FOUND_VER_VAR} ${ARGS_QT_VERSION})
endmacro()
