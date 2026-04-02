#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <random>
#include <filesystem>
#include <sys/stat.h>
#include <sys/xattr.h>

namespace file_utils {
    
static bool file_exists(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

static bool read_file_bytes(const char* path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    f.seekg(0, std::ios::end);
    std::streamsize n = f.tellg();
    if (n < 0) return false;

    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(n));
    return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()), n));
}

static inline bool create_empty_file(const char* target_path) {
    std::ofstream file(target_path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    file.close();
    return true;
}

static bool make_dirs(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::create_directories(p, ec);
}

static bool create_directory_if_not_exists(const char* dir_path) {
    if (!file_exists(dir_path)) {
        make_dirs(dir_path);
    }
    return file_exists(dir_path);
}

static bool delete_path(const std::filesystem::path& dir_path) {
    std::error_code ec;
    std::filesystem::remove_all(dir_path, ec);
    return !file_exists(dir_path);
}

#ifndef XATTR_NAME_SELINUX
#define XATTR_NAME_SELINUX "security.selinux"
#endif

#ifndef SELINUX_SYSTEM_FILE_FLAG
#define SELINUX_SYSTEM_FILE_FLAG "u:object_r:system_file:s0"
#endif

#ifndef SELINUX_KERNEL_FILE_FLAG
#define SELINUX_KERNEL_FILE_FLAG "u:object_r:kernel:s0"
#endif

enum class SelinuxFileFlag {
  SELINUX_SYSTEM_FILE,
  SELINUX_KERNEL_FILE,
};

static inline bool set_file_selinux_access_mode(const char* file_full_path, SelinuxFileFlag flag) {
    if (chmod(file_full_path, 0777) != 0) return false;
    const char* selinux_flag = flag == SelinuxFileFlag::SELINUX_SYSTEM_FILE ? SELINUX_SYSTEM_FILE_FLAG : SELINUX_KERNEL_FILE_FLAG;
    if (setxattr(file_full_path, XATTR_NAME_SELINUX, selinux_flag, strlen(selinux_flag) + 1, 0) != 0) return false;
    return true;
}

static bool copy_selinux_context(const char* source_file_path, const char* target_file_path) {
    char selinux_context[512] = { 0 }; // adjust the size as per your requirement
    // Retrieve the SELinux context from the source file
    ssize_t length = getxattr(source_file_path, XATTR_NAME_SELINUX, selinux_context, sizeof(selinux_context));
    if (length == -1) return false;
    selinux_context[length] = '\0'; // ensure null termination
    // Set the SELinux context to the target file
    if (setxattr(target_file_path, XATTR_NAME_SELINUX, selinux_context, strlen(selinux_context) + 1, 0)) return false;
    return true;
}

static bool random_expand_file(const char* file_path) {
    std::ofstream file(file_path, std::ios::binary | std::ios::app);
    if (!file.is_open()) return false;
    std::random_device rd;
    std::mt19937 gen(rd());
    // 随机大小：1MB–5MB
    std::uniform_int_distribution<size_t> size_dis(1 * 1024 * 1024, 5 * 1024 * 1024);
    size_t random_size = size_dis(gen);
    std::uniform_int_distribution<unsigned> byte_dis(0, 255);
    std::vector<char> buffer(random_size);
    for (size_t i = 0; i < random_size; ++i) {
        buffer[i] = static_cast<char>(byte_dis(gen));
    }
    file.write(buffer.data(), buffer.size());
    if (!file) return false;
    file.close();
    return true;
}

} // namespace file_utils
