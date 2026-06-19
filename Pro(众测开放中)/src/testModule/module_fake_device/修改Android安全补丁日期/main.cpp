#include <unistd.h>
#include "android_system_property_utils.h"
#include "resetprop_helper.h"
#include "kernel_module_kit_umbrella.h"

static constexpr const char* k_security_patch_property_keys[] = {
    "ro.build.version.security_patch",
    "ro.vendor.build.security_patch",
    "ro.system.build.version.security_patch",
    "ro.product.build.version.security_patch",
    "ro.system_ext.build.version.security_patch",
    "ro.odm.build.version.security_patch",
    "ro.odm.build.security_patch",
    "ro.bootimage.build.version.security_patch",
    "ro.vendor_dlkm.build.version.security_patch",
    "ro.odm_dlkm.build.version.security_patch",
    "ro.system_dlkm.build.version.security_patch",
};

static bool set_security_patch_date(const char* key, const char* new_date, std::string* old_value_out = nullptr) {
    if (!key || !*key) return false;
    if (!new_date || !*new_date) return false;

    const std::string old_value = get_system_property(key);
    if (old_value_out) *old_value_out = old_value;

    // 属性不存在，跳过，不打印
    if (old_value.empty()) return false;

    // 已经是目标值，跳过，不打印
    if (old_value == new_date) return false;

    return resetprop::set_property_value(key, new_date, ResetPropMode::kNoTrigger);
}

static void set_all_security_patch_dates(const char* new_date) {
    for (const char* key : k_security_patch_property_keys) {
        std::string old_value;
        const bool written = set_security_patch_date(key, new_date, &old_value);
        if (written) printf("[security_patch] %s: %s => %s\n", key, old_value.c_str(), new_date);
    }
}

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    std::string resetprop_bin_path = std::string(module_private_dir) + "resetprop";
    resetprop::init(resetprop_bin_path);

    std::string security_patch_date;
    kernel_module::read_string_disk_storage("security_patch_date", security_patch_date);
    printf("security_patch_date:%s\n", security_patch_date.c_str());
    if(security_patch_date.empty()) return 0;

    set_all_security_patch_dates(security_patch_date.c_str());
    return 0;
}

static std::string getSecurityPatchDate() {
    for (const char* key : k_security_patch_property_keys) {
        const std::string old_value = get_system_property(key);
        if(!old_value.empty()) return old_value;
    }
    return {};
}

// WebUI HTTP服务器回调函数
class MyWebHttpHandler : public kernel_module::WebUIHttpHandler { // HTTP服务器基于civetweb库
public:
    // 这里的Web服务器仅起到读取、保存配置文件的作用。
    bool handlePost(CivetServer* server, struct mg_connection* conn, const std::string& path, const std::string& body) override {
        std::string resp;
        if(path == "/getSecurityPatchDate") resp = getSecurityPatchDate();
        else if(path == "/setSecurityPatchDate") resp = is_ok(kernel_module::write_string_disk_storage("security_patch_date", body.c_str())) ? "OK" : "FAILED";
        kernel_module::webui::send_text(conn, 200, resp);
        return true;
    }
};

// SKRoot 模块名片
// 字段说明见 module_descriptor.h
SKROOT_MODULE_NAME("修改Android安全补丁日期")
SKROOT_MODULE_VERSION("1.0.0")
SKROOT_MODULE_DESC("一键修改 Android 安全补丁日期，自动同步多个系统分区相关属性")
SKROOT_MODULE_AUTHOR("SKRoot")
SKROOT_MODULE_ID32("nv2ZfKbBpBYxYvFqwgcGHEvQkjFjaRvS")
SKROOT_MODULE_WEB_UI(MyWebHttpHandler)
SKROOT_MODULE_UPDATE_JSON("https://abcz316.github.io/SKRoot-linuxKernelRoot/module_fake_device/sk_fake_security_date_update.json")