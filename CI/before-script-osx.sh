# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH

mkdir build
cd build
~/Work/libwebrtc/cmake-3.2.2/bin/cmake \
-DDepsPath=/tmp/obsdeps \
-DVLCPath=$PWD/../../vlc-master ..
-DCMAKE_INSTALL_PREFIX=/opt/obs \
-DCMAKE_BUILD_TYPE=RelWithDebInfo ..