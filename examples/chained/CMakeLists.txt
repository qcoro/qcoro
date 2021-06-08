add_executable(chained-example main.cpp)
target_link_libraries(chained-example QCoro Qt${QT_VERSION_MAJOR}::Core)
