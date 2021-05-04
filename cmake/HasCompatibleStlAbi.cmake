# This check verifies that Qt is compiled with the same standard library
# as the one being used for QCoro, i.e. both Qt and QCoro are compiled
# with libstdc++ or libc++. Mixing libstdc++-built Qt with libc++-build
# QCoro may cause issues.
#
# So far I'm aware of only one issue in Qt6, which publicly exposes
# std::exception_ptr in its API in QUnhandledException. std::exception_ptr
# is an alias to std::__exception_ptr::exception_ptr on libstdc++,
# so building QCoro with libc++ against libstdc++ Qt ends up in linking
# error, because QUnhandledException::QUnhandledException(std::exception_ptr)
# doesn't exist in the Qt binaries.

include(CheckSourceCompiles)

function(has_compatible_stl_abi result)
    message(CHECK_START "Checking stdlib compatibility with Qt")
    set(CMAKE_REQUIRED_LIBRARIES Qt${QT_VERSION_MAJOR}::Core)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_source_compiles(CXX
        "#include <QUnhandledException>
         #include <exception>
         int main() {
             [[maybe_unused]] auto e = QUnhandledException(std::exception_ptr{});
         }"
         QUNHANDLEDEXCPTION_LINKS
    )
    if (NOT QUNHANDLEDEXCPTION_LINKS)
        message(CHECK_FAIL "std::exception_ptr mismatch")
        set(${result} FALSE PARENT_SCOPE)
        return()
    endif()
    message(CHECK_PASS "Success")
    set(${result} TRUE PARENT_SCOPE)
endfunction()
