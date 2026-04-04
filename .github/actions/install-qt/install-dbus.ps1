# Install dbus from MSYS2 (pre-installed on GitHub Actions Windows runners)
C:\msys64\usr\bin\pacman.exe -S --noconfirm mingw-w64-x86_64-dbus

# Copy the dbus binaries and libraries into the Qt installation directory
# so they're on PATH (Qt's bin/ is already added to GITHUB_PATH)
$qtBaseDir = $args[0]
$msys2DbusDir = "C:\msys64\mingw64"

Copy-Item "$msys2DbusDir\bin\dbus-daemon.exe" "$qtBaseDir\bin\" -Force
Copy-Item "$msys2DbusDir\bin\dbus-1-3.dll" "$qtBaseDir\bin\" -Force
# Copy the session.conf and system.conf needed by dbus-daemon
New-Item -ItemType Directory -Path "$qtBaseDir\share\dbus-1" -Force | Out-Null
Copy-Item "$msys2DbusDir\share\dbus-1\session.conf" "$qtBaseDir\share\dbus-1\" -Force
Copy-Item "$msys2DbusDir\share\dbus-1\system.conf" "$qtBaseDir\share\dbus-1\" -Force
