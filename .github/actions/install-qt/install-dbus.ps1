[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Install-PackageProvider -Name NuGet -MinimumVersion 2.8.5.201 -Force
Set-PSRepository -Name 'PSGallery' -SourceLocation "https://www.powershellgallery.com/api/v2" -InstallationPolicy Trusted
Install-Module -Name 7Zip4PowerShell -Force

curl https://files.kde.org/craft/master/Qt_5.15.2-1/windows/msvc2019_64/cl/RelWithDebInfo/libs/dbus/dbus-1.13.18-3-131-20210415T121131-windows-msvc2019_64-cl.7z -o C:\Qt\dbus.7z
Expand-7Zip -ArchiveFileName C:\Qt\dbus.7z -TargetPath $args[0]

