add_executable(iodevice-example main.cpp)
target_link_libraries(iodevice-example
    QCoro${QT_VERSION_MAJOR}Core
    QCoro${QT_VERSION_MAJOR}Network
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)
