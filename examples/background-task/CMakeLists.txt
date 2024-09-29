add_executable(background-task-example)
target_sources(background-task-example PRIVATE main.cpp)
target_link_libraries(background-task-example
    PRIVATE
    QCoro${QT_VERSION_MAJOR}Core
    Qt${QT_VERSION_MAJOR}::Core
)