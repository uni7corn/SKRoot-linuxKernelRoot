#include "patch_freeze_task.h"
#include "analyze/base_func.h"
#include "analyze/Arm64_asm.h"
PatchFreezeTask::PatchFreezeTask(const std::vector<char>& file_buf, const KernelSymbolOffset& sym,
	const SymbolAnalyze& symbol_analyze) : PatchBase(file_buf, sym, symbol_analyze) {

}

PatchFreezeTask::~PatchFreezeTask()
{
}

int PatchFreezeTask::get_need_read_cap_cnt() {
	int cnt = get_cap_cnt();
	if (cnt < 5) {
		cnt = 3;
	}
	return cnt;
}


size_t PatchFreezeTask::patch_freeze_task(size_t hook_func_start_addr, const std::vector<size_t>& task_struct_offset_cred,
	std::vector<patch_bytes_data>& vec_out_patch_bytes_data) {
	size_t freeze_task_addr = m_sym.freeze_task;
	int atomic_usage_len = get_cred_atomic_usage_len();

	size_t freeze_task_entry_hook_jump_back_addr = freeze_task_addr + 4;
	std::stringstream sstrAsm;
	sstrAsm
		<< "STP X7, X8, [sp, #-16]!" << std::endl
		<< "STP X9, X10, [sp, #-16]!" << std::endl;
	sstrAsm << "MOV X7, X0" << std::endl;
	for (auto x = 0; x < task_struct_offset_cred.size(); x++) {
		if (x != task_struct_offset_cred.size() - 1) {
			sstrAsm << "LDR X7, [X7, #" << task_struct_offset_cred[x] << "]" << std::endl;
		}
	}
	sstrAsm << "LDR X7, [X7, #" << task_struct_offset_cred[task_struct_offset_cred.size() - 1] << "]" << std::endl
		<< "CBZ X7, #JUMP_END" << std::endl
		<< "ADD X7, X7, #" << atomic_usage_len << std::endl
		<< "MOV X8, #8" << std::endl
		<< "LABEL_CYCLE_UID:"
		<< "LDR  W9, [X7], #4" << std::endl
		<< "CBNZ  W9, #JUMP_END" << std::endl
		<< "SUBS  X8, X8, #1" << std::endl
		<< "B.NE #JUMP_CYCLE_UID" << std::endl
		<< "LDP X9, X10, [sp], #16" << std::endl
		<< "LDP X7, X8, [sp], #16" << std::endl
		<< "MOV W0, WZR" << std::endl
		<< "RET" << std::endl
		<< "LABEL_END:"
		<< "LDP X9, X10, [sp], #16" << std::endl
		<< "LDP X7, X8, [sp], #16" << std::endl
		<< "MOV X0, X0" << std::endl;
		size_t end_order_len = count_endl(sstrAsm.str()) * 4;
		sstrAsm <<  "B #" << freeze_task_entry_hook_jump_back_addr - (hook_func_start_addr + end_order_len) << std::endl;

	std::string strAsmCode = Arm64AsmLabelToOffset(sstrAsm.str(), "LABEL_END:", "JUMP_END");
	strAsmCode = Arm64AsmLabelToOffset(strAsmCode, "LABEL_CYCLE_UID:", "JUMP_CYCLE_UID");
	std::cout << std::endl << strAsmCode << std::endl;

	std::string strBytes = Arm64AsmToBytes(strAsmCode);
	if (!strBytes.length()) {
		return 0;
	}
	size_t nHookFuncSize = strBytes.length() / 2;

	char hookOrigCmd[4] = { 0 };
	memcpy(&hookOrigCmd, (void*)((size_t)&m_file_buf[0] + freeze_task_addr), sizeof(hookOrigCmd));
	std::string strHookOrigCmd = bytes2hex((const unsigned char*)hookOrigCmd, sizeof(hookOrigCmd));

	end_order_len = (count_endl(sstrAsm.str()) - 2) * 4;
	strBytes = strBytes.substr(0, (end_order_len) * 2) + strHookOrigCmd + strBytes.substr((end_order_len + 4) * 2);

	vec_out_patch_bytes_data.push_back({ strBytes, hook_func_start_addr });

	std::stringstream sstrAsm2;
	sstrAsm2
		<< "B #" << (int64_t)(hook_func_start_addr - freeze_task_addr) << std::endl;
	std::string strBytes2 = Arm64AsmToBytes(sstrAsm2.str());
	if (!strBytes2.length()) {
		return 0;
	}
	vec_out_patch_bytes_data.push_back({ strBytes2, freeze_task_addr });
	hook_func_start_addr += nHookFuncSize;
	std::cout << "#下一段HOOK函数起始可写位置：" << std::hex << hook_func_start_addr << std::endl << std::endl;
	return hook_func_start_addr;
}
