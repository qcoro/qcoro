add_library(qcoroasyncsqlpsql MODULE)
target_sources(qcoroasyncsqlpsql PRIVATE
    main.cpp
    psql_driver.cpp
    psql_result.cpp
    psql_types.cpp
    utils.cpp
)

ecm_qt_declare_logging_category(qcoroasyncsqlpsql
    HEADER psql_log.h
    IDENTIFIER AsyncPsqlLog
    CATEGORY_NAME cz.dvratil.qcoro.asyncsql.psql
    DESCRIPTION "QCoro AsyncSql PSQL driver"
    EXPORT QCORO
)

target_include_directories(qcoroasyncsqlpsql PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(qcoroasyncsqlpsql PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    ${QCORO_TARGET_PREFIX}Core
    ${QCORO_TARGET_PREFIX}Network
    ${QCORO_TARGET_PREFIX}AsyncSql
    PostgreSQL::PostgreSQL
)

set_target_properties(qcoroasyncsqlpsql PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/qcoro/asyncsqldrivers"
)
