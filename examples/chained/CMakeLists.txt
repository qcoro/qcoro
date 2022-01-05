add_executable(chained-example main.cpp)
target_link_libraries(chained-example
    QCoro${QT_VERSION_MAJOR}Core
    Qt${QT_VERSION_MAJOR}::Core
)
