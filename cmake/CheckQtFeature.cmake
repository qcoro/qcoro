include(CheckCXXSourceCompiles)

function(check_qt_feature feature module result)
    string(TOLOWER ${module} _module_lc)
    set(_source_code
        "#include <qglobal.h>
         #include <qt${_module_lc}-config.h>
         int main() {
             QT_REQUIRE_CONFIG(${feature});
         }")
    set(CMAKE_REQUIRED_LIBRARIES Qt${QT_VERSION_MAJOR}::${module})
    check_cxx_source_compiles("${_source_code}" ${result})
    set(${result} ${${result}} PARENT_SCOPE)
endfunction()
