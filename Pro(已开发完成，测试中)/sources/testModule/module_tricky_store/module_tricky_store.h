#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>

static bool file_exists(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

static bool delete_path(const std::filesystem::path& dir_path) {
    std::error_code ec;
    std::filesystem::remove_all(dir_path, ec);
    return !file_exists(dir_path);
}

static bool copy_dir(const std::filesystem::path& src, const std::filesystem::path& dst) {
    std::error_code ec;
    std::filesystem::create_directories(dst, ec);
    if (ec) return false;
    std::filesystem::copy(src, dst,
             std::filesystem::copy_options::recursive |
             std::filesystem::copy_options::overwrite_existing,
             ec);
    return !ec;
}

static void chmod_tree_777(const std::filesystem::path& dir) {
    ::chmod(dir.c_str(), 0777);
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir)) {
        ::chmod(e.path().c_str(), 0777);
    }
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

static bool read_text_file(const char* target_path, std::string& text) {
    std::vector<uint8_t> buf;
	if(!read_file_bytes(target_path, buf)) return false;
	text.assign(buf.begin(), buf.end());
    return true;
}

static bool write_text_file(const char* target_path, std::string_view text) {
    if (!target_path) return false;
    std::ofstream file(target_path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    file << text;
    return file.good();
}

static std::string run_cmd(const std::string& cmd) {
	std::string cmd_add_err_info = cmd;
	cmd_add_err_info += " 2>&1";
    FILE * fp = popen(cmd_add_err_info.c_str(), "r");
    if(!fp) return {};
    int pip = fileno(fp);

    std::string result;
    while(true) {
        char rbuf[1024] = {0};
        ssize_t r = read(pip, rbuf, sizeof(rbuf));
        if (r == -1 && errno == EAGAIN) continue;
        else if(r > 0) { std::string str_convert(rbuf, r); result += str_convert; }
        else break;
    }
    pclose(fp);
    return result;
}

static void run_script(const char* script) {
    const char* sh = "/system/bin/sh";
    char* const argv[] = {
        (char*)sh,
        (char*)script,
        nullptr
    };
    execve(sh, argv, environ);
}
