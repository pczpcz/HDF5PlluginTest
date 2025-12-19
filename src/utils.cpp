#include "utils.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <thread>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#endif

namespace fs = std::filesystem;

bool Utils::fileExists(const std::string &path)
{
    return fs::exists(path);
}

size_t Utils::getFileSize(const std::string &path)
{
    try
    {
        return fs::file_size(path);
    }
    catch (...)
    {
        return 0;
    }
}

bool Utils::createDirectory(const std::string &path)
{
    try
    {
        return fs::create_directories(path);
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::string> Utils::listFiles(const std::string &directory, const std::string &pattern)
{
    std::vector<std::string> files;
    try
    {
        for (const auto &entry : fs::directory_iterator(directory))
        {
            if (fs::is_regular_file(entry.path()))
            {
                std::string filename = entry.path().filename().string();
                // 简单的模式匹配（支持通配符*）
                if (pattern == "*" || filename.find(pattern) != std::string::npos)
                {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    catch (...)
    {
        // 忽略错误
    }
    return files;
}

std::string Utils::getBaseName(const std::string &path)
{
    try
    {
        return fs::path(path).filename().string();
    }
    catch (...)
    {
        return path;
    }
}

std::string Utils::removeExtension(const std::string &filename)
{
    try
    {
        fs::path p(filename);
        return p.stem().string();
    }
    catch (...)
    {
        // 如果失败，尝试手动移除扩展名
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos)
        {
            return filename.substr(0, pos);
        }
        return filename;
    }
}

std::string Utils::toLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return result;
}

std::string Utils::toUpper(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });
    return result;
}

std::vector<std::string> Utils::split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

std::string Utils::trim(const std::string &str)
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start))
    {
        start++;
    }

    auto end = str.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

std::string Utils::getCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string Utils::formatDuration(long long milliseconds)
{
    if (milliseconds < 1000)
    {
        return std::to_string(milliseconds) + " ms";
    }
    else if (milliseconds < 60000)
    {
        double seconds = milliseconds / 1000.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << seconds << " s";
        return ss.str();
    }
    else if (milliseconds < 3600000)
    {
        double minutes = milliseconds / 60000.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << minutes << " min";
        return ss.str();
    }
    else
    {
        double hours = milliseconds / 3600000.0;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << hours << " h";
        return ss.str();
    }
}

std::string Utils::formatSize(size_t bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit_index < 4)
    {
        size /= 1024.0;
        unit_index++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

std::string Utils::formatRatio(double ratio)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ratio;
    return ss.str();
}

std::string Utils::formatTime(long long milliseconds)
{
    return formatDuration(milliseconds);
}

std::string Utils::getSystemInfo()
{
    std::stringstream ss;

#ifdef _WIN32
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((OSVERSIONINFO *)&osvi))
    {
        ss << "Windows " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion;
        ss << " Build " << osvi.dwBuildNumber;
    }
#else
    std::ifstream os_release("/etc/os-release");
    if (os_release.is_open())
    {
        std::string line;
        while (std::getline(os_release, line))
        {
            if (line.find("PRETTY_NAME=") == 0)
            {
                ss << line.substr(13, line.length() - 14);
                break;
            }
        }
        os_release.close();
    }
#endif

    return ss.str();
}

std::string Utils::getCPUInfo()
{
    std::stringstream ss;

#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    ss << "CPU Cores: " << sysInfo.dwNumberOfProcessors;
#else
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open())
    {
        std::string line;
        int core_count = 0;
        std::string model_name;

        while (std::getline(cpuinfo, line))
        {
            if (line.find("processor") == 0)
            {
                core_count++;
            }
            else if (line.find("model name") == 0 && model_name.empty())
            {
                size_t pos = line.find(':');
                if (pos != std::string::npos)
                {
                    model_name = line.substr(pos + 2);
                }
            }
        }
        cpuinfo.close();

        if (!model_name.empty())
        {
            ss << model_name;
        }
        ss << " (" << core_count << " cores)";
    }
#endif

    return ss.str();
}

size_t Utils::getAvailableMemory()
{
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    return memInfo.freeram * memInfo.mem_unit;
#endif
}

bool Utils::isHDF5File(const std::string &path)
{
    // 简单的HDF5文件检查：检查文件扩展名
    std::string ext = fs::path(path).extension().string();
    std::string lower_ext = toLower(ext);

    return lower_ext == ".h5" || lower_ext == ".hdf5" || lower_ext == ".hdf";
}

std::vector<std::string> Utils::getHDF5DatasetPaths(const std::string &file_path)
{
    // 这里应该使用HDF5 API获取数据集路径
    // 目前返回空向量，实际实现需要HDF5库
    return {};
}

std::string Utils::getHDF5FileInfo(const std::string &file_path)
{
    std::stringstream ss;
    ss << "File: " << file_path << "\n";
    ss << "Size: " << formatSize(getFileSize(file_path)) << "\n";
    ss << "Type: " << (isHDF5File(file_path) ? "HDF5" : "Unknown") << "\n";
    return ss.str();
}

std::string Utils::getCompressionFilterName(int filter_id)
{
    // HDF5过滤器ID到名称的映射
    switch (filter_id)
    {
    case 1:
        return "DEFLATE (GZIP)";
    case 2:
        return "SHUFFLE";
    case 3:
        return "FLETCHER32";
    case 4:
        return "SZIP";
    case 5:
        return "NBIT";
    case 6:
        return "SCALEOFFSET";
    default:
        return "Unknown Filter";
    }
}

bool Utils::isCompressionFilterAvailable(int filter_id)
{
    // 这里应该检查HDF5过滤器是否可用
    // 目前返回true，实际实现需要HDF5库
    return true;
}

std::vector<int> Utils::getAvailableCompressionFilters()
{
    // 返回可用的压缩过滤器ID列表
    // 目前返回一些常见的过滤器
    return {1, 2, 4}; // DEFLATE, SHUFFLE, SZIP
}

void Utils::logInfo(const std::string &message)
{
    std::cout << "[INFO] " << getCurrentTimeString() << " - " << message << std::endl;
}

void Utils::logWarning(const std::string &message)
{
    std::cout << "[WARN] " << getCurrentTimeString() << " - " << message << std::endl;
}

void Utils::logError(const std::string &message)
{
    std::cerr << "[ERROR] " << getCurrentTimeString() << " - " << message << std::endl;
}

void Utils::logDebug(const std::string &message)
{
#ifdef DEBUG
    std::cout << "[DEBUG] " << getCurrentTimeString() << " - " << message << std::endl;
#endif
}

bool Utils::saveConfig(const std::string &path, const std::string &config)
{
    try
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            return false;
        }
        file << config;
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string Utils::loadConfig(const std::string &path)
{
    try
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return buffer.str();
    }
    catch (...)
    {
        return "";
    }
}
