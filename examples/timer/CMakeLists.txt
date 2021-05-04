add_executable(timer-example main.cpp)
target_link_libraries(timer-example qcoro Qt${QT_VERSION_MAJOR}::Core)
