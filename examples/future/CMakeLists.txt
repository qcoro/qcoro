add_executable(future-example main.cpp)
target_link_libraries(future-example qcoro Qt5::Core Qt5::Concurrent)
