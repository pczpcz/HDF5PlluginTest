#include "compression_tester.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

CompressionTester::CompressionTester() : processor_()
{
}

std::vector<CompressionResult> CompressionTester::runTestSuite(const TestConfig &config)
{
    std::vector<CompressionResult> all_results;

    std::cout << "Starting compression test suite..." << std::endl;
    std::cout << "Input file: " << config.input_file << std::endl;
    std::cout << "Output directory: " << config.output_dir << std::endl;
    std::cout << "Filters to test: " << config.filters_to_test.size() << std::endl;

    if (!Utils::fileExists(config.input_file))
    {
        std::cerr << "Error: Input file does not exist: " << config.input_file << std::endl;
        return all_results;
    }

    // 获取原始文件大小
    size_t original_size = Utils::getFileSize(config.input_file);
    std::cout << "Original file size: " << Utils::formatSize(original_size) << std::endl;

    // 添加基准测试结果（未压缩）
    CompressionResult baseline;
    baseline.filter_name = "None";
    baseline.parameters = "";
    baseline.compression_level = 0;
    baseline.compression_ratio = 1.0;
    baseline.compression_time_ms = 0;
    baseline.decompression_time_ms = 0;
    baseline.compressed_size_bytes = original_size;
    baseline.original_size_bytes = original_size;
    all_results.push_back(baseline);

    // 测试每个过滤器
    for (const auto &filter_name : config.filters_to_test)
    {
        std::cout << "\nTesting filter: " << filter_name << std::endl;
        std::cout << "Description: " << HDF5Processor::getFilterDescription(filter_name) << std::endl;

        // 获取该过滤器的测试级别
        auto filter_levels = getFilterLevels();
        std::vector<int> levels;

        if (config.test_all_levels && filter_levels.find(filter_name) != filter_levels.end())
        {
            levels = filter_levels[filter_name];
        }
        else
        {
            // 使用默认级别
            levels = {1, 6, 9};
        }

        // 测试每个压缩级别
        auto level_results = testFilterWithLevels(config.input_file, filter_name, levels);
        all_results.insert(all_results.end(), level_results.begin(), level_results.end());
    }

    std::cout << "\nTest suite completed. Total results: " << all_results.size() << std::endl;

    return all_results;
}

bool CompressionTester::generateReport(
    const std::vector<CompressionResult> &results,
    const std::string &output_file,
    const std::string &format)
{
    std::cout << "Generating report in " << format << " format: " << output_file << std::endl;

    std::string report_content;

    if (format == "markdown")
    {
        report_content = generateMarkdownReport(results);
    }
    else if (format == "csv")
    {
        report_content = generateCSVReport(results);
    }
    else if (format == "json")
    {
        report_content = generateJSONReport(results);
    }
    else
    {
        std::cerr << "Error: Unknown report format: " << format << std::endl;
        return false;
    }

    // 保存报告到文件
    if (Utils::saveConfig(output_file, report_content))
    {
        std::cout << "Report saved successfully: " << output_file << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Error: Failed to save report to: " << output_file << std::endl;
        return false;
    }
}

std::map<std::string, std::vector<int>> CompressionTester::getFilterLevels()
{
    std::map<std::string, std::vector<int>> levels;

    levels["GZIP"] = {1, 6, 9};

    levels["SZIP"] = {1, 6, 9};

    levels["SHUFFLE"] = {1, 6, 9};

    levels["LZ4"] = {1, 6, 9};

    levels["ZSTD"] = {1, 6, 9};

    levels["VBZ"] = {1, 6, 9};

    levels["BLOSC"] = {1, 6, 9};

    levels["BLOSC2"] = {1, 6, 9};

    return levels;
}

std::map<std::string, std::string> CompressionTester::getFilterParameters()
{
    std::map<std::string, std::string> params;

    // SHUFFLE参数
    params["SHUFFLE"] = "shuffle=1";

    // 块大小参数
    params["CHUNK_SIZE_64K"] = "chunk_size=65536";
    params["CHUNK_SIZE_128K"] = "chunk_size=131072";
    params["CHUNK_SIZE_1M"] = "chunk_size=1048576";

    return params;
}

std::vector<CompressionResult> CompressionTester::testFilterWithLevels(
    const std::string &input_file,
    const std::string &filter_name,
    const std::vector<int> &levels)
{
    std::vector<CompressionResult> results;

    // 从输入文件路径生成输出目录
    std::string base_name = Utils::getBaseName(input_file);
    std::string name_without_ext = Utils::removeExtension(base_name);
    std::string output_dir = "results";

    for (int level : levels)
    {
        std::cout << "  Testing level " << level << "... ";

        CompressionResult result = processor_.testCompression(
            input_file, filter_name, "", level, output_dir);
        results.push_back(result);

        std::cout << "Ratio: " << Utils::formatRatio(result.compression_ratio)
                  << ", Time: " << result.compression_time_ms << " ms" << std::endl;
    }

    return results;
}

std::vector<CompressionResult> CompressionTester::testFilterWithShuffle(
    const std::string &input_file,
    const std::string &filter_name,
    int level)
{
    std::vector<CompressionResult> results;

    // 从输入文件路径生成输出目录
    std::string base_name = Utils::getBaseName(input_file);
    std::string name_without_ext = Utils::removeExtension(base_name);
    std::string output_dir = "results/" + name_without_ext + "_" + filter_name + "_SHUFFLE";

    // 确保输出目录存在
    if (!Utils::createDirectory(output_dir))
    {
        std::cerr << "Warning: Failed to create output directory: " << output_dir << std::endl;
        output_dir = ""; // 使用默认输出目录
    }

    // 测试SHUFFLE + 过滤器的组合
    std::string combined_name = "SHUFFLE+" + filter_name;
    std::string parameters = "shuffle=1";

    std::cout << "  Testing " << combined_name << " (level " << level << ")... ";

    CompressionResult result = processor_.testCompression(
        input_file, filter_name, parameters, level, output_dir);

    // 修改过滤器名称以反映组合
    result.filter_name = combined_name;
    result.parameters = parameters;

    results.push_back(result);

    std::cout << "Ratio: " << Utils::formatRatio(result.compression_ratio)
              << ", Time: " << result.compression_time_ms << " ms" << std::endl;

    return results;
}

std::string CompressionTester::generateMarkdownReport(const std::vector<CompressionResult> &results)
{
    std::stringstream ss;

    for (auto &result : results)
    {
        std::cout << "all_results: " << result.filter_name << "," << result.compressed_size_bytes << "," << result.original_size_bytes << std::endl;
    }

    // 报告头部
    ss << "# HDF5 Compression Test Report\n\n";
    ss << "## Test Information\n";
    ss << "- Test Time: " << Utils::getCurrentTimeString() << "\n";
    ss << "- System: " << Utils::getSystemInfo() << "\n";
    ss << "- CPU: " << Utils::getCPUInfo() << "\n";
    ss << "- Available Memory: " << Utils::formatSize(Utils::getAvailableMemory()) << "\n\n";

    // 结果表格
    ss << "## Test Results\n\n";
    ss << "| Filter | Parameters | Level | Ratio | Comp Time (ms) | Decomp Time (ms) | Size | Original Size |\n";
    ss << "|--------|------------|-------|-------|----------------|------------------|------|---------------|\n";

    for (const auto &result : results)
    {
        ss << "| " << result.filter_name
           << " | " << result.parameters
           << " | " << result.compression_level
           << " | " << std::fixed << std::setprecision(2) << result.compression_ratio
           << " | " << result.compression_time_ms
           << " | " << result.decompression_time_ms
           << " | " << Utils::formatSize(result.compressed_size_bytes)
           << " | " << Utils::formatSize(result.original_size_bytes)
           << " |\n";
    }

    // 分析部分
    ss << "\n## Analysis\n\n";

    // 声明变量
    std::vector<CompressionResult>::const_iterator max_ratio, min_time, best_balance;
    bool has_results = !results.empty();

    if (has_results)
    {
        // 找到最佳压缩比
        max_ratio = std::max_element(results.begin(), results.end(),
                                     [](const CompressionResult &a, const CompressionResult &b)
                                     {
                                         return a.compression_ratio < b.compression_ratio;
                                     });

        // 找到最快压缩
        min_time = std::min_element(results.begin(), results.end(),
                                    [](const CompressionResult &a, const CompressionResult &b)
                                    {
                                        return a.compression_time_ms < b.compression_time_ms;
                                    });

        // 找到最佳平衡（压缩比/时间）
        best_balance = std::max_element(results.begin(), results.end(),
                                        [](const CompressionResult &a, const CompressionResult &b)
                                        {
                                            double score_a = a.compression_ratio / (a.compression_time_ms + 1.0);
                                            double score_b = b.compression_ratio / (b.compression_time_ms + 1.0);
                                            return score_a < score_b;
                                        });

        ss << "### Best Compression Ratio\n";
        ss << "- **Filter**: " << max_ratio->filter_name << "\n";
        ss << "- **Level**: " << max_ratio->compression_level << "\n";
        ss << "- **Ratio**: " << std::fixed << std::setprecision(2) << max_ratio->compression_ratio << "\n";
        ss << "- **Time**: " << max_ratio->compression_time_ms << " ms\n\n";

        ss << "### Fastest Compression\n";
        ss << "- **Filter**: " << min_time->filter_name << "\n";
        ss << "- **Level**: " << min_time->compression_level << "\n";
        ss << "- **Ratio**: " << std::fixed << std::setprecision(2) << min_time->compression_ratio << "\n";
        ss << "- **Time**: " << min_time->compression_time_ms << " ms\n\n";

        ss << "### Best Balance (Ratio/Time)\n";
        ss << "- **Filter**: " << best_balance->filter_name << "\n";
        ss << "- **Level**: " << best_balance->compression_level << "\n";
        ss << "- **Ratio**: " << std::fixed << std::setprecision(2) << best_balance->compression_ratio << "\n";
        ss << "- **Time**: " << best_balance->compression_time_ms << " ms\n";

        double balance_score = best_balance->compression_ratio / (best_balance->compression_time_ms + 1.0);
        ss << "- **Score**: " << std::fixed << std::setprecision(4) << balance_score << " ratio/ms\n";
    }
    else
    {
        ss << "No test results available for analysis.\n";
    }

    // 结论
    ss << "\n## Conclusion\n\n";
    ss << "Based on the test results, the recommended compression settings depend on the use case:\n\n";
    ss << "1. **For maximum compression ratio**: Use "
       << (has_results ? max_ratio->filter_name + " level " + std::to_string(max_ratio->compression_level) : "ZSTD level 19")
       << "\n";
    ss << "2. **For fastest compression**: Use "
       << (has_results ? min_time->filter_name + " level " + std::to_string(min_time->compression_level) : "LZ4 level 1")
       << "\n";
    ss << "3. **For best balance**: Use "
       << (has_results ? best_balance->filter_name + " level " + std::to_string(best_balance->compression_level) : "GZIP level 6")
       << "\n";

    return ss.str();
}

std::string CompressionTester::generateCSVReport(const std::vector<CompressionResult> &results)
{
    std::stringstream ss;

    // CSV头部
    ss << "filter_name,parameters,compression_level,compression_ratio,"
       << "compression_time_ms,decompression_time_ms,"
       << "compressed_size_bytes,original_size_bytes\n";

    // 数据行
    for (const auto &result : results)
    {
        ss << result.filter_name << ","
           << "\"" << result.parameters << "\","
           << result.compression_level << ","
           << std::fixed << std::setprecision(4) << result.compression_ratio << ","
           << result.compression_time_ms << ","
           << result.decompression_time_ms << ","
           << result.compressed_size_bytes << ","
           << result.original_size_bytes << "\n";
    }

    return ss.str();
}

std::string CompressionTester::generateJSONReport(const std::vector<CompressionResult> &results)
{
    std::stringstream ss;

    ss << "{\n";
    ss << "  \"test_report\": {\n";
    ss << "    \"timestamp\": \"" << Utils::getCurrentTimeString() << "\",\n";
    ss << "    \"system_info\": {\n";
    ss << "      \"os\": \"" << Utils::getSystemInfo() << "\",\n";
    ss << "      \"cpu\": \"" << Utils::getCPUInfo() << "\",\n";
    ss << "      \"available_memory\": " << Utils::getAvailableMemory() << "\n";
    ss << "    },\n";
    ss << "    \"results\": [\n";

    for (size_t i = 0; i < results.size(); ++i)
    {
        const auto &result = results[i];
        ss << "      {\n";
        ss << "        \"filter_name\": \"" << result.filter_name << "\",\n";
        ss << "        \"parameters\": \"" << result.parameters << "\",\n";
        ss << "        \"compression_level\": " << result.compression_level << ",\n";
        ss << "        \"compression_ratio\": " << std::fixed << std::setprecision(4) << result.compression_ratio << ",\n";
        ss << "        \"compression_time_ms\": " << result.compression_time_ms << ",\n";
        ss << "        \"decompression_time_ms\": " << result.decompression_time_ms << ",\n";
        ss << "        \"compressed_size_bytes\": " << result.compressed_size_bytes << ",\n";
        ss << "        \"original_size_bytes\": " << result.original_size_bytes << "\n";
        ss << "      }";

        if (i < results.size() - 1)
        {
            ss << ",";
        }
        ss << "\n";
    }

    ss << "    ]\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}
