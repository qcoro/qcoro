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

SHELL ["/bin/bash", "-c"]

RUN source /etc/os-release && \
    if [[ "${VERSION_ID}" == "10" ]]; then \
        echo "deb http://deb.debian.org/debian buster-backports main" > /etc/apt/sources.list.d/backports.list; \
    fi

RUN apt-get update \
    && apt-get upgrade --yes \
    && apt-get install --yes --no-install-recommends \
        cmake=3.18.4\* cmake-data=3.18.4\* \
        python3-pip python3-setuptools python3-wheel python3-dev \
        dbus dbus-x11 \
        libglib2.0-dev libxkbcommon-dev \
        libegl-dev libgl-dev libegl1=1.3.2\* libgl1=1.3.2\* libglx0=1.3.2\* libglvnd0=1.3.2\* \
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
