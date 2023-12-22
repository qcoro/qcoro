# SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

cmake_policy(SET CMP0140 NEW)

function(detectAndroidNDKVersion outVar)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
        # CMAKE_ANDROID_NDK_VERSION introduced in CMake 3.20
        set(${outVar} "${CMAKE_ANDROID_NDK_VERSION}")
    else()
        if (NOT CMAKE_ANDROID_NDK)
            message(STATUS "Couldn't detect Android NDK version: CMAKE_ANDROID_NDK not set")
            return()
        endif()
        if (NOT EXISTS "${CMAKE_ANDROID_NDK}/source.properties")
            message(STATUS "Couldn't detect Android NDK version: ${CMAKE_ANDROID_NDK}/source.properties doesn't exist")
            return()
        endif()
        file(STRINGS "${CMAKE_ANDROID_NDK}/source.properties" _sources REGEX "^Pkg\.Revision = [0-9]+\.[0-9]+\.[0-9]+$")
        string(REGEX MATCH "= ([0-9]+\.[0-9]+)\." _match "${_sources}")
        set(${outVar} "${CMAKE_MATCH_1}")
        if (NOT ${outVar})
            message(STATUS "Couldn't detect Android NDK version: ${CMAKE_ANDROID_NDK}/source.properties doesn't contain Pkg.Revision")
            return()
        endif()

    endif()

    message(STATUS "Detected Android NDK version ${${outVar}}")
    return(PROPAGATE ${outVar})
endfunction()
