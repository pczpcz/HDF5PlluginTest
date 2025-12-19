#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class Utils
{
public:
    // 文件操作
    static bool fileExists(const std::string &path);
    static size_t getFileSize(const std::string &path);
    static bool createDirectory(const std::string &path);
    static std::vector<std::string> listFiles(const std::string &directory, const std::string &pattern = "*");
    static std::string getBaseName(const std::string &path);
    static std::string removeExtension(const std::string &filename);

    // 字符串操作
    static std::string toLower(const std::string &str);
    static std::string toUpper(const std::string &str);
    static std::vector<std::string> split(const std::string &str, char delimiter);
    static std::string trim(const std::string &str);

    // 时间操作
    static std::string getCurrentTimeString();
    static std::string formatDuration(long long milliseconds);

    // 格式化输出
    static std::string formatSize(size_t bytes);
    static std::string formatRatio(double ratio);
    static std::string formatTime(long long milliseconds);

    // 系统信息
    static std::string getSystemInfo();
    static std::string getCPUInfo();
    static size_t getAvailableMemory();

    // HDF5相关
    static bool isHDF5File(const std::string &path);
    static std::vector<std::string> getHDF5DatasetPaths(const std::string &file_path);
    static std::string getHDF5FileInfo(const std::string &file_path);

    // 压缩相关
    static std::string getCompressionFilterName(int filter_id);
    static bool isCompressionFilterAvailable(int filter_id);
    static std::vector<int> getAvailableCompressionFilters();

    // 日志
    static void logInfo(const std::string &message);
    static void logWarning(const std::string &message);
    static void logError(const std::string &message);
    static void logDebug(const std::string &message);

    // 配置
    static bool saveConfig(const std::string &path, const std::string &config);
    static std::string loadConfig(const std::string &path);

private:
    Utils() = delete;
    ~Utils() = delete;
};

#endif // UTILS_HPP
