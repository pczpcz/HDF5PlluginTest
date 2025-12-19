#ifndef FILTER_DEFINITIONS_HPP
#define FILTER_DEFINITIONS_HPP

#include <string>
#include <vector>
#include <map>
#include <hdf5.h>

// 过滤器ID定义（基于example文件夹中的示例程序）
#define H5Z_FILTER_BITGROOM 32018
#define H5Z_FILTER_BLOSC 32001
#define H5Z_FILTER_BLOSC2 32026
#define H5Z_FILTER_BSHUF 32008
#define H5Z_FILTER_BZIP2 307
#define H5Z_FILTER_GRANULARBR 32019
#define H5Z_FILTER_LZ4 32004
#define H5Z_FILTER_LZF 32005
#define H5Z_FILTER_ZFP 32013
#define H5Z_FILTER_ZSTD 32015
#define H5Z_FILTER_VBZ 32020

// 标准HDF5过滤器ID
#define H5Z_FILTER_DEFLATE 1
#define H5Z_FILTER_SHUFFLE 2
#define H5Z_FILTER_FLETCHER32 3
#define H5Z_FILTER_SZIP 4
#define H5Z_FILTER_NBIT 5
#define H5Z_FILTER_SCALEOFFSET 6

namespace FilterDefinitions
{

    // 过滤器信息结构
    struct FilterInfo
    {
        int filter_id;
        std::string name;
        std::string description;
        std::vector<int> supported_levels;
        size_t min_params;
        size_t max_params;
        bool requires_chunking;
    };

    // 获取所有可用过滤器的信息
    std::vector<FilterInfo> getAllFilters();

    // 根据过滤器名称获取过滤器信息
    FilterInfo getFilterInfo(const std::string &filter_name);

    // 根据过滤器ID获取过滤器信息
    FilterInfo getFilterInfo(int filter_id);

    // 获取过滤器的默认参数
    std::vector<unsigned int> getDefaultFilterParams(int filter_id, int compression_level = -1);

    // 获取过滤器的参数描述
    std::string getFilterParamsDescription(int filter_id, const std::vector<unsigned int> &params);

    // 支持的过滤器名称列表
    const std::vector<std::string> SUPPORTED_FILTERS = {
        "GZIP",        // H5Z_FILTER_DEFLATE
        "SHUFFLE",     // H5Z_FILTER_SHUFFLE
        "FLETCHER32",  // H5Z_FILTER_FLETCHER32
        "SZIP",        // H5Z_FILTER_SZIP
        "NBIT",        // H5Z_FILTER_NBIT
        "SCALEOFFSET", // H5Z_FILTER_SCALEOFFSET
        "BITGROOM",    // H5Z_FILTER_BITGROOM
        "BLOSC",       // H5Z_FILTER_BLOSC
        "BLOSC2",      // H5Z_FILTER_BLOSC2
        "BSHUF",       // H5Z_FILTER_BSHUF
        "BZIP2",       // H5Z_FILTER_BZIP2
        "GRANULARBR",  // H5Z_FILTER_GRANULARBR
        "LZ4",         // H5Z_FILTER_LZ4
        "LZF",         // H5Z_FILTER_LZF
        "ZFP",         // H5Z_FILTER_ZFP
        "ZSTD",        // H5Z_FILTER_ZSTD
        "VBZ"          // H5Z_FILTER_VBZ
    };

} // namespace FilterDefinitions

#endif // FILTER_DEFINITIONS_HPP
