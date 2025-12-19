# 使用Ubuntu作为基础镜像
FROM ubuntu:22.04

RUN echo 0 > /proc/sys/kernel/yama/ptrace_scope || true

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV HDF5_VERSION=1.10.11
ENV VBZ_VERSION=1.0.1
ENV BLOSC2_VERSION=2.10.0

# 安装基础依赖（包括Conan用于VBZ插件构建）
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libtool \
    automake \
    autoconf \
    zlib1g-dev \
    libssl-dev \
    python3 \
    python3-pip \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/* && \
    pip3 install conan

#1.10.7
#RUN apt-get update && apt-get install -y libhdf5-dev=2.0.0 libhdf5-serial-dev         
# https://github.com/HDFGroup/hdf5_plugins/blob/master/docs/RegisteredFilterPlugins.md
# 安装HDF5（不删除源代码，供hdf5_plugins使用）
RUN cd /tmp && \
    wget https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-${HDF5_VERSION%.*}/hdf5-$HDF5_VERSION/src/hdf5-$HDF5_VERSION.tar.gz && \
    tar -xzf hdf5-$HDF5_VERSION.tar.gz && \
    cd hdf5-$HDF5_VERSION && \
    ./configure --prefix=/usr/local --enable-cxx --enable-hl --enable-shared && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -r /tmp/hdf5*
# 注意：不删除/tmp/hdf5*，因为hdf5_plugins需要HDF5源代码

# 安装Blosc2
#RUN cd /tmp && \
# wget https://github.com/Blosc/c-blosc2/archive/refs/tags/v${BLOSC2_VERSION}.tar.gz -O blosc2.tar.gz && \
#tar -xzf blosc2.tar.gz && \
#cd c-blosc2-${BLOSC2_VERSION} && \
#mkdir build && cd build && \
#cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
#make -j$(nproc) && \
#make install && \
#ldconfig && \
#rm -rf /tmp/c-blosc2*

# 安装其他压缩库
#RUN apt-get update && apt-get install -y \
#    liblz4-dev \
#    libzstd-dev \
#    libsnappy-dev \
#    libbz2-dev \
#    && rm -rf /var/lib/apt/lists/*

#关于LZ4插件，在1.10.11版本中，只能在window平台下使用，linux平台直接被禁用了，但是在2.0.0版本中，已经支持了linux平台
#关于BLOSC2插件，在1.10.11版本中，未支持，但是在2.0.0版本中，已经支持
RUN cd /tmp && \
    wget https://github.com/HDFGroup/hdf5_plugins/archive/refs/tags/1.10.11.tar.gz -O hdf5_plugins.tar.gz && \
    tar -xzf hdf5_plugins.tar.gz && \
    cd hdf5_plugins-1.10.11 && \
    mkdir build && cd build && \
    #HDF5_ROOT=/usr/lib/x86_64-linux-gnu/hdf5/serial cmake \
    #HDF5_ROOT=/usr/local/lib/hdf5 cmake \
    cmake -C ../config/cmake/cacheinit.cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    #-DCMAKE_INSTALL_PREFIX:PATH=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib/plugins \
    -DUSE_SHARED_LIBS:BOOL=ON \
    -DBUILD_SHARED_LIBS:BOOL=ON \
    -DHDF5_LIBRARIES=/usr/local/include \
    -DHDF5_INCLUDE_DIRS=/usr/local/lib \
    #-DH5PL_TGZPATH:PATH=/tmp/hdf5_plugins-1.10.11/libs \
    -DTGZPATH:PATH=/tmp/hdf5_plugins-1.10.11/libs \
    -DH5PL_ALLOW_EXTERNAL_SUPPORT:STRING=TGZ \
    #-DH5PL_INSTALL_DOC_DIR:PATH=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib/docs \
    #-DH5PL_BUILD_EXAMPLES:BOOL=OFF \
    -DH5PL_BUILD_TESTING:BOOL=OFF \
    -DBUILD_EXAMPLES:BOOL=OFF \
    .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/hdf5_plugins*

#dpkg -l | grep libvbz-hdf-plugin       v1.0.2
RUN apt-get update && apt-get install -y libvbz-hdf-plugin-dev
#RUN cd /tmp && \
#  wget https://github.com/nanoporetech/vbz_compression/archive/refs/tags/1.0.13.tar.gz -O vbz_compression.tar.gz && \
# tar -xzf vbz_compression.tar.gz && \
# cd vbz_compression-1.0.13 && \
# mkdir build && cd build && \
# cmake .. && \
# make -j$(nproc) && \
# make install && \
# ldconfig && \
# rm -rf /tmp/vbz_compression*

# 设置HDF5插件环境变量
ENV HDF5_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu:/usr/local/HDF_Group/h5pl/1.10.11/lib/plugin
ENV LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu
ENV LD_LIBRARY_PATH=/usr/local/HDF_Group/h5pl/1.10.11/lib/plugin:$LD_LIBRARY_PATH
ENV LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.8
ENV ASAN_OPTIONS="detect_leaks=1:halt_on_error=0"

# 设置工作目录
WORKDIR /workspace

# 复制项目文件
COPY . .

# 构建项目
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-fsanitize=address" \
    -DCMAKE_CXX_FLAGS="-fsanitize=address" &&\
    make -j$(nproc)

# 设置默认命令
CMD ["./build/bin/hdf5_compression_bench"]
