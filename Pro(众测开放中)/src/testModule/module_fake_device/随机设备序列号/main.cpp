#include <sys/wait.h>
#include "kernel_module_kit_umbrella.h"

static void run_script(const char* script) {
    const char* sh = "/system/bin/sh";
    char* const argv[] = {
        (char*)sh,
        (char*)script,
        nullptr
    };
    execve(sh, argv, environ);
}

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    pid_t pid = ::fork();
    if (pid == 0) {
        run_script("main.sh");
        _exit(127);
    }
    int status; waitpid(pid, &status, 0);
    return 0;
}

// SKRoot 模块名片
// 字段说明见 module_descriptor.h
SKROOT_MODULE_NAME("随机设备序列号")
SKROOT_MODULE_VERSION("0.0.1")
SKROOT_MODULE_DESC("随机伪装硬件序列号(ro.serialno)")
SKROOT_MODULE_AUTHOR("SKRoot & 蜃")
SKROOT_MODULE_ID32("0224718349d85a9c74200ec57ab7ac95")
SKROOT_MODULE_UPDATE_JSON("https://abcz316.github.io/SKRoot-linuxKernelRoot/module_fake_device/modify_ro_serialno_update.json")