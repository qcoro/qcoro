[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force
Set-PSRepository -Name 'PSGallery' -SourceLocation "https://www.powershellgallery.com/api/v2" -InstallationPolicy Trusted
Install-Module -Name 7Zip4PowerShell -Force

Invoke-WebRequest -Uri https://files.kde.org/craft/master/22.05/windows/msvc2019_64/cl/RelWithDebInfo/libs/dbus/dbus-1.14.0-178-20220428T163337-windows-msvc2019_64-cl.7z -OutFile C:\Qt\dbus.7z
Expand-7Zip -ArchiveFileName C:\Qt\dbus.7z -TargetPath $args[0]

