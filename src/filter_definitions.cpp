#include "filter_definitions.hpp"
#include <iostream>
#include <algorithm>

namespace FilterDefinitions
{

    std::vector<FilterInfo> getAllFilters()
    {
        std::vector<FilterInfo> filters;

        // 标准HDF5过滤器
        filters.push_back({H5Z_FILTER_DEFLATE, "GZIP", "DEFLATE compression algorithm (gzip)", {1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 1, true});
        filters.push_back({H5Z_FILTER_SHUFFLE, "SHUFFLE", "Byte shuffling filter", {}, 0, 0, true});
        filters.push_back({H5Z_FILTER_FLETCHER32, "FLETCHER32", "Fletcher32 checksum", {}, 0, 0, false});
        filters.push_back({H5Z_FILTER_SZIP, "SZIP", "NASA's lossless compression", {4, 8, 32}, 2, 2, true});
        filters.push_back({H5Z_FILTER_NBIT, "NBIT", "N-bit compression", {}, 0, 0, false});
        filters.push_back({H5Z_FILTER_SCALEOFFSET, "SCALEOFFSET", "Scale-offset compression", {}, 0, 0, false});

        // 第三方过滤器（基于example文件夹）
        filters.push_back({H5Z_FILTER_BITGROOM, "BITGROOM", "BitGrooming for floating-point data", {1, 2, 3}, 1, 1, true});
        filters.push_back({H5Z_FILTER_BLOSC, "BLOSC", "Blosc meta-compressor", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 4, 4, true});
        filters.push_back({H5Z_FILTER_BLOSC2, "BLOSC2", "Blosc2 meta-compressor", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 4, 4, true});
        filters.push_back({H5Z_FILTER_BSHUF, "BSHUF", "Bitshuffle filter", {0, 1, 2}, 1, 1, true});
        filters.push_back({H5Z_FILTER_BZIP2, "BZIP2", "Bzip2 compression", {1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 1, true});
        filters.push_back({H5Z_FILTER_GRANULARBR, "GRANULARBR", "Granular Bit Rounding", {1, 2, 3}, 1, 1, true});
        filters.push_back({H5Z_FILTER_LZ4, "LZ4", "LZ4 fast compression", {1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 1, true});
        filters.push_back({H5Z_FILTER_LZF, "LZF", "LZF compression", {1, 2, 3}, 1, 1, true});
        filters.push_back({H5Z_FILTER_ZFP, "ZFP", "ZFP floating-point compression", {1, 2, 3, 4}, 4, 4, true});
        filters.push_back({H5Z_FILTER_ZSTD, "ZSTD", "Zstandard compression", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}, 1, 1, true});
        filters.push_back({H5Z_FILTER_VBZ, "VBZ", "Nanopore VBZ compression for signal data", {1}, 4, 4, true});

        return filters;
    }

    FilterInfo getFilterInfo(const std::string &filter_name)
    {
        auto filters = getAllFilters();
        for (const auto &filter : filters)
        {
            if (filter.name == filter_name)
            {
                return filter;
            }
        }

        // 返回默认的未知过滤器
        return {-1, filter_name, "Unknown filter", {}, 0, 0, false};
    }

    FilterInfo getFilterInfo(int filter_id)
    {
        auto filters = getAllFilters();
        for (const auto &filter : filters)
        {
            if (filter.filter_id == filter_id)
            {
                return filter;
            }
        }

        // 返回默认的未知过滤器
        return {filter_id, "UNKNOWN", "Unknown filter", {}, 0, 0, false};
    }

    std::vector<unsigned int> getDefaultFilterParams(int filter_id, int compression_level)
    {
        std::vector<unsigned int> params;

        // 基于example文件夹中的示例程序设置默认参数
        switch (filter_id)
        {
        case H5Z_FILTER_DEFLATE:
            params.push_back(compression_level > 0 ? compression_level : 6);
            break;
        case H5Z_FILTER_BZIP2:
            params.push_back(compression_level > 0 ? compression_level : 2);
            break;
        case H5Z_FILTER_LZ4:
            // LZ4参数：块大小（以字节为单位）
            params.push_back(65536); // 默认64KB块
            break;
        case H5Z_FILTER_ZSTD:
            params.push_back(compression_level > 0 ? compression_level : 3);
            break;
        case H5Z_FILTER_BLOSC:
            // BLOSC参数：压缩级别、压缩器类型、块大小、类型大小
            params.push_back(compression_level > 0 ? compression_level : 5);
            params.push_back(1); // blosclz压缩器
            params.push_back(0); // 自动块大小
            params.push_back(4); // 类型大小（假设为4字节整数）
            break;
        case H5Z_FILTER_BLOSC2:
            params.push_back(compression_level > 0 ? compression_level : 5);
            params.push_back(1); // blosclz压缩器
            params.push_back(0); // 自动块大小
            params.push_back(4); // 类型大小
            break;
        case H5Z_FILTER_SZIP:
            params.push_back(4);  // 编码选项
            params.push_back(32); // 像素每块
            break;
        case H5Z_FILTER_ZFP:
            // ZFP参数：精度、准确度、速率、可逆
            params.push_back(32); // 精度模式
            params.push_back(0);  // 精度值
            params.push_back(0);  // 准确度
            params.push_back(0);  // 速率
            break;
        case H5Z_FILTER_VBZ:
            // VBZ参数：版本、整数大小、使用delta zigzag压缩、ZSTD压缩级别
            params.push_back(1);                                             // 版本
            params.push_back(2);                                             // 整数大小（2字节）
            params.push_back(1);                                             // 使用delta zigzag压缩
            params.push_back(compression_level > 0 ? compression_level : 3); // ZSTD压缩级别
            break;
        default:
            // 对于其他过滤器，使用默认参数
            if (compression_level > 0)
            {
                params.push_back(compression_level);
            }
            break;
        }

        return params;
    }

    std::string getFilterParamsDescription(int filter_id, const std::vector<unsigned int> &params)
    {
        std::string description;

        switch (filter_id)
        {
        case H5Z_FILTER_DEFLATE:
            if (!params.empty())
            {
                description = "Compression level: " + std::to_string(params[0]);
            }
            break;
        case H5Z_FILTER_BZIP2:
            if (!params.empty())
            {
                description = "Compression level: " + std::to_string(params[0]);
            }
            break;
        case H5Z_FILTER_LZ4:
            if (!params.empty())
            {
                description = "Block size: " + std::to_string(params[0]) + " bytes";
            }
            break;
        case H5Z_FILTER_ZSTD:
            if (!params.empty())
            {
                description = "Compression level: " + std::to_string(params[0]);
            }
            break;
        case H5Z_FILTER_BLOSC:
            if (params.size() >= 4)
            {
                description = "Level: " + std::to_string(params[0]) +
                              ", Compressor: " + std::to_string(params[1]) +
                              ", Block size: " + (params[2] == 0 ? "auto" : std::to_string(params[2])) +
                              ", Type size: " + std::to_string(params[3]);
            }
            break;
        case H5Z_FILTER_SZIP:
            if (params.size() >= 2)
            {
                description = "Encoding: " + std::to_string(params[0]) +
                              ", Pixels per block: " + std::to_string(params[1]);
            }
            break;
        case H5Z_FILTER_ZFP:
            if (params.size() >= 4)
            {
                description = "Mode: " + std::to_string(params[0]) +
                              ", Precision: " + std::to_string(params[1]) +
                              ", Accuracy: " + std::to_string(params[2]) +
                              ", Rate: " + std::to_string(params[3]);
            }
            break;
        default:
            if (!params.empty())
            {
                description = "Parameters: ";
                for (size_t i = 0; i < params.size(); ++i)
                {
                    if (i > 0)
                        description += ", ";
                    description += std::to_string(params[i]);
                }
            }
            break;
        }

        return description.empty() ? "Default parameters" : description;
    }

} // namespace FilterDefinitions
