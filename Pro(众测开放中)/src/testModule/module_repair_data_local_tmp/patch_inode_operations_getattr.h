#pragma once
#include <iostream>
#include <set>
#include "patch_base.h"

struct InodePatchOffsets {
	uint32_t comm_offset = 0;
	uint32_t inode_i_ino = 0;
	uint32_t inode_i_sb = 0;
	uint32_t super_block_s_dev = 0;
};

class PatchInodeOperationsGetattr : public PatchBase {
public:
	PatchInodeOperationsGetattr(const PatchBase& patch_base, uint64_t inode_operations_getattr);
	~PatchInodeOperationsGetattr();

	KModErr patch_inode_operations_getattr(uint32_t kstat_ino_offset, uint64_t old_ino, uint64_t new_ino);
private:
	uint64_t m_inode_operations_getattr = 0;
};