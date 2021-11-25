add_library(QCoroIO ${io_SRCS})
target_sources(QCoroIO
    PRIVATE
    ioengine.cpp
)
target_link_libraries(QCoroIO
    PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
)

if ("${QCORO_IO_BACKEND}" STREQUAL "iouring")
    target_sources(QCoroIO
        PRIVATE
        ioengine_iouring.cpp
    )
    target_link_libraries(QCoroIO PRIVATE Liburing::Liburing)
endif()

