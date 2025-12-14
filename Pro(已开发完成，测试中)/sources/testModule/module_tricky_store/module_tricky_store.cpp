#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "module_tricky_store.h"
#include "kernel_module_kit_umbrella.h"

#define TRICKY_STORE_VERSION "v1.4.1"

#define TARGET_TS_DIR "/data/adb/"
#define TARGET_TXT "/data/adb/tricky_store/target.txt"
#define KEYBOX_XML "/data/adb/tricky_store/keybox.xml"
#define TEE_STATUS "/data/adb/tricky_store/tee_status"
#define SERVICE_SH "/data/adb/modules/tricky_store/service.sh"

bool is_first_run(const std::string& root_key) {
    bool first_run = true;
    kernel_module::read_bool_disk_storage(root_key.c_str(), "is_first_run", first_run);
    return first_run;
}

bool set_first_run(const std::string& root_key, bool first_run) {
    return is_ok(kernel_module::write_bool_disk_storage(root_key.c_str(), "is_first_run", first_run));
}

bool write_target_txt_applist() {
    std::vector<std::string> packageNames;
    std::string packages = run_cmd("pm list packages -3");
    std::istringstream iss(packages);
    std::string line;
    while (getline(iss, line)) {
        size_t pos = line.find("package:");
        if (pos != std::string::npos) {
            line.erase(pos, std::string("package:").length());
        }
        packageNames.push_back(line);
    }
    printf("thrid package count: %zu\n", packageNames.size());
    std::stringstream sstr;
    for(auto & pkg : packageNames) sstr << pkg << "\n";
    return write_text_file(TARGET_TXT, sstr.str());
}

std::string get_target_txt() {
    std::string text;
    read_text_file(TARGET_TXT, text);
    return text;
}

bool set_target_txt(const std::string& text) {
    return write_text_file(TARGET_TXT, text);
}

std::string get_keybox_xml() {
    std::string text;
    read_text_file(KEYBOX_XML, text);
    return text;
}

bool set_keybox_xml(const std::string& text) {
    return write_text_file(KEYBOX_XML, text);
}

bool get_auto_third_app_toggle(const std::string& root_key) {
    bool enable = false;
    kernel_module::read_bool_disk_storage(root_key.c_str(), "auto_third_app_toggle", enable);
    return enable;
}

bool set_auto_third_app_toggle(const std::string& root_key, bool enable) {
    if(enable) {
        write_target_txt_applist();
    } else {
        set_target_txt("");
    }
    return is_ok(kernel_module::write_bool_disk_storage(root_key.c_str(), "auto_third_app_toggle", enable));
}

bool get_fix_tee_toggle() {
    std::string text;
    read_text_file(TEE_STATUS, text);
    return text == "teeBroken=true";
}

bool set_fix_tee_toggle(bool enable) {
    return write_text_file(TEE_STATUS, enable ? "teeBroken=true" : "teeBroken=false");
}

int skroot_module_main(const char* root_key, const char* module_private_dir) {
    printf("Hello! module_tricky_store!\n");

    bool first_run = is_first_run(root_key);
    bool auto_third_app_toggle = get_auto_third_app_toggle(root_key);
    bool tee_fix_toggle = get_fix_tee_toggle();
    printf("first_run: %d\n", !!first_run);
    printf("auto_third_app_toggle: %d\n", !!auto_third_app_toggle);
    printf("tee_fix_toggle: %d\n", !!tee_fix_toggle);

    std::string old_target_txt = get_target_txt();
    std::string old_keybox_xml = get_keybox_xml();

    std::string my_ts_path = std::string(module_private_dir) + "TS/";
    copy_dir(my_ts_path, TARGET_TS_DIR);
    chmod_tree_777(TARGET_TS_DIR);

    if(!first_run) {
        set_fix_tee_toggle(tee_fix_toggle);
        set_target_txt(old_target_txt);
        set_keybox_xml(old_keybox_xml);
    }
    set_first_run(root_key, false);

    pid_t pid = fork();
    if (pid == 0) {
        sleep(5);
        if(auto_third_app_toggle) {
            bool ok = write_target_txt_applist();
            printf("[module_tricky_store] write target.txt applist: %s\n", ok ? "success" : "failed");
        }
        
        printf("[module_tricky_store] run_script: %s\n", SERVICE_SH);
        run_script(SERVICE_SH);
        _exit(127);
    }
    return 0;
}

// WebUI HTTP服务器回调函数
class MyWebHttpHandler : public kernel_module::WebUIHttpHandler { // HTTP服务器基于civetweb库
public:
    void onPrepareCreate(const char* root_key, const char* module_private_dir, uint32_t port) override {
        m_root_key = root_key;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn) override {
        char buf[4096] = {0}; mg_read(conn, buf, sizeof(buf) - 1);
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string body(buf);
        printf("[module_tricky_store] POST request\nPath: %s\nBody: %s\n", path.c_str(), body.c_str());

        std::string resp;
        if(path == "/getVersion") resp = TRICKY_STORE_VERSION;
        else if(path == "/getTargetTxt") resp = get_target_txt();
        else if(path == "/setTargetTxt") resp = set_target_txt(body) ? "OK" : "FAILED";
        else if(path == "/getKeyboxXml") resp = get_keybox_xml();
        else if(path == "/setKeyboxXml") resp = set_keybox_xml(body) ? "OK" : "FAILED";
        else if(path == "/getAutoThirdAppToggle") resp = get_auto_third_app_toggle(m_root_key) ? "1" : "0";
        else if(path == "/setAutoThirdAppToggle") resp = set_auto_third_app_toggle(m_root_key, body == "1") ? "OK" : "FAILED";
        else if(path == "/getFixTeeToggle") resp = get_fix_tee_toggle() ? "1" : "0";
        else if(path == "/setFixTeeToggle") resp = set_fix_tee_toggle(body == "1") ? "OK" : "FAILED";

        mg_printf(conn,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/plain\r\n"
                  "Connection: close\r\n\r\n%s",
                  resp.c_str());
        return true;
    }
private:
    std::string m_root_key;
};

// SKRoot 模块名片
SKROOT_MODULE_NAME("Tricky Store");
SKROOT_MODULE_VERSION("1.4.1-a");
SKROOT_MODULE_DESC("提供系统证书接管与 TEE 状态修复能力。");
SKROOT_MODULE_AUTHOR("5ec1cff, aviraxp and Cyberenchanter");
SKROOT_MODULE_UUID32("c3a70f603b48380a611131d29c50aac3");
SKROOT_MODULE_WEB_UI(MyWebHttpHandler)
SKROOT_MODULE_UPDATE_JSON("https://abcz316.github.io/SKRoot-linuxKernelRoot/module_tricky_store/update.json")