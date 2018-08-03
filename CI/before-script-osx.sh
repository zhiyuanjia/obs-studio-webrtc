# Make sure ccache is found
# export PATH=/usr/local/opt/ccache/libexec:$PATH

mkdir build
cd build
cmake \
-DDepsPath=/tmp/obsdeps \
-DVLCPath=$PWD/../../vlc-master ..
-DCMAKE_INSTALL_PREFIX=/opt/obs \
-DCMAKE_BUILD_TYPE=RelWithDebInfo ..