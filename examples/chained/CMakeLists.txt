add_executable(chained-example main.cpp)
target_link_libraries(chained-example
    QCoroCore
    Qt${QT_VERSION_MAJOR}::Core
)
