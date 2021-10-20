set(io_SRCS
    ioengine.cpp)

add_library(QCoroIO ${io_SRCS})
target_link_libraries(QCoroIO
    PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
)

if ("${QCORO_IO_BACKEND}" STREQUAL "iouring")
    add_subdirectory(io_uring)
    target_link_libraries(QCoroIO PRIVATE qcoroio_iouring)
endif()


