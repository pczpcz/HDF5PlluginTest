#include "hdf5_processor.hpp"
#include "filter_definitions.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <algorithm>
#include <set>

using namespace std::chrono;

HDF5Processor::HDF5Processor()
{
    // 初始化HDF5库
    herr_t status = H5open();
    if (status < 0)
    {
        std::cerr << "Warning: Failed to initialize HDF5 library" << std::endl;
    }
}

HDF5Processor::~HDF5Processor()
{
    // 注意：不要在这里调用H5close()
    H5close(); // 应该只在程序结束时调用一次
    // 多次调用H5close()可能导致"infinite loop closing library"错误
}

// 辅助函数：将过滤器名称转换为过滤器ID
static int getFilterIdFromName(const std::string &filter_name)
{
    if (filter_name == "GZIP" || filter_name == "DEFLATE")
        return H5Z_FILTER_DEFLATE;
    if (filter_name == "SHUFFLE")
        return H5Z_FILTER_SHUFFLE;
    if (filter_name == "FLETCHER32")
        return H5Z_FILTER_FLETCHER32;
    if (filter_name == "SZIP")
        return H5Z_FILTER_SZIP;
    if (filter_name == "NBIT")
        return H5Z_FILTER_NBIT;
    if (filter_name == "SCALEOFFSET")
        return H5Z_FILTER_SCALEOFFSET;
    if (filter_name == "BITGROOM")
        return H5Z_FILTER_BITGROOM;
    if (filter_name == "BLOSC")
        return H5Z_FILTER_BLOSC;
    if (filter_name == "BLOSC2")
        return H5Z_FILTER_BLOSC2;
    if (filter_name == "BSHUF")
        return H5Z_FILTER_BSHUF;
    if (filter_name == "BZIP2")
        return H5Z_FILTER_BZIP2;
    if (filter_name == "GRANULARBR")
        return H5Z_FILTER_GRANULARBR;
    if (filter_name == "LZ4")
        return H5Z_FILTER_LZ4;
    if (filter_name == "LZF")
        return H5Z_FILTER_LZF;
    if (filter_name == "ZFP")
        return H5Z_FILTER_ZFP;
    if (filter_name == "ZSTD")
        return H5Z_FILTER_ZSTD;
    if (filter_name == "VBZ")
        return H5Z_FILTER_VBZ;
    return -1;
}

// 辅助函数：获取过滤器的默认参数  https://github.com/HDFGroup/hdf5_plugins/blob/master/docs/RegisteredFilterPlugins.md
static std::vector<unsigned int> getDefaultFilterParams(int filter_id, int compression_level)
{
    std::vector<unsigned int> params;
    int level;
    switch (filter_id)
    {
    case H5Z_FILTER_DEFLATE:
        params.push_back(compression_level);
        break;
    case H5Z_FILTER_BZIP2:
        params.push_back(compression_level > 0 && compression_level < 9 ? compression_level : 2);
        break;
    case H5Z_FILTER_LZ4:
        // LZ4参数：块大小（以字节为单位）
        if (compression_level <= 0)
        {
            params.push_back(65535);
        }
        else
        {
            params.push_back(std::numeric_limits<unsigned int>::max() / compression_level);
        }
        break;
    case H5Z_FILTER_ZSTD:
        if (compression_level <= 0)
        {
            params.push_back(3);
        }
        else
        {
            level = compression_level * 20 / 9;
            if (level > 20)
            {
                level = 20;
            }
            params.push_back(level);
        }
        break;
    case H5Z_FILTER_BLOSC:
        // BLOSC参数：压缩级别、压缩器类型、块大小、类型大小
        level = compression_level;
        if (level > 9)
            level = 9;
        params.push_back(0);
        params.push_back(0);
        params.push_back(0);
        params.push_back(0);
        params.push_back(level);
        params.push_back(1);
        params.push_back(2);
        break;
    case H5Z_FILTER_BLOSC2:
        level = compression_level;
        if (level > 9)
            level = 9;
        params.push_back(0);
        params.push_back(0);
        params.push_back(0);
        params.push_back(0);
        params.push_back(level);
        params.push_back(1);
        params.push_back(2);
        params.push_back(2);
        params.push_back(4);
        params.push_back(8);
        break;
    case H5Z_FILTER_SZIP:
        // SZIP参数：编码选项（固定为4）和像素每块
        // 根据compression_level设置像素每块值（支持4、8、32）
        params.push_back(4); // 编码选项（EC编码）
        if (compression_level == 4 || compression_level == 8 || compression_level == 32)
        {
            params.push_back(compression_level); // 像素每块
        }
        else
        {
            params.push_back(32); // 默认像素每块
        }
        break;
    case H5Z_FILTER_VBZ:
        // Oxford Nanopore uses this filter specifically to compress raw DNA signal data (signed integer). To achieve this, it uses both:
        // streamvbyte (https://github.com/lemire/streamvbyte)
        // zstd (https://github.com/facebook/zstd)
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

// https://portal.hdfgroup.org/documentation/hdf5/latest/_l_b_com_dset.html
// The H5Pset_deflate call modifies the Dataset Creation Property List instance to use ZLIB or DEFLATE compression. The H5Pset_szip call modifies it to use SZIP compression. There are different compression parameters required for each compression method.
// SZIP compression can only be used with atomic datatypes that are integer, float, or char. It cannot be applied to compound, array, variable-length, enumerations, or other user-defined datatypes. The call to H5Dcreate will fail if attempting to create an SZIP compressed dataset with a non-allowed datatype. The conflict can only be detected when the property list is used.
CompressionResult HDF5Processor::testCompression(
    const std::string &input_file,
    const std::string &filter_name,
    const std::string &parameters,
    int compression_level,
    const std::string &output_dir)
{
    CompressionResult result;
    result.filter_name = filter_name;
    result.parameters = parameters;
    result.compression_level = compression_level;
    result.compression_ratio = 1.0;
    result.compression_time_ms = 0;
    result.decompression_time_ms = 0;
    result.compressed_size_bytes = 0;
    result.original_size_bytes = 0;

    std::cout << "Testing compression: " << filter_name
              << " (level " << compression_level << ")" << std::endl;

    if (!Utils::fileExists(input_file))
    {
        std::cerr << "Input file not found: " << input_file << std::endl;
        return result;
    }

    // 获取原始文件大小
    // size_t original_size = Utils::getFileSize(input_file);
    // result.original_size_bytes = original_size;

    // 将过滤器名称转换为ID
    int filter_id = getFilterIdFromName(filter_name);
    if (filter_id == -1)
    {
        std::cerr << "Unknown filter: " << filter_name << std::endl;
        return result;
    }

    // 生成输出文件名
    std::string output_filename;
    if (output_dir.empty())
    {
        // 如果没有指定输出目录，使用当前目录
        std::string base_name = Utils::getBaseName(input_file);
        std::string name_without_ext = Utils::removeExtension(base_name);
        output_filename = name_without_ext + "_" + filter_name + "_L" +
                          std::to_string(compression_level) + ".h5";
    }
    else
    {
        std::string base_name = Utils::getBaseName(input_file);
        std::string name_without_ext = Utils::removeExtension(base_name);
        output_filename = output_dir + "/" + name_without_ext + "_" + filter_name + "_L" +
                          std::to_string(compression_level) + ".h5";
    }

    std::cout << "Output file: " << output_filename << std::endl;

    // 获取过滤器参数
    std::vector<unsigned int> filter_params = getDefaultFilterParams(filter_id, compression_level);

    // 开始压缩计时
    auto compress_start = high_resolution_clock::now();

    // 打开输入文件
    hid_t src_file_id = H5Fopen(input_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (src_file_id < 0)
    {
        std::cerr << "Failed to open input file: " << input_file << std::endl;
        return result;
    }

    // 创建输出文件
    hid_t dst_file_id = H5Fcreate(output_filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (dst_file_id < 0)
    {
        H5Fclose(src_file_id);
        std::cerr << "Failed to create output file: " << output_filename << std::endl;
        return result;
    }

    // 使用H5Lvisit_by_name递归遍历所有链接，复制整个文件结构并压缩read_xxxx/Raw/Signal数据集
    std::cout << "Copying file structure and compressing read_xxxx/Raw/Signal datasets..." << std::endl;

    // 定义处理数据集的数据结构
    struct ProcessData
    {
        hid_t src_file_id;
        hid_t dst_file_id;
        int filter_id;
        const std::vector<unsigned int> *filter_params;
        size_t *compressed_size;
        size_t *original_size;
        std::set<std::string> created_groups; // 记录已创建的组路径
    };

    ProcessData process_data = {
        src_file_id,
        dst_file_id,
        filter_id,
        &filter_params,
        &result.compressed_size_bytes,
        &result.original_size_bytes,
        {}}; // 初始化created_groups为空集合

    // 使用H5Lvisit_by_name遍历链接
    auto process_callback = [](hid_t group, const char *name, const H5L_info_t *info, void *operator_data) -> herr_t
    {
        ProcessData *data = static_cast<ProcessData *>(operator_data);
        std::string full_path = std::string(name);
        std::cout << "Processing object: " << full_path << std::endl;

        // 确保路径不以双斜杠开头
        H5O_info_t obj_info;
        herr_t status = H5Oget_info_by_name(group, name, &obj_info, H5P_DEFAULT);
        if (status < 0)
        {
            std::cerr << "Failed to get object info for: " << full_path << std::endl;
            return 0; // 继续遍历
        }

        // 处理不同类型的对象
        if (obj_info.type == H5O_TYPE_GROUP)
        {
            // 检查组是否已经在created_groups map中
            if (data->created_groups.find(full_path) == data->created_groups.end())
            {
                // 组不在map中，需要复制
                // std::cout << "Copying group: " << full_path << std::endl;

                // 创建对象复制属性列表
                hid_t ocpypl_id = H5Pcreate(H5P_OBJECT_COPY);
                if (ocpypl_id < 0)
                {
                    std::cerr << "Failed to create object copy property list" << std::endl;
                }
                else
                {
                    // 使用浅层复制：只复制当前组，不复制其子对象
                    status = H5Pset_copy_object(ocpypl_id, H5O_COPY_SHALLOW_HIERARCHY_FLAG /* | H5O_COPY_EXPAND_SOFT_LINK_FLAG | H5O_COPY_EXPAND_EXT_LINK_FLAG*/);
                    if (status < 0)
                    {
                        std::cerr << "Failed to set copy object flag" << std::endl;
                    }

                    herr_t copy_status = H5Ocopy(data->src_file_id, full_path.c_str(),
                                                 data->dst_file_id, full_path.c_str(),
                                                 ocpypl_id, H5P_DEFAULT);

                    // 关闭属性列表
                    H5Pclose(ocpypl_id);

                    if (copy_status < 0)
                    {
                        std::cerr << "Failed to copy group: " << full_path << std::endl;
                        if (data->created_groups.find(full_path) == data->created_groups.end())
                        {
                            std::cout << "Group " << full_path << " not found in map" << std::endl;
                        }
                    }
                    else
                    {
                        // 复制成功，将组路径添加到map中，并检查插入是否成功
                        auto insert_result = data->created_groups.insert(full_path);
                        if (insert_result.second)
                        {
                            // std::cout << "Successfully inserted group into map: " << full_path << std::endl;

                            // 获取目标文件中当前组的直接子group节点，加入到map中（只需要下一层）
                            // 使用H5Gget_num_objs和H5Gget_objname_by_idx的方式更简洁
                            hid_t dst_group_id = H5Gopen(data->dst_file_id, full_path.c_str(), H5P_DEFAULT);
                            if (dst_group_id >= 0)
                            {
                                // 获取组中对象的数量
                                hsize_t num_objs = 0;
                                herr_t num_status = H5Gget_num_objs(dst_group_id, &num_objs);
                                if (num_status >= 0 && num_objs > 0)
                                {
                                    for (hsize_t i = 0; i < num_objs; i++)
                                    {
                                        // 获取对象名称
                                        char child_name[1024];
                                        ssize_t name_len = H5Gget_objname_by_idx(dst_group_id, i, child_name, sizeof(child_name));
                                        if (name_len > 0)
                                        {
                                            // 获取对象信息
                                            H5O_info_t child_info;
                                            herr_t child_status = H5Oget_info_by_name(dst_group_id, child_name, &child_info, H5P_DEFAULT);
                                            if (child_status >= 0 && child_info.type == H5O_TYPE_GROUP)
                                            {
                                                // 构建子组的完整路径
                                                std::string child_full_path = full_path + "/" + std::string(child_name);

                                                // 将子组添加到map中
                                                auto child_insert_result = data->created_groups.insert(child_full_path);
                                                if (child_insert_result.second)
                                                {
                                                    // std::cout << "  Added child group to map: " << child_full_path << std::endl;
                                                }
                                                else
                                                {
                                                    // std::cout << "  Child group already in map: " << child_full_path << std::endl;
                                                }
                                            }
                                        }
                                    }
                                }

                                // 关闭组
                                H5Gclose(dst_group_id);
                            }
                            else
                            {
                                std::cerr << "Failed to open destination group for child traversal: " << full_path << std::endl;
                            }
                        }
                        else
                        {
                            // std::cout << "Group already existed in map (insert failed): " << full_path << std::endl;
                        }
                    }
                }
            }
            else
            {
                // std::cout << "Group already created (in map): " << full_path << std::endl;
            }
        }
        else if (obj_info.type == H5O_TYPE_DATASET)
        {
            // 检查是否为read_xxxx/Raw/Signal数据集
            bool is_target_dataset = false;
            {
                size_t signal_pos = full_path.find("/Raw/Signal");
                if (signal_pos != std::string::npos)
                {
                    is_target_dataset = true;
                }
            }

            if (is_target_dataset)
            {
                // 打开源数据集
                hid_t src_dset_id = H5Dopen(data->src_file_id, full_path.c_str(), H5P_DEFAULT);
                if (src_dset_id < 0)
                {
                    std::cerr << "Failed to open source dataset: " << full_path << std::endl;
                    return 0;
                }

                hid_t src_dcpl_id = H5Dget_create_plist(src_dset_id);
                if (src_dcpl_id < 0)
                {
                    H5Dclose(src_dset_id);
                    return 0;
                }

                /*
                 * Retrieve and print the filter id, compression level and filter's name for bzip2.
                 */
                // char src_filter_name[128];
                // hid_t src_filter_id = H5Pget_filter2(src_dcpl_id, (unsigned)0, NULL, NULL, NULL, sizeof(src_filter_name),
                //                                      src_filter_name, NULL);
                // std::cout << "src filter id :" << src_filter_id << "src filter name :" << src_filter_name << std::endl;

                // 获取数据集信息
                hid_t src_type_id = H5Dget_type(src_dset_id);
                hid_t src_space_id = H5Dget_space(src_dset_id);
                int rank = H5Sget_simple_extent_ndims(src_space_id);
                hsize_t dims[3];
                H5Sget_simple_extent_dims(src_space_id, dims, NULL);
                std::cout << "rank:" << rank << "dims:" << dims[0] << " " << dims[1] << " " << dims[2] << std::endl;

                // 创建数据集创建属性列表
                hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
                if (dcpl_id < 0)
                {
                    std::cerr << "Failed to create property list" << std::endl;
                    H5Tclose(src_type_id);
                    H5Sclose(src_space_id);
                    H5Dclose(src_dset_id);
                    return 0;
                }

                // 设置分块布局 (H5D_CHUNKED) - 压缩必须使用分块布局
                // 计算合适的分块尺寸
                H5Pset_layout(dcpl_id, H5D_CHUNKED);
                hsize_t chunk_dims[3];
                for (int i = 0; i < rank; ++i)
                {
                    // 对于大数据集，使用合理的分块大小
                    // 例如，对于一维数据，使用64KB的分块
                    // if (dims[i] > 65536)
                    // {
                    //     chunk_dims[i] = 65536;
                    // }
                    // else
                    {
                        chunk_dims[i] = dims[i];
                    }
                }

                status = H5Pset_chunk(dcpl_id, rank, chunk_dims);
                if (status < 0)
                {
                    std::cerr << "Failed to set chunk size" << std::endl;
                }

                // 设置过滤器 - 对于 SZIP、SHUFFLE 和 GZIP 使用专门的 API
                if (data->filter_id == H5Z_FILTER_SZIP)
                {
                    // SZIP 参数：编码选项（固定为4）和像素每块
                    unsigned int options_mask = 4;      // EC 编码
                    unsigned int pixels_per_block = 32; // 默认值
                    if (data->filter_params->size() >= 2)
                    {
                        options_mask = (*data->filter_params)[0];
                        pixels_per_block = (*data->filter_params)[1];
                    }
                    status = H5Pset_szip(dcpl_id, options_mask, pixels_per_block);
                    if (status < 0)
                    {
                        std::cerr << "Failed to set SZIP filter" << std::endl;
                    }
                }
                else if (data->filter_id == H5Z_FILTER_SHUFFLE)
                {
                    // SHUFFLE 过滤器没有参数
                    status = H5Pset_shuffle(dcpl_id);
                    if (status < 0)
                    {
                        std::cerr << "Failed to set SHUFFLE filter" << std::endl;
                    }
                }
                else if (data->filter_id == H5Z_FILTER_DEFLATE)
                {
                    // DEFLATE/GZIP 参数：压缩级别
                    unsigned int level = 6; // 默认压缩级别
                    if (data->filter_params->size() >= 1)
                    {
                        level = (*data->filter_params)[0];
                    }
                    status = H5Pset_deflate(dcpl_id, level);
                    if (status < 0)
                    {
                        std::cerr << "Failed to set DEFLATE filter" << std::endl;
                    }
                }
                else if (data->filter_id == H5Z_FILTER_LZ4)
                {
                    htri_t avail = H5Zfilter_avail(H5Z_FILTER_LZ4);
                    if (avail)
                    {
                        std::cout << "LZ4 filter is available" << std::endl;
                    }
                    else
                    {
                        std::cout << "LZ4 filter is not available" << std::endl;
                        return -1;
                    }
                    // 对于其他过滤器，使用通用的 H5Pset_filter
                    const unsigned int cd_values[1] = {(unsigned int)(32 * sizeof(int16_t))};
                    status = H5Pset_filter(dcpl_id, data->filter_id, H5Z_FLAG_MANDATORY,
                                           1, cd_values);
                    if (status < 0)
                    {
                        std::cerr << "Failed to set filter" << std::endl;
                    }
                }
                else
                {
                    // 对于其他过滤器，使用通用的 H5Pset_filter
                    status = H5Pset_filter(dcpl_id, data->filter_id, H5Z_FLAG_OPTIONAL,
                                           data->filter_params->size(), data->filter_params->data());
                    if (status < 0)
                    {
                        std::cerr << "Failed to set filter" << std::endl;
                    }
                }

                hid_t dst_dset_id = H5Dcreate(data->dst_file_id, full_path.c_str(), src_type_id,
                                              src_space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
                if (dst_dset_id < 0)
                {
                    std::cerr << "Failed to create destination dataset" << std::endl;
                    H5Pclose(dcpl_id);
                    H5Tclose(src_type_id);
                    H5Sclose(src_space_id);
                    H5Dclose(src_dset_id);
                    return 0;
                }

                // 读取和写入数据
                size_t element_size = H5Tget_size(src_type_id);
                std::cout << "Element size: " << element_size;
                size_t total_elements = 1;
                for (int i = 0; i < rank; ++i)
                {
                    total_elements *= dims[i];
                    // std::cout << "dim: " << dims[i];
                }
                //????
                std::cout << "rank: " << rank << std::endl;
                size_t data_size = element_size * total_elements;
                *data->original_size += data_size;
                int16_t *buffer = (int16_t *)malloc(dims[0] * sizeof(int16_t));
                // 读取数据
                status = H5Dread(src_dset_id, H5T_NATIVE_INT16, H5S_ALL,
                                 H5S_ALL, H5P_DEFAULT, buffer);

                if (status >= 0)
                {
                    status = H5Dwrite(dst_dset_id, src_type_id, H5S_ALL, H5S_ALL,
                                      H5P_DEFAULT, buffer);
                }

                // 获取压缩后大小
                hsize_t storage_size = H5Dget_storage_size(dst_dset_id);
                if (storage_size > 0)
                {
                    *data->compressed_size += storage_size;
                }

                // storage_size = H5Dget_storage_size(src_dset_id);
                //*data->original_size += storage_size;

                // 清理资源
                free(buffer);
                H5Dclose(dst_dset_id);
                H5Pclose(dcpl_id);
                H5Tclose(src_type_id);
                H5Sclose(src_space_id);
                H5Dclose(src_dset_id);
                H5Pclose(src_dcpl_id);
            }
            else
            {
                std::cout << "is not target dataset: " << full_path << std::endl;
            }
        }
        else
        {
            // 对于非数据集对象，直接复制
            std::cout << "is not dataset or group: " << full_path << std::endl;
        }

        return 0; // 继续遍历
    };

    // 执行遍历
    herr_t status = H5Lvisit_by_name(src_file_id, "/", H5_INDEX_NAME, H5_ITER_NATIVE,
                                     process_callback, &process_data, H5P_DEFAULT);
    if (status < 0)
    {
        std::cerr << "Failed to traverse HDF5 file structure" << std::endl;
    }

    // 打印已创建组的数量
    std::cout << "Total groups created: " << process_data.created_groups.size() << std::endl;

    // 打印所有已创建的组
    std::cout << "All created groups:" << std::endl;
    for (const auto &group_path : process_data.created_groups)
    {
        std::cout << "  - " << group_path << std::endl;
    }

    // 结束压缩计时
    auto compress_end = high_resolution_clock::now();
    auto compress_duration = duration_cast<milliseconds>(compress_end - compress_start);
    result.compression_time_ms = compress_duration.count();

    // 计算压缩比
    if (result.compressed_size_bytes > 0 && result.original_size_bytes > 0)
    {
        result.compression_ratio = static_cast<double>(result.original_size_bytes) / result.compressed_size_bytes;
    }

    // 测试解压缩
    auto decompress_start = high_resolution_clock::now();

    // 重新打开输出文件验证数据
    hid_t verify_file_id = H5Fopen(output_filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (verify_file_id >= 0)
    {
        // 简单验证：检查文件是否可以打开并读取
        H5Fclose(verify_file_id);
    }

    auto decompress_end = high_resolution_clock::now();
    auto decompress_duration = duration_cast<milliseconds>(decompress_end - decompress_start);
    result.decompression_time_ms = decompress_duration.count();

    // 清理资源
    H5Fclose(dst_file_id);
    H5Fclose(src_file_id);

    std::cout << "  Compression ratio: " << Utils::formatRatio(result.compression_ratio)
              << ", Time: " << result.compression_time_ms << " ms"
              << ", Output: " << output_filename << std::endl;

    return result;
}

std::string HDF5Processor::getFilterDescription(const std::string &filter_name)
{
    // 返回过滤器的描述信息
    if (filter_name == "GZIP")
    {
        return "DEFLATE compression algorithm (gzip)";
    }
    else if (filter_name == "SHUFFLE")
    {
        return "Byte shuffling filter (usually combined with other compressors)";
    }
    else if (filter_name == "SZIP")
    {
        return "NASA's lossless compression algorithm";
    }
    else if (filter_name == "LZ4")
    {
        return "Fast lossless compression algorithm";
    }
    else if (filter_name == "ZSTD")
    {
        return "Zstandard compression by Facebook";
    }
    else if (filter_name == "VBZ")
    {
        return "Nanopore VBZ compression for signal data";
    }
    else if (filter_name == "BLOSC")
    {
        return "Blosc meta-compressor";
    }
    else if (filter_name == "BLOSC2")
    {
        return "Blosc2 meta-compressor with improved features";
    }
    else if (filter_name == "BITSHUFFLE")
    {
        return "Bit shuffling filter for improved compression";
    }
    else if (filter_name == "BZIP2")
    {
        return "Bzip2 compression algorithm";
    }
    else if (filter_name == "LZF")
    {
        return "LZF compression algorithm";
    }
    return "Unknown filter";
}

bool HDF5Processor::isFilterAvailable(const std::string &filter_name)
{
    int filter_id = getFilterIdFromName(filter_name);
    if (filter_id == -1)
    {
        return false;
    }

    htri_t avail = H5Zfilter_avail(filter_id);
    return (avail > 0);
}

long long HDF5Processor::getCurrentTimeMs()
{
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<milliseconds>(duration).count();
}

std::vector<HDF5Processor::DatasetInfo> HDF5Processor::findSignalDatasets(hid_t file_id)
{
    std::vector<DatasetInfo> datasets;

    // 简化实现：在实际应用中，这里应该遍历HDF5文件中的所有数据集
    // 这里返回一个空的向量
    return datasets;
}

size_t HDF5Processor::getDatasetSize(const DatasetInfo &info)
{
    // 计算数据集的大小
    size_t element_size = H5Tget_size(info.datatype);
    size_t total_elements = 1;

    for (int i = 0; i < info.rank; ++i)
    {
        total_elements *= info.dims[i];
    }

    return element_size * total_elements;
}
