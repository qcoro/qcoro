add_executable(timer-example main.cpp)
target_link_libraries(timer-example
    QCoro${QT_VERSION_MAJOR}Core
    Qt${QT_VERSION_MAJOR}::Core
)
