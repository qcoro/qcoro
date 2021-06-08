add_executable(await-sync-string await-sync-string.cpp)
target_link_libraries(await-sync-string QCoro)

add_executable(await-async-string await-async-string.cpp)
target_link_libraries(await-async-string QCoro Qt${QT_VERSION_MAJOR}::Core)
