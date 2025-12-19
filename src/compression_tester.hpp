#ifndef COMPRESSION_TESTER_HPP
#define COMPRESSION_TESTER_HPP

#include <string>
#include <vector>
#include <map>
#include "hdf5_processor.hpp"

class CompressionTester
{
public:
    CompressionTester();
    ~CompressionTester() = default;

    // 配置
    struct TestConfig
    {
        std::string input_file;
        std::string output_dir;
        std::vector<std::string> filters_to_test;
        bool test_all_levels = true;
        bool include_shuffle = true;
        bool verbose = false;
    };

    // 运行完整测试套件
    std::vector<CompressionResult> runTestSuite(const TestConfig &config);

    // 生成报告
    bool generateReport(
        const std::vector<CompressionResult> &results,
        const std::string &output_file,
        const std::string &format = "markdown");

    static std::map<std::string, std::vector<int>> getFilterLevels();
    static std::map<std::string, std::string> getFilterParameters();

private:
    // 内部测试方法
    std::vector<CompressionResult> testFilterWithLevels(
        const std::string &input_file,
        const std::string &filter_name,
        const std::vector<int> &levels);

    std::vector<CompressionResult> testFilterWithShuffle(
        const std::string &input_file,
        const std::string &filter_name,
        int level);

    // 报告生成
    std::string generateMarkdownReport(const std::vector<CompressionResult> &results);
    std::string generateCSVReport(const std::vector<CompressionResult> &results);
    std::string generateJSONReport(const std::vector<CompressionResult> &results);

    HDF5Processor processor_;
};

#endif // COMPRESSION_TESTER_HPP
