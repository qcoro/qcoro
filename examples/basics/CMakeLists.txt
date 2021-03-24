add_executable(await-sync-string await-sync-string.cpp)
target_link_libraries(await-sync-string qcoro)

add_executable(await-async-string await-async-string.cpp)
target_link_libraries(await-async-string qcoro Qt5::Core)
