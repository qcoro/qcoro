add_executable(future-example main.cpp)
target_link_libraries(future-example QCoro Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Concurrent)
