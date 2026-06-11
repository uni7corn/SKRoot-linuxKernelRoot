#pragma once
#include <iostream>
#include <set>
#include "patch_base.h"

class PatchCompatFilldir : public PatchBase {
public:
	PatchCompatFilldir(const PatchBase& patch_base, uint64_t compat_filldir);
	~PatchCompatFilldir();

	KModErr patch_compat_filldir(uint64_t old_ino, uint64_t new_ino);
private:
	uint64_t m_compat_filldir = 0;
};