add_executable(network-example main.cpp)
target_link_libraries(network-example
    QCoro${QT_VERSION_MAJOR}Network
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)
