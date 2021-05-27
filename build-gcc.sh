SOURCE_DIR=$(dirname $(realpath $0))

cmake \
    -DCMAKE_INSTALL_PREFIX=$(pwd) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS=-s \
    $SOURCE_DIR || exit
make && make install || exit
