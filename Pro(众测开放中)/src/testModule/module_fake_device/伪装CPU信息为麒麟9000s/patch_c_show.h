#pragma once
#include <iostream>
#include <set>
#include "patch_base.h"

class PatchCShow : public PatchBase {
public:
	PatchCShow(const PatchBase& patch_base, uint64_t c_show);
	~PatchCShow();

	KModErr patch_c_show(const std::string& fake_cpuinfo);
private:
	uint64_t m_c_show = 0;
};