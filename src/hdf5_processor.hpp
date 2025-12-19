#ifndef HDF5_PROCESSOR_HPP
#define HDF5_PROCESSOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <hdf5.h>
#include <hdf5_hl.h>

struct CompressionResult
{
    std::string filter_name;
    std::string parameters;
    int compression_level;
    double compression_ratio;
    long long compression_time_ms;
    long long decompression_time_ms;
    size_t compressed_size_bytes;
    size_t original_size_bytes;
};

class HDF5Processor
{
public:
    HDF5Processor();
    ~HDF5Processor();

    // 禁止拷贝和赋值
    HDF5Processor(const HDF5Processor &) = delete;
    HDF5Processor &operator=(const HDF5Processor &) = delete;

    // 移动构造函数和赋值运算符
    HDF5Processor(HDF5Processor &&) noexcept;
    HDF5Processor &operator=(HDF5Processor &&) noexcept;

    // 压缩测试
    CompressionResult testCompression(
        const std::string &input_file,
        const std::string &filter_name,
        const std::string &parameters,
        int compression_level,
        const std::string &output_dir = "");

    // 工具函数
    static std::string getFilterDescription(const std::string &filter_name);
    static bool isFilterAvailable(const std::string &filter_name);

private:
    // HDF5对象管理
    struct DatasetInfo
    {
        std::string path;
        hid_t datatype;
        hid_t dataspace;
        hsize_t dims[3];
        int rank;
    };

    std::vector<DatasetInfo> findSignalDatasets(hid_t file_id);
    size_t getDatasetSize(const DatasetInfo &info);

    // 时间测量
    static long long getCurrentTimeMs();
};

#endif // HDF5_PROCESSOR_HPP
