macro(qcoro_enable_coroutines)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") # GCC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        if (MSVC)
            # clang-cl behaves like MSVC and enables coroutines automatically when C++20 is enabled
        else()
            if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts")
            endif()
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # MSVC auto-enables coroutine support when C++20 is enabled
    else()
        message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not currently supported.")
    endif()
endmacro()
