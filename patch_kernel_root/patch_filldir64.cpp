#include "patch_filldir64.h"
#include "analyze/base_func.h"
#include "3rdparty/aarch64_asm_helper.h"
using namespace asmjit;
using namespace asmjit::a64;
using namespace asmjit::a64::Predicate;
PatchFilldir64::PatchFilldir64(const std::vector<char>& file_buf, const KernelSymbolOffset& sym,
	const SymbolAnalyze& symbol_analyze) : PatchBase(file_buf, sym, symbol_analyze) {

}

PatchFilldir64::~PatchFilldir64()
{
}

size_t PatchFilldir64::patch_filldir64(size_t root_key_addr_offset, size_t hook_func_start_addr, std::vector<patch_bytes_data>& vec_out_patch_bytes_data) {
	if (hook_func_start_addr == 0) { return 0; }
	std::cout << "Start hooking addr:  " << std::hex << hook_func_start_addr << std::endl << std::endl;
	size_t filldir64_addr = m_sym.filldir64;

	size_t filldir64_entry_hook_jump_back_addr = filldir64_addr + 4;

	aarch64_asm_info asm_info = init_aarch64_asm();
	auto& a = asm_info.a;
	Label label_direct_end = a->newLabel();
	Label label_end = a->newLabel();
	Label label_cycle_name = a->newLabel();
	a->cmp(w2, Imm(16));
	a->b(CondCode::kNE, label_direct_end);
	a->stp(x7, x8, ptr(sp).pre(-16));
	a->stp(x9, x10, ptr(sp).pre(-16));
	int root_key_adr_offset = root_key_addr_offset - (hook_func_start_addr + a->offset());
	aarch64_asm_adr_x(a, x7, root_key_adr_offset);
	a->mov(x8, Imm(0));
	a->bind(label_cycle_name);
	a->ldrb(w9, ptr(x1, x8));
	a->ldrb(w10, ptr(x7, x8));
	a->cmp(w9, w10);
	a->b(CondCode::kNE, label_end);
	a->add(x8, x8, Imm(1));
	a->cmp(x8, Imm(16));
	a->b(CondCode::kLT, label_cycle_name);
	a->ldp(x9, x10, ptr(sp).post(16));
	a->ldp(x7, x8, ptr(sp).post(16));
	a->mov(x0, xzr);
	a->ret(x30);
	a->bind(label_end);
	a->ldp(x9, x10, ptr(sp).post(16));
	a->ldp(x7, x8, ptr(sp).post(16));
	a->bind(label_direct_end);
	a->mov(x0, x0);
	aarch64_asm_b(a, (int32_t)(filldir64_entry_hook_jump_back_addr - (hook_func_start_addr + a->offset())));
	std::cout << print_aarch64_asm(asm_info) << std::endl;
	std::string strBytes = aarch64_asm_to_bytes(asm_info);
	if (!strBytes.length()) {
		return 0;
	}
	size_t nHookFuncSize = strBytes.length() / 2;

	char hookOrigCmd[4] = { 0 };
	memcpy(&hookOrigCmd, (void*)((size_t)&m_file_buf[0] + filldir64_addr), sizeof(hookOrigCmd));
	std::string strHookOrigCmd = bytes2hex((const unsigned char*)hookOrigCmd, sizeof(hookOrigCmd));
	
	int end_order_len = a->offset() - 2 * 4;
	strBytes = strBytes.substr(0, (end_order_len) * 2) + strHookOrigCmd + strBytes.substr((end_order_len + 4) * 2);

	vec_out_patch_bytes_data.push_back({ strBytes, hook_func_start_addr });
	
	aarch64_asm_info asm_info2 = init_aarch64_asm();
	aarch64_asm_b(asm_info2.a, (int32_t)(hook_func_start_addr - filldir64_addr));
	std::string strBytes2 = aarch64_asm_to_bytes(asm_info2);
	if (!strBytes2.length()) {
		return 0;
	}
	vec_out_patch_bytes_data.push_back({ strBytes2, filldir64_addr });
	hook_func_start_addr += nHookFuncSize;
	return hook_func_start_addr;
}
