add_executable(await-sync-string await-sync-string.cpp)
target_link_libraries(await-sync-string QCoro${QT_VERSION_MAJOR})

add_executable(await-async-string await-async-string.cpp)
target_link_libraries(await-async-string QCoro${QT_VERSION_MAJOR} Qt${QT_VERSION_MAJOR}::Core)
