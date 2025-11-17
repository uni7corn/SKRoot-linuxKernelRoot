#pragma once
#include <iostream>
#include <vector>
#include "kernel_module_kit_umbrella.h"

class PatchBase {
public:
	PatchBase(const char* root_key);
	PatchBase(const PatchBase& other);
	~PatchBase();
protected:
	std::string get_root_key();
	KModErr patch_kernel_before_hook(uint64_t kernel_addr, const aarch64_asm_info& asm_info);
	KModErr patch_kernel_after_hook(uint64_t kernel_addr, const aarch64_asm_info& asm_info);

private:
	std::string m_root_key;
};
