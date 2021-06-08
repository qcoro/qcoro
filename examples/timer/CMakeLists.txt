add_executable(timer-example main.cpp)
target_link_libraries(timer-example QCoro Qt${QT_VERSION_MAJOR}::Core)
