#ifndef ARM64_ASM_HELPER_H_
#define ARM64_ASM_HELPER_H_
#include <string>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <array>
#include <filesystem>
#include "base_func.h"

static std::string Arm64AsmToBytes(const std::string& strAsm) {
	//获取汇编文本
	std::array<char, MAX_PATH> buffer{};
	DWORD len = ::GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	std::filesystem::path exePath{ buffer.data() };
	std::filesystem::path exeDir = exePath.parent_path();
	std::filesystem::path asmFilePath = exeDir / "aarch64-linux-android-as.exe";
	std::string asmFilePathStr = asmFilePath.string();
	if (!std::filesystem::exists(asmFilePath)) {
		std::cerr << "Error: aarch64-linux-android-as.exe not found. Please extract this file from the Android NDK." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string bytes;
	std::vector<std::string> lines = splite_lines(strAsm);

	const size_t GROUP_SIZE = 10;
	size_t totalLines = lines.size();
	for (size_t i = 0; i < totalLines; i += GROUP_SIZE) {
		size_t chunkCount = min(GROUP_SIZE, totalLines - i);

		std::string echoCmd = "(";
		for (size_t j = 0; j < chunkCount; ++j) {
			echoCmd += "echo ";
			echoCmd += lines[i + j];
			if (j + 1 < chunkCount) {
				echoCmd += " & ";
			}
		}
		echoCmd += ")";
		std::string cmd = echoCmd + " | \"" + asmFilePathStr + "\" -ahlm - -o NUL 2>&1";

		FILE* pipe = _popen(cmd.c_str(), "r");
		if (!pipe) {
			std::cerr << "Error: 无法通过 _popen 启动子进程执行命令。" << std::endl;
			return "";
		}

		// 读取子进程（as 编译器）输出到 bytes
		constexpr size_t BUF_SIZE = 256;
		char buf[BUF_SIZE];
		bool firstLine = true;
		while (fgets(buf, BUF_SIZE, pipe) != nullptr) {
			if (firstLine) {
				firstLine = false;
				continue;
			}
			if (strlen(buf) == 0) { continue; }
			if (strlen(buf) == 1 && buf[0] == '\n') { continue; }
			if (strstr(buf, "Error")) {
				std::cerr << buf << std::endl;
				return {};
			}
			if (strstr(buf, "AARCH64 GAS")) { continue; }

			std::istringstream iss(buf);
			std::string col1, col2, col3;
			if (!(iss >> col1 >> col2 >> col3)) {
				continue;  // 少于 3 列就跳过
			}
			bytes += col3;
		}

		int returnCode = _pclose(pipe);
		if (returnCode != 0) {
			std::cerr << "Warning: 第 " << (i / GROUP_SIZE + 1)
				<< " 块汇编调用返回码为 " << returnCode << "。\n";
		}
	}
	return bytes;
}

static std::string Arm64AsmLabelToOffset(const std::string& asm_code, const char* end_label_name, const char* jump_label_name) {
	// 得到结尾位置
	std::string s = asm_code;
	size_t n = s.find(end_label_name);
	if (n == -1) {
		return s;
	}
	std::string before = s.substr(0, n);
	size_t end_back_idx_line = count_endl(before);
	replace_all_distinct(s, end_label_name, "");

	// 逐行切割
	std::vector<std::string> lines = splite_lines(s);

	// 替换每一行中的 #JUMP_END
	const std::string placeholder = jump_label_name;
	for (size_t idx = 0; idx < lines.size(); ++idx) {
		auto p = lines[idx].find(placeholder);
		if (p != std::string::npos) {
			int imm = (end_back_idx_line - idx) * 4;
			lines[idx].replace(p, placeholder.size(), std::to_string(imm));
		}
	}

	// 拼回去
	std::string out;
	for (size_t i = 0; i < lines.size(); ++i) {
		out += lines[i];
		out += "\n";
	}
	return out;
}
#endif /* ARM64_ASM_HELPER_H_ */
