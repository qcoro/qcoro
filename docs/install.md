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

## Add it to your CMake

```cmake
find_package(QCoro REQUIRED)

# Set necessary compiler flags to enable coroutine support
qcoro_enable_coroutines()

...

target_link_libraries(your-target QCoro::QCoro)
```


