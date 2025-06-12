#include "patch_freeze_task.h"
#include "analyze/base_func.h"
#include "3rdparty/aarch64_asm_helper.h"
using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;
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
	if (hook_func_start_addr == 0) { return 0; }
	std::cout << "Start hooking addr:  " << std::hex << hook_func_start_addr << std::endl << std::endl;

	size_t freeze_task_addr = m_sym.freeze_task;
	int atomic_usage_len = get_cred_atomic_usage_len();

	size_t freeze_task_entry_hook_jump_back_addr = freeze_task_addr + 4;

	aarch64_asm_info asm_info = init_aarch64_asm();
	auto& a = asm_info.a;
	Label label_end = a->newLabel();
	Label label_cycle_uid = a->newLabel();
	a->stp(x7, x8, ptr(sp).pre(-16));
	a->stp(x9, x10, ptr(sp).pre(-16));
	a->mov(x7, x0);
	for (auto x = 0; x < task_struct_offset_cred.size(); x++) {
		if (x != task_struct_offset_cred.size() - 1) {
			a->ldr(x7, ptr(x7, task_struct_offset_cred[x]));
		}
	}
	a->ldr(x7, ptr(x7, task_struct_offset_cred.back()));
	a->cbz(x7, label_end);
	a->add(x7, x7, Imm(atomic_usage_len));
	a->mov(x8, Imm(8));
	a->bind(label_cycle_uid);
	a->ldr(w9, ptr(x7).post(4));
	a->cbnz(w9, label_end);
	a->subs(x8, x8, Imm(1));
	a->b(CondCode::kNE, label_cycle_uid);
	a->ldp(x9, x10, ptr(sp).post(16));
	a->ldp(x7, x8, ptr(sp).post(16));
	a->mov(w0, wzr);
	a->ret(x30);
	a->bind(label_end);
	a->ldp(x9, x10, ptr(sp).post(16));
	a->ldp(x7, x8, ptr(sp).post(16));
	a->mov(x0, x0);
	aarch64_asm_b(a, (int32_t)(freeze_task_entry_hook_jump_back_addr - (hook_func_start_addr + a->offset())));
	std::cout << print_aarch64_asm(asm_info) << std::endl;
	std::string strBytes = aarch64_asm_to_bytes(asm_info);
	if (!strBytes.length()) {
		return 0;
	}
	size_t nHookFuncSize = strBytes.length() / 2;

	char hookOrigCmd[4] = { 0 };
	memcpy(&hookOrigCmd, (void*)((size_t)&m_file_buf[0] + freeze_task_addr), sizeof(hookOrigCmd));
	std::string strHookOrigCmd = bytes2hex((const unsigned char*)hookOrigCmd, sizeof(hookOrigCmd));

	int end_order_len = a->offset() - 2 * 4;
	strBytes = strBytes.substr(0, (end_order_len) * 2) + strHookOrigCmd + strBytes.substr((end_order_len + 4) * 2);

	vec_out_patch_bytes_data.push_back({ strBytes, hook_func_start_addr });

	aarch64_asm_info asm_info2 = init_aarch64_asm();
	aarch64_asm_b(asm_info2.a, (int32_t)(hook_func_start_addr - freeze_task_addr));
	std::string strBytes2 = aarch64_asm_to_bytes(asm_info2);
	if (!strBytes2.length()) {
		return 0;
	}

	vec_out_patch_bytes_data.push_back({ strBytes2, freeze_task_addr });
	hook_func_start_addr += nHookFuncSize;
	return hook_func_start_addr;
}
