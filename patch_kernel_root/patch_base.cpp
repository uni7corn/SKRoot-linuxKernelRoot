#include "patch_base.h"
#include "analyze/Arm64_asm.h"
PatchBase::PatchBase(const std::vector<char>& file_buf, const KernelSymbolOffset& sym,
	const SymbolAnalyze& symbol_analyze) : m_file_buf(file_buf), m_sym(sym), m_symbol_analyze(symbol_analyze) {

}

PatchBase::~PatchBase()
{
}

int PatchBase::get_cred_atomic_usage_len() {
	int len = 8;
	if (m_symbol_analyze.is_kernel_version_less("6.6.0")) {
		len = 4;
	}
	return len;
}

int PatchBase::get_cred_securebits_padding() {
	if (get_cred_atomic_usage_len() == 8) {
		return 4;
	}
	return 0;
}

std::string PatchBase::get_cap_ability_max() {
	std::string cap;
	if (m_symbol_analyze.is_kernel_version_less("5.8.0")) {
		cap = "0x3FFFFFFFFF";
	}
	else if (m_symbol_analyze.is_kernel_version_less("5.9.0")) {
		cap = "0xFFFFFFFFFF";
	}
	else {
		cap = "0x1FFFFFFFFFF";
	}
	return cap;
}

int PatchBase::get_cap_cnt() {
	int cnt = 0;
	if (m_symbol_analyze.is_kernel_version_less("4.3.0")) {
		cnt = 4;
	} else {
		cnt = 5;
	}
	return cnt;
}