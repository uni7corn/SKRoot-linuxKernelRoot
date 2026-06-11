#pragma once
#include <iostream>
#include <set>
#include "patch_base.h"

class PatchFilldir64 : public PatchBase {
public:
	PatchFilldir64(const PatchBase& patch_base, uint64_t filldir64);
	~PatchFilldir64();

	KModErr patch_filldir64(uint64_t old_ino, uint64_t new_ino);
private:
	uint64_t m_filldir64 = 0;
};