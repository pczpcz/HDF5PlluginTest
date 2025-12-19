cmake_minimum_required(VERSION 3.15)
project(hdf5_compression_bench VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找HDF5
message(STATUS "Looking for HDF5 with CXX and HL components...")
find_package(HDF5 REQUIRED COMPONENTS CXX HL)
if(HDF5_FOUND)
  message(STATUS "HDF5 found: ${HDF5_VERSION}")
  message(STATUS "HDF5 include dirs: ${HDF5_INCLUDE_DIRS}")
  message(STATUS "HDF5 libraries: ${HDF5_LIBRARIES}")
  message(STATUS "HDF5 CXX libraries: ${HDF5_CXX_LIBRARIES}")
  message(STATUS "HDF5 HL libraries: ${HDF5_HL_LIBRARIES}")
else()
  message(FATAL_ERROR "HDF5 not found! Please install libhdf5-dev and libhdf5-cpp-dev")
endif()

# 查找其他依赖
message(STATUS "Looking for ZLIB...")
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
  message(STATUS "ZLIB found: ${ZLIB_VERSION}")
  message(STATUS "ZLIB include dirs: ${ZLIB_INCLUDE_DIRS}")
  message(STATUS "ZLIB libraries: ${ZLIB_LIBRARIES}")
else()
  message(FATAL_ERROR "ZLIB not found!")
endif()

find_package(Threads REQUIRED)
message(STATUS "Threads library: ${CMAKE_THREAD_LIBS_INIT}")

# 查找可选的压缩库
message(STATUS "Looking for optional compression libraries...")
find_package(LZ4 QUIET)
if(LZ4_FOUND)
  message(STATUS "LZ4 found: ${LZ4_VERSION}")
  message(STATUS "LZ4 include dirs: ${LZ4_INCLUDE_DIRS}")
  message(STATUS "LZ4 libraries: ${LZ4_LIBRARIES}")
else()
  message(STATUS "LZ4 not found (optional)")
endif()

find_package(Zstd QUIET)
if(Zstd_FOUND)
  message(STATUS "Zstd found: ${Zstd_VERSION}")
  message(STATUS "Zstd include dirs: ${Zstd_INCLUDE_DIRS}")
  message(STATUS "Zstd libraries: ${Zstd_LIBRARIES}")
else()
  message(STATUS "Zstd not found (optional)")
endif()

find_package(Snappy QUIET)
if(Snappy_FOUND)
  message(STATUS "Snappy found: ${Snappy_VERSION}")
  message(STATUS "Snappy include dirs: ${Snappy_INCLUDE_DIRS}")
  message(STATUS "Snappy libraries: ${Snappy_LIBRARIES}")
else()
  message(STATUS "Snappy not found (optional)")
endif()

find_package(BZip2 QUIET)
if(BZIP2_FOUND)
  message(STATUS "BZip2 found: ${BZIP2_VERSION}")
  message(STATUS "BZip2 include dirs: ${BZIP2_INCLUDE_DIRS}")
  message(STATUS "BZip2 libraries: ${BZIP2_LIBRARIES}")
else()
  message(STATUS "BZip2 not found (optional)")
endif()

# 设置项目包含目录
include_directories(${HDF5_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)

# 添加子目录
add_subdirectory(src)
#add_subdirectory(tests)

# 安装目标
install(TARGETS hdf5_compression_bench
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# 安装配置文件
#install(FILES README.md DESTINATION share/doc/hdf5_compression_bench)
#install(DIRECTORY docs/ DESTINATION share/doc/hdf5_compression_bench/docs)

# 启用测试
#enable_testing()
