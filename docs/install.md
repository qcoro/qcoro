# Get Started

## Install QCoro

```shell
git clone https://github.com/danvratil/qcoro.git
cd qcoro
mkdir build
cd build
cmake ..
make
# This will install QCoro into /usr/local/ prefix, change it by passing -DCMAKE_INSTALL_PREFIX=/usr
# to the cmake command above.
sudo make install
```

Alternatively, if you don't want to install QCoro into your system, you can just clone the repository (or add 
it as a git submodule) to your project. Since QCoro is a header-only library, you just need to update your
project's include paths to pick up the `qcoro` header directory.

## Add it to your CMake

```cmake
find_package(QCoro REQUIRED)
...
target_link_libraries(your-target QCoro::qcoro)
```


