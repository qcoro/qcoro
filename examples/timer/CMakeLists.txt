add_executable(timer-example main.cpp)
target_link_libraries(timer-example
    QCoroCore
    Qt${QT_VERSION_MAJOR}::Core
)
