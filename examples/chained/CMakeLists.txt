add_executable(chained-example main.cpp)
target_link_libraries(chained-example qcoro Qt${QT_VERSION_MAJOR}::Core)
