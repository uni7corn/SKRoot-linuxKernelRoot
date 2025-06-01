#pragma once
#include "patch_do_execve.h"
#include "analyze/base_func.h"
#include "analyze/Arm64_asm.h"
PatchDoExecve::PatchDoExecve(const std::vector<char>& file_buf, const KernelSymbolOffset& sym,
	const SymbolAnalyze& symbol_analyze) : PatchBase(file_buf, sym, symbol_analyze) {

}
PatchDoExecve::~PatchDoExecve() {}

std::pair<size_t, size_t> PatchDoExecve::get_do_execve_param() {
	size_t do_execve_addr = 0;
	size_t do_execve_key_reg;
	if (m_symbol_analyze.is_kernel_version_less("3.19.0")) {
		do_execve_addr = m_sym.do_execve_common;
		do_execve_key_reg = 0;
	}
	else  if (m_symbol_analyze.is_kernel_version_less("4.18.0")) {
		do_execve_addr = m_sym.do_execveat_common;
		do_execve_key_reg = 1;
	}
	else if (m_symbol_analyze.is_kernel_version_less("5.9.0")) {
		do_execve_addr = m_sym.__do_execve_file;
		do_execve_key_reg = 1;
	}
	else {
		// default linux kernel useage
		do_execve_addr = m_sym.do_execveat_common;
		do_execve_key_reg = 1;
	}

	if (do_execve_addr == 0) {
		do_execve_addr = m_sym.do_execve;
		do_execve_key_reg = 0;
	}
	if (do_execve_addr == 0) {
		do_execve_addr = m_sym.do_execveat;
		do_execve_key_reg = 1;
	}
	return { do_execve_addr, do_execve_key_reg};
}

int PatchDoExecve::get_need_write_cap_cnt() {
	return get_cap_cnt();
}

size_t PatchDoExecve::patch_do_execve(const std::string& str_root_key, size_t hook_func_start_addr,
	const std::vector<size_t>& task_struct_offset_cred,
	const std::vector<size_t>& task_struct_offset_seccomp,
	std::vector<patch_bytes_data>& vec_out_patch_bytes_data) {

	auto [do_execve_addr, do_execve_key_reg] = get_do_execve_param();
	int atomic_usage_len = get_cred_atomic_usage_len();
	int securebits_padding = get_cred_securebits_padding();
	std::string cap_ability_max = get_cap_ability_max();
	int cap_cnt = get_need_write_cap_cnt();

	size_t do_execve_entry_hook_jump_back_addr = do_execve_addr + 4;

	std::string str_show_root_key_mem_byte = bytes2hex((const unsigned char*)str_root_key.c_str(), str_root_key.length());
	std::cout << "#生成的ROOT密匙字节集：" << str_show_root_key_mem_byte.c_str() << std::endl << std::endl;

	vec_out_patch_bytes_data.push_back({ str_show_root_key_mem_byte, hook_func_start_addr });

	size_t nHookFuncSize = str_root_key.length();
	hook_func_start_addr += nHookFuncSize;

	std::stringstream sstrAsm;

	sstrAsm
		<< "MOV X0, X0" << std::endl
		<< "STP X7, X8, [sp, #-16]!" << std::endl
		<< "STP X9, X10, [sp, #-16]!" << std::endl
		<< "STP X11, X12, [sp, #-16]!" << std::endl
		<< "MOV X7, 0xFFFFFFFFFFFFF001" << std::endl
		<< "CMP X" << do_execve_key_reg << ", X7" << std::endl
		<< "BCS #JUMP_END" << std::endl
		<< "LDR X7, [X" << do_execve_key_reg << "]" << std::endl
		<< "CBZ X7, #JUMP_END" << std::endl;
		size_t end_order_cnt = count_endl(sstrAsm.str());
		int key_offset = (hook_func_start_addr - 48) - (hook_func_start_addr + end_order_cnt * 4);
		sstrAsm << "ADR X8, #"<< key_offset << std::endl
		<< "MOV X9, #0" << std::endl
		<< "LABEL_CYCLE_NAME:"
		<< "LDRB W10, [X7, X9]" << std::endl
		<< "LDRB W11, [X8, X9]" << std::endl
		<< "CMP W10, W11" << std::endl
		<< "B.NE #JUMP_END" << std::endl
		<< "ADD X9, X9, 1" << std::endl
		<< "CMP X9, #" << str_root_key.length() << std::endl
		<< "BLT #JUMP_CYCLE_NAME" << std::endl
		<< "MRS X8, SP_EL0" << std::endl
		<< "MOV X10, X8" << std::endl;
		for (auto x = 0; x < task_struct_offset_cred.size(); x++) {
			if (x != task_struct_offset_cred.size() - 1) {
				sstrAsm << "LDR X10, [X10, #" << task_struct_offset_cred[x] << "]" << std::endl;
			}
		}
		sstrAsm << "LDR X10, [X10, #" << task_struct_offset_cred[task_struct_offset_cred.size() - 1] << "]" << std::endl
		<< "ADD X10, X10, #" << atomic_usage_len << std::endl
		<< "STR XZR, [X10], #8" << std::endl
		<< "STR XZR, [X10], #8" << std::endl
		<< "STR XZR, [X10], #8" << std::endl
		<< "STR XZR, [X10], #8" << std::endl
		<< "MOV W9, 0xC" << std::endl
		<< "STR W9, [X10], #"<< 4 + securebits_padding << std::endl
		<< "MOV X9, "<< cap_ability_max << std::endl 
		<< "STP X9, X9, [X10], #16" << std::endl
		<< "STP X9, X9, [X10], #16" << std::endl;
		if (cap_cnt == 5) {
			sstrAsm << "STR X9, [X10], #8" << std::endl;
		}
		sstrAsm  << "LDXR W10, [X8]" << std::endl
		<< "BIC W10, W10,#0xFFF" << std::endl
		<< "STXR W11, W10, [X8]" << std::endl
		<< "STR XZR, [X8, #" << task_struct_offset_seccomp[task_struct_offset_seccomp.size() - 1] << "]" << std::endl
		<< "LABEL_END:"
		<< "LDP X11, X12, [sp], #16" << std::endl
		<< "LDP X9, X10, [sp], #16" << std::endl
		<< "LDP X7, X8, [sp], #16" << std::endl;
		size_t end_order_len = count_endl(sstrAsm.str()) * 4;
		sstrAsm << "B #" << (int64_t)(do_execve_entry_hook_jump_back_addr - (hook_func_start_addr + end_order_len)) << std::endl;

	std::string strAsmCode = Arm64AsmLabelToOffset(sstrAsm.str(), "LABEL_END:", "JUMP_END");
	strAsmCode = Arm64AsmLabelToOffset(strAsmCode, "LABEL_CYCLE_NAME:", "JUMP_CYCLE_NAME");
	std::cout << std::endl << strAsmCode << std::endl;

	std::string strBytes = Arm64AsmToBytes(strAsmCode);
	if (!strBytes.length()) {
		return 0;
	}

	nHookFuncSize = strBytes.length() / 2;

	char hookOrigCmd[4] = { 0 };
	memcpy(&hookOrigCmd, (void*)((size_t)&m_file_buf[0] + do_execve_addr), sizeof(hookOrigCmd));
	std::string strHookOrigCmd = bytes2hex((const unsigned char*)hookOrigCmd, sizeof(hookOrigCmd));
	strBytes = strHookOrigCmd + strBytes.substr(0x4 * 2);

	vec_out_patch_bytes_data.push_back({ strBytes, hook_func_start_addr });

	std::stringstream sstrAsm2;
	sstrAsm2
		<< "B #" << (int64_t)(hook_func_start_addr - do_execve_addr) << std::endl;
	std::string strBytes2 = Arm64AsmToBytes(sstrAsm2.str());
	if (!strBytes2.length()) {
		return 0;
	}

	vec_out_patch_bytes_data.push_back({ strBytes2, do_execve_addr });

	hook_func_start_addr += nHookFuncSize;
	std::cout << "#下一段HOOK函数起始可写位置：" << std::hex << hook_func_start_addr << std::endl << std::endl;

	return hook_func_start_addr;
}