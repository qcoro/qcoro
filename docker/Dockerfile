# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
# SPDX-License-Identifier: MIT
#
# Docker image for specific versions of clang or gcc and specific version of Qt.

ARG compiler_version
ARG compiler_image
FROM ${compiler_image}:${compiler_version}
ARG qt_version
ARG qt_modules
ARG qt_archives

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        cmake python3-pip python3-setuptools python3-wheel python3-dev \
        dbus dbus-x11 \
        libgl-dev \
    && apt-get clean

RUN pip3 install aqtinstall

WORKDIR /root

COPY install-qt.sh ./install-qt.sh

RUN ./install-qt.sh "${qt_version}" "${qt_modules}" "${qt_archives}"

ENV QT_BASE_DIR "/opt/qt/${qt_version}/gcc_64"
ENV PATH "${QT_BASE_DIR}/bin:${PATH}"
ENV CMAKE_PREFIX_PATH="${QT_BASE_DIR}/lib/cmake"
ENV LD_LIBRARY_PATH "${QT_BASE_DIR}/lib:${LD_LIBRARY_PATH}"
ENV XDG_DATA_DIRS "${QT_BASE_DIR}/share:${XDG_DATA_DIRS}"
