import json
from socket import create_connection
from argparse import ArgumentParser

qt = [
    {
        "version": "5.15.2",
        "archives": [ "qtbase", "icu", "qtwebsockets", "qtdeclarative" ],
        "modules": [ "qtwebengine" ]
    },
    {
        "version": "6.2.0",
        "archives": [ "qtbase", "icu", "qtdeclarative" ],
        "modules": [ "qtwebsockets", "qtwebengine" ]
    }
]

platforms = [
    {
        "name": "windows",
        "compilers": [
            { "name": "msvc" },
            { "name": "clang-cl" }
        ]
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
                "versions": [ "10", "11", "12" ]
            },
            {
                "name": "clang",
                "versions": [ "11", "14", "dev" ]
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

def get_base_image_for_compiler(compiler):
    if compiler == "gcc":
        return "gcc"
    elif compiler == "clang":
        return "silkeh/clang"
    else:
        return None

def create_configuration(qt, platform, compiler, compiler_version = ""):
    return {
        "qt_version": qt["version"],
        "qt_modules": ' '.join(qt["modules"]),
        "qt_archives": ' '.join(qt["archives"]),
        "platform": platform,
        "compiler": compiler,
        "compiler_base_image": get_base_image_for_compiler(compiler),
        "compiler_version": compiler_version,
        "compiler_full": compiler if not compiler_version else f"{compiler}-{compiler_version}",
        "runs_on": get_os_for_platform(platform),
        "with_qtdbus": "OFF" if platform == "macos" else "ON"
    }


parser = ArgumentParser()
parser.add_argument('--platform')
args = parser.parse_args()

filtered_platforms = list(filter(lambda p: p['name'] == args.platform, platforms))

for qt_version in qt:
    for platform in filtered_platforms:
        for compiler in platform["compilers"]:
            if "versions" in compiler:
                for compiler_version in compiler["versions"]:
                    output["include"].append(
                        create_configuration(qt_version, platform["name"], compiler["name"], compiler_version))
            else:
                output["include"].append(
                    create_configuration(qt_version, platform["name"], compiler["name"]))

print(json.dumps(output))
