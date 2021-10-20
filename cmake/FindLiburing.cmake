#===============================================================
# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
#==============================================================

find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_LIBURING QUIET liburing)

find_path(LIBURING_INCLUDE_DIR NAMES liburing.h
    HINTS
    ${PC_LIBURING_INCLUDEDIR}
    ${PC_LIBURING_INCLUDE_DIRS}
)

find_library(LIBURING_LIBRARY NAMES uring
    HINTS
    ${PC_LIBURING_LIBDIR}
    ${PC_LIBURING_LIBRARY_DIRS}
)

set(LIBURING_VERSION "${PC_LIBURING_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Liburing
    REQUIRED_VARS LIBURING_LIBRARY LIBURING_INCLUDE_DIR
    VERSION_VAR LIBURING_VERSION
)
mark_as_advanced(LIBURING_INCLUDE_DIR LIBURING_LIBRARY)

if (Liburing_FOUND AND NOT TARGET Liburing::Liburing)
    add_library(Liburing::Liburing UNKNOWN IMPORTED)
    set_target_properties(Liburing::Liburing PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBURING_INCLUDE_DIR}")
    set_property(TARGET Liburing::Liburing APPEND PROPERTY IMPORTED_LOCATION "${LIBURING_LIBRARY}")
endif()

