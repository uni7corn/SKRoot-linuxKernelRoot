#include "patch_blkdev_open.h"
#include <unistd.h>
#include <cstdio>

#include "kernel_module_kit_umbrella.h"
#include "process_utils.h"
#include "block_device_helper.h"

// 开始修补内核
static KModErr patch_kernel_handler(const std::vector<block_device_helper::DevNodeInfo>& dev_list, const std::string& test_comm_name, uint64_t control_kaddr) {
    uint64_t blkdev_open_sym = 0;
    RETURN_IF_ERROR(kernel_module::kallsyms_lookup_name("blkdev_open", blkdev_open_sym));
    printf("blkdev_open(sym) addr: %p\n", (void*)blkdev_open_sym);

    uint32_t comm_offset = 0;
    RETURN_IF_ERROR(kernel_module::get_task_struct_comm_offset(comm_offset));
    printf("comm offset: 0x%x\n", comm_offset);

    BlkdevOpenPatchOffsets off = {};
    RETURN_IF_ERROR(kernel_module::get_file_f_mode_offset(off.file_f_mode));
    printf("f_mode offset: 0x%x\n", off.file_f_mode);

    RETURN_IF_ERROR(kernel_module::get_inode_i_rdev_offset(off.inode_i_rdev));
    printf("i_rdev offset: 0x%x\n", off.inode_i_rdev);

    PatchBase patchBase(comm_offset);
    PatchBlkdevOpen patchBlkdevOpen(patchBase, blkdev_open_sym);
    KModErr err = patchBlkdevOpen.patch_blkdev_open(dev_list, test_comm_name, control_kaddr, off);
    printf("patch blkdev_open addr: %p ret: %s\n", (void*)blkdev_open_sym, to_string(err).c_str());
    RETURN_IF_ERROR(err);
    return KModErr::OK;
}

static uint64_t alloc_control_kaddr() {
    uint64_t control_kaddr = 0;
    if(is_ok(kernel_module::alloc_kernel_mem(1, control_kaddr))) {
        kernel_module::fill00_kernel_mem(control_kaddr, 1);
    }
    return control_kaddr;
}

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    std::string new_comm = process_utils::reset_random_process_name();
    if(new_comm.empty()) {
        printf("reset process name failed\n");
        return -1;
    }
    uint64_t control_kaddr = alloc_control_kaddr();
    if(!control_kaddr) {
        printf("alloc control kaddr failed\n");
        return -1;
    }

    std::vector<block_device_helper::DevNodeInfo> dev_list;
    if (!block_device_helper::build_protected_device_list(dev_list)) {
        printf("build_protected_device_list failed\n");
        return -1;
    }
    KModErr err = patch_kernel_handler(dev_list, new_comm, control_kaddr);
    printf("patch_kernel_handler ret: %s\n", to_string(err).c_str());
    if (!is_ok(err)) {
        printf("patch_kernel_handler failed\n");
        return -1;
    }
    if (!block_device_helper::verify_protected_devices_not_writable(dev_list)) {
        printf("patch blkdev_open verify failed: protected block device still writable-openable\n");
        return -1;
    }
    printf("verify protect: success\n");
    process_utils::fork_delayed_task(40, [=] {
        char ch = '\x01';
        kernel_module::write_kernel_mem(control_kaddr, &ch, 1);
        printf("[module_protect_device] enable switch\n");
    });
    return 0;
}

// SKRoot 模块名片
// 字段说明见 module_descriptor.h
SKROOT_MODULE_NAME("防格机 (防变黑砖)")
SKROOT_MODULE_VERSION("1.0.2")
SKROOT_MODULE_DESC("内核级防黑砖保护，拦截恶意程序对核心分区的写入，防止设备变黑砖。开机1分钟后保护生效。需提前手动备份persist文件夹(路径：/mnt/vendor/persist/)，双清后可恢复正常开机。")
SKROOT_MODULE_AUTHOR("SKRoot")
SKROOT_MODULE_ID32("BbA6MdJYoS2ggs58zU327m5ufBihCOrS")
SKROOT_MODULE_UPDATE_JSON("https://abcz316.github.io/SKRoot-linuxKernelRoot/module_protect_device/update.json")