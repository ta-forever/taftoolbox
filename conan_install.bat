conan install . -s=build_type=Debug -s=arch=x86 -o=qt/*:shared=True --build=missing
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build\generators\conan_toolchain.cmake -A x86
