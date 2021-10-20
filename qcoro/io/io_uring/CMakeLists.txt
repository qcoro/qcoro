set(iouring_SRCS
    iouring_engine.cpp
)

add_library(qcoroio_iouring STATIC ${iouring_SRCS})
target_link_libraries(qcoroio_iouring
    PRIVATE
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
    Liburing::Liburing
)
