import json
from socket import create_connection

qt = [
    {
        "version": "5.15.2",
        "archives": [ "qtbase", "icu", "qtwebsockets" ],
        "modules": []
    },
    {
        "version": "6.2.0",
        "archives": [ "qtbase", "icu" ],
        "modules": [ "qtwebsockets" ]
    }
]

platforms = [
    {
        "name": "windows",
        "compilers": [{ "name": "msvc" }]
    },
    {
        "name": "macos",
        "compilers": [{ "name": "apple-clang" }]
    },
    {
        "name": "linux",
        "compilers": [
            {
                "name": "gcc",
                "versions": [ "10.3.0", "11.3.0" ]
            },
            {
                "name": "clang",
                "versions": [ "11", "14", "15" ]
            }
        ]
    }
]


output = {
    "include": []
}


def get_os_for_platform(platform):
    if platform == "windows":
        return "windows-2022"
    if platform == "linux":
        return "ubuntu-20.04"
    if platform == "macos":
        return "macos-11"
    raise RuntimeError(f"Invalid platform '{platform}'.")


def create_configuration(qt, platform, compiler, compiler_version = ""):
    return {
        "qt_version": qt["version"],
        "qt_modules": ' '.join(qt["modules"]),
        "qt_archives": ' '.join(qt["archives"]),
        "platform": platform,
        "compiler": compiler,
        "compiler_version": compiler_version,
        "compiler_full": compiler if not compiler_version else f"{compiler}-{compiler_version}",
        "runs_on": get_os_for_platform(platform),
        "with_qtdbus": "OFF" if platform == "macos" else "ON"
    }

for qt_version in qt:
    for platform in platforms:
        for compiler in platform["compilers"]:
            if "versions" in compiler:
                for compiler_version in compiler["versions"]:
                    output["include"].append(
                        create_configuration(qt_version, platform["name"], compiler["name"], compiler_version))
            else:
                output["include"].append(
                    create_configuration(qt_version, platform["name"], compiler["name"]))

print(json.dumps(output))
