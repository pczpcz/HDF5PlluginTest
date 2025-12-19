# 添加可执行文件
message(STATUS "Creating executable: hdf5_compression_bench")
message(STATUS "Source files: main.cpp, hdf5_processor.cpp, compression_tester.cpp, utils.cpp, filter_definitions.cpp")
add_executable(hdf5_compression_bench
  main.cpp
  hdf5_processor.cpp
  compression_tester.cpp
  utils.cpp
  filter_definitions.cpp
)

# 链接库
message(STATUS "Linking libraries for hdf5_compression_bench:")
message(STATUS "  HDF5_LIBRARIES: ${HDF5_LIBRARIES}")
message(STATUS "  HDF5_HL_LIBRARIES: ${HDF5_HL_LIBRARIES}")
message(STATUS "  ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")
target_link_libraries(hdf5_compression_bench
  ${HDF5_LIBRARIES}
  ${HDF5_HL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  Threads::Threads
)

# 添加可选的压缩库
message(STATUS "Adding optional compression libraries:")
if(LZ4_FOUND)
  message(STATUS "  Adding LZ4: ${LZ4_LIBRARIES}")
  target_link_libraries(hdf5_compression_bench ${LZ4_LIBRARIES})
  target_compile_definitions(hdf5_compression_bench PRIVATE HAVE_LZ4)
else()
  message(STATUS "  LZ4 not available")
endif()

if(Zstd_FOUND)
  message(STATUS "  Adding Zstd: ${Zstd_LIBRARIES}")
  target_link_libraries(hdf5_compression_bench ${Zstd_LIBRARIES})
  target_compile_definitions(hdf5_compression_bench PRIVATE HAVE_ZSTD)
else()
  message(STATUS "  Zstd not available")
endif()

if(Snappy_FOUND)
  message(STATUS "  Adding Snappy: ${Snappy_LIBRARIES}")
  target_link_libraries(hdf5_compression_bench ${Snappy_LIBRARIES})
  target_compile_definitions(hdf5_compression_bench PRIVATE HAVE_SNAPPY)
else()
  message(STATUS "  Snappy not available")
endif()

if(BZIP2_FOUND)
  message(STATUS "  Adding BZip2: ${BZIP2_LIBRARIES}")
  target_link_libraries(hdf5_compression_bench ${BZIP2_LIBRARIES})
  target_compile_definitions(hdf5_compression_bench PRIVATE HAVE_BZIP2)
else()
  message(STATUS "  BZip2 not available")
endif()

# 设置目标属性
set_target_properties(hdf5_compression_bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装目标
install(TARGETS hdf5_compression_bench
  RUNTIME DESTINATION bin
)
