#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include "hdf5_processor.hpp"
#include "compression_tester.hpp"
#include "utils.hpp"
#include "filter_definitions.hpp"

#define FILTER_VBZ_ID 32020
#define FILTER_VBZ_VERSION_OPTION 0
#define FILTER_VBZ_INTEGER_SIZE_OPTION 1
#define FILTER_VBZ_USE_DELTA_ZIG_ZAG_COMPRESSION 2
#define FILTER_VBZ_ZSTD_COMPRESSION_LEVEL_OPTION 3

void printHelp()
{
    std::cout << "HDF5 Compression Benchmark Tool\n";
    std::cout << "Usage:\n";
    std::cout << "  hdf5_compression_bench [command] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  test            Run compression tests\n";
    std::cout << "  help            Show this help message\n\n";
    std::cout << "Options:\n";
    std::cout << "  --input FILE    Input file path\n";
    std::cout << "  --output FILE   Output file path\n";
    std::cout << "  --dir DIR       Output directory\n";
    std::cout << "  --filters LIST  Comma-separated list of filters to test\n";
    std::cout << "  --levels LIST   Comma-separated list of compression levels\n";
    std::cout << "  --format FORMAT Output format (markdown, csv, json)\n";
    std::cout << "  --verbose       Enable verbose output\n";
}

int runTests(const std::vector<std::string> &args)
{
    CompressionTester::TestConfig config;
    config.verbose = false;
    config.test_all_levels = true;
    config.include_shuffle = true;

    // 解析参数
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (args[i] == "--input" && i + 1 < args.size())
        {
            config.input_file = args[++i];
        }
        else if (args[i] == "--dir" && i + 1 < args.size())
        {
            config.output_dir = args[++i];
        }
        else if (args[i] == "--filters" && i + 1 < args.size())
        {
            std::string filters_str = args[++i];
            config.filters_to_test = Utils::split(filters_str, ',');
        }
        else if (args[i] == "--verbose")
        {
            config.verbose = true;
        }
        else if (args[i] == "--format" && i + 1 < args.size())
        {
            // 格式参数，在generateReport中使用
            i++; // 跳过格式值
        }
    }

    if (config.input_file.empty())
    {
        // 如果没有指定输入文件，使用当前目录下的HDF5文件
        auto files = Utils::listFiles(".", "*.h5");
        if (!files.empty())
        {
            config.input_file = files[0];
            std::cout << "Using input file: " << config.input_file << std::endl;
        }
        else
        {
            config.input_file = "test_data.h5";
            std::cout << "No HDF5 file found, will create test data" << std::endl;
        }
    }

    if (config.output_dir.empty())
    {
        config.output_dir = "results";
    }

    CompressionTester tester;
    std::cout << "Running compression tests...\n";
    std::cout << "Input file: " << config.input_file << "\n";
    std::cout << "Output directory: " << config.output_dir << "\n";
    std::cout << "Testing " << config.filters_to_test.size() << " filters\n";

    auto results = tester.runTestSuite(config);

    // 生成报告
    std::string report_file = config.output_dir + "/test_report.md";
    if (tester.generateReport(results, report_file, "markdown"))
    {
        std::cout << "Test report generated: " << report_file << "\n";
    }

    return 0;
}

int main(int argc, char *argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    if (args.empty() || args[0] == "help")
    {
        printHelp();
        return 0;
    }

    std::string command = args[0];
    args.erase(args.begin());

    if (command == "test")
    {
        return runTests(args);
    }
    else
    {
        std::cout << "Unknown command: " << command << "\n";
        printHelp();
        return 1;
    }

    return 0;
}
