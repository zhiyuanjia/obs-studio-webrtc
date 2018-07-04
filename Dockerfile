FROM ubuntu:xenial

RUN apt-get update && apt-get install -y build-essential pkg-config cmake git-core checkinstall \
        libx11-dev libgl1-mesa-dev libvlc-dev libpulse-dev libxcomposite-dev \
        libxinerama-dev libv4l-dev libudev-dev libfreetype6-dev \
        libfontconfig-dev qtbase5-dev libqt5x11extras5-dev libx264-dev \
        libxcb-xinerama0-dev libxcb-shm0-dev libjack-jackd2-dev libcurl4-openssl-dev \
        clang libc++-dev zlib1g-dev yasm nasm curl swig

ENV CC=/usr/bin/clang \
 CXX=/usr/bin/clang++ \
 LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib/obs-plugins:/usr/local/bin:/usr/local/lib/obs-plugins:/usr/bin/obs"

# FFMPEG
RUN git clone --depth 1 git://source.ffmpeg.org/ffmpeg.git \
    && cd ffmpeg \
    && ./configure --enable-shared --prefix=/usr \
    && make -j4 \
    && mkdir doc-pak \
    && checkinstall --pkgname=FFmpeg --fstrans=no --backup=no \
          --pkgversion="$(date +%Y%m%d)-git" --deldoc=yes

# OPENSSL
RUN curl -fSL https://www.openssl.org/source/old/1.1.0/openssl-1.1.0g.tar.gz -o openssl.tar.gz \
    && mkdir openssl \
    && tar -xzvf openssl.tar.gz -C openssl --strip-components=1 \
    && cd openssl \
    && ./config --openssldir=/usr/local/ssl \
    && make -j4 \
    && make install

# CURL
RUN curl -fSL https://curl.haxx.se/download/curl-7.60.0.tar.gz -o curl.tar.gz \
    && mkdir curl \
    && tar -xzvf curl.tar.gz -C curl --strip-components=1 \
    && apt-get remove curl -y \
    && cd curl \
    && ./configure --with-ssl=/usr/local/ssl \
    && make -j4 \
    && make install
    #  \
    # && ln -s /usr/local/bin/curl /usr/bin/curl \
    # && ln -sfn /usr/lib/x86_64-linux-gnu/libcurl.so /usr/local/lib/libcurl.so \
    # && ln -sfn /usr/lib/x86_64-linux-gnu/libcurl.so.4 /usr/local/lib/libcurl.so.4 \
    # && ln -sfn /usr/lib/x86_64-linux-gnu/libcurl.so.4.5.0 /usr/local/lib/libcurl.so.4.5.0
    # 
    # && ln -sf /usr/local/lib/libcurl.so /usr/lib/x86_64-linux-gnu/libcurl.so \
    # && ln -sf /usr/local/lib/libcurl.so.4 /usr/lib/x86_64-linux-gnu/libcurl.so.4.5.0 \
    # && ln -sf /usr/local/lib/libcurl.so.4.4.0 /usr/lib/x86_64-linux-gnu/libcurl.so.4.5.0

# LIBWEBRTC
RUN mkdir libwebrtc \
    && cd libwebrtc \
    && /usr/local/bin/curl -fSL https://cdn.sportsbooth.tv/libwebrtc/linux.sh -o libwebrtc.sh \
    && chmod a+x libwebrtc.sh \
    && ./libwebrtc.sh --skip-license --exclude-subdir \
    && cp -r cmake /usr/local \
    && cp -r include /usr/local \
    && cp -r lib /usr/local

RUN git clone --recursive https://github.com/CoSMoSoftware/OBS-studio-webrtc.git \
    && cd OBS-studio-webrtc \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make -j4 \
    && make install \
    && cpack