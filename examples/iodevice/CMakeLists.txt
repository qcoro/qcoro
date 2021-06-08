add_executable(iodevice-example main.cpp)
target_link_libraries(iodevice-example QCoro Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)
