#include <set>
#include <unordered_map>
#include <signal.h>

#include "main.h"
#include "cJSON.h"
#include "android_packages_list_utils.h"
#include "boot_session_utils.h"
#include "persist_data_perm_helper.h"
#include "pkg_process_helper.h"

static inline constexpr const char* kPrivateDirName = "empty_dir";

static fs::path g_empty_dir;
static bool g_need_reload = false;


// 把 ["aa","bb","cc"] 解析成 std::set<std::string>
static std::set<std::string> parse_json(const std::string& json) {
    std::set<std::string> result;
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) return result;
    if (root->type != cJSON_Array) {
        cJSON_Delete(root);
        return result;
    }
    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        if (!item) continue;
        if (item->type != cJSON_String) continue;
        if (!item->valuestring) continue;
        result.insert(std::string(item->valuestring));
    }
    cJSON_Delete(root);
    return result;
}

// 把 set<string> 导出为 JSON 数组字符串：["aa","bb","cc"]
static std::string json_array_from_set(const std::set<std::string>& s) {
    cJSON* arr = cJSON_CreateArray();
    for (const auto& k : s) {
        cJSON* str = cJSON_CreateString(k.c_str());
        cJSON_AddItemToArray(arr, str);
    }
    char* out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    if (!out) return "[]";
    std::string ret(out);
    cJSON_free(out);
    return ret;
}

// 扫描 /data/user/<uid>/ 里是否存在以 prefix 开头的包目录（例如 com.android.）
static bool has_pkg_dir_with_prefix_in_data_user(const std::string& prefix) {
    const fs::path user_root = "/data/user";
    std::error_code ec;

    if (!fs::exists(user_root, ec) || ec) return false;
    if (!fs::is_directory(user_root, ec) || ec) return false;

    // /data/user/<userId> 通常是数字目录：0, 10, 999...
    for (const auto& u : fs::directory_iterator(user_root, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) { ec.clear(); continue; }
        if (!u.is_directory(ec) || ec) { ec.clear(); continue; }

        const std::string uid = u.path().filename().string();
        if (!is_all_digits(uid)) continue;

        // 扫描 /data/user/<userId>/<pkgDir>
        std::error_code ec2;
        for (const auto& pkg : fs::directory_iterator(u.path(), fs::directory_options::skip_permission_denied, ec2)) {
            if (ec2) { ec2.clear(); break; }
            if (!pkg.is_directory(ec2) || ec2) { ec2.clear(); continue; }

            const std::string name = pkg.path().filename().string();
            if (name.rfind(prefix, 0) == 0) {
                return true; // 找到 com.android.* 目录
            }
        }
    }
    return false;
}

// 阻塞等待：每 interval 检测一次，直到满足条件后执行一次 on_decrypted
static void wait_decrypted_and_run(std::function<void()> on_decrypted,
                                  std::chrono::seconds interval = std::chrono::seconds(3),
                                  const std::string& prefix = "com.android.") {
    for (;;) {
        if (has_pkg_dir_with_prefix_in_data_user(prefix)) {
            // 解密成功：只执行一次
            on_decrypted();
            return;
        }
        std::this_thread::sleep_for(interval);
    }
}

static bool remove_ano_tmp_for_pkg(const std::string& pkg) {
    if (pkg.empty()) return false;

    const fs::path user_root = "/data/user";
    std::error_code ec;
    if (!fs::exists(user_root, ec) || !fs::is_directory(user_root, ec) || ec) return false;

    bool hit_any = false;
    for (const auto& entry : fs::directory_iterator(user_root, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) { ec.clear(); continue; }
        if (!entry.is_directory(ec)) { ec.clear(); continue; }

        const std::string uid = entry.path().filename().string();
        if (!is_all_digits(uid)) continue;

        fs::path ano_tmp = entry.path() / pkg / "files" / "ano_tmp";
        if (fs::exists(ano_tmp, ec) && !ec && fs::is_directory(ano_tmp, ec) && !ec) {
            hit_any = true;
            fs::remove_all(ano_tmp, ec);
            printf("remove ano_tmp success: %s\n", pkg.c_str());
        }
    }
    return hit_any;
}

static void cleanup() {
    // 等价于：
    // echo 16384 > /proc/sys/fs/inotify/max_queued_events
    // echo 128 > /proc/sys/fs/inotify/max_user_instances
    // echo 8192 > /proc/sys/fs/inotify/max_user_watches
    write_text_file("/proc/sys/fs/inotify/max_queued_events", "16384\n");
    write_text_file("/proc/sys/fs/inotify/max_user_instances", "128\n");
    write_text_file("/proc/sys/fs/inotify/max_user_watches", "8192\n");

    // 等价于：
    // rm -rf /data/system/dropbox/*
    // rm -rf /data/system/usagestats/*
    // rm -rf /data/system/appops.xml
    // rm -rf /data/system/ifw/*
    // rm -rf /data/system/ndebugsocket/*
    // rm -rf /data/anr/*
    // rm -rf /data/tombstones/*
    // rm -rf /data/misc/logd/*
    // rm -rf /data/misc/update_engine/log/*
    clear_directory_contents("/data/system/dropbox");
    clear_directory_contents("/data/system/usagestats");
    delete_path("/data/system/appops.xml");
    clear_directory_contents("/data/system/ifw");
    clear_directory_contents("/data/system/ndebugsocket");
    clear_directory_contents("/data/anr");
    clear_directory_contents("/data/tombstones");
    clear_directory_contents("/data/misc/logd");
    clear_directory_contents("/data/misc/update_engine/log");
}

static bool set_persist_data_locked_once(bool should_lock) {
    static bool s_locked = false;
    if (should_lock) {
        if (s_locked) return true;
        if (!persist_data_lock(g_empty_dir)) return false;
        cleanup();
        s_locked = true;
        return true;
    }

    if (!s_locked) return true;
    if (!persist_data_unlock(g_empty_dir)) return false;
    cleanup();
    s_locked = false;
    return true;
}

static void monitor_pkgs_loop(const std::set<std::string>& pkgs) {
    using namespace std::chrono_literals;
    static std::map<std::string, bool> s_pkg_remove_armed;
    for (;;) {
        bool any_running = false;
        for (const auto& pkg : pkgs) {
            const bool running = is_pkg_running(pkg, 600 * 1024); // 忽略600MB以下的
            if (running) {
                any_running = true;
                if(!s_pkg_remove_armed[pkg]) printf("detect running:%s\n", pkg.c_str());
                s_pkg_remove_armed[pkg] = true;
                continue;
            }
            bool& remove_armed = s_pkg_remove_armed[pkg];
            if (remove_armed) {
                printf("detect close:%s\n", pkg.c_str());
                remove_ano_tmp_for_pkg(pkg);
                remove_armed = false;
            }
        }
        set_persist_data_locked_once(any_running && !g_need_reload);
        if(g_need_reload) break;
        std::this_thread::sleep_for(3s);
    }
}

static void daemon_loop() {
    std::string json;
    kernel_module::read_string_disk_storage("target_pkg_json", json);
    std::set<std::string> target_pkg_list = parse_json(json);
    printf("target pkg list (%zd total) :\n", target_pkg_list.size());
    
    kernel_module::write_int32_disk_storage("pid", (int)getpid());
    kernel_module::write_string_disk_storage("boot_session", boot_session_utils::read_boot_session().c_str());

    wait_decrypted_and_run([&target_pkg_list]{
        printf("data user decrypted!\n");
        sleep(10);
        kill_pkg_list_until_stopped(target_pkg_list);
        for (const auto& pkg : target_pkg_list) {
            printf("target pkg: %s\n", pkg.c_str());
            remove_ano_tmp_for_pkg(pkg);
        }
        cleanup();
        monitor_pkgs_loop(target_pkg_list);
    });
}

static void on_sigusr1(int signo) {
    (void)signo;
    printf("recv signUser1: reload pkgs\n");
    g_need_reload = true;
}

static void register_sigusr1() {
    struct sigaction sa = {};
    sa.sa_handler = on_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, nullptr);
}

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    g_empty_dir = fs::path(module_private_dir) / kPrivateDirName;
    persist_data_unlock(g_empty_dir);
    spawn_delayed_task(5, [=] {
        register_sigusr1();
        persist_data_unlock(g_empty_dir);
        daemon_loop();
        while(g_need_reload) { g_need_reload = false; daemon_loop(); }
    });
    return 0;
}

void module_on_uninstall(const char* root_key, const char* module_private_dir) {
    g_empty_dir = fs::path(module_private_dir) / kPrivateDirName;
    persist_data_unlock(g_empty_dir);
}

static std::set<std::string> get_app_list() {
    std::unordered_map<std::string, uint32_t> app_list = android_pkgmap::read_all_pkg_uids_exclude_system();
    std::set<std::string> result;
    for (const auto& kv : app_list) {
        std::string_view pkg = kv.first;
        if (pkg.starts_with("com.")) {
            result.insert(kv.first);
        }
    }
    return result;
}

// WebUI HTTP服务器回调函数
class MyWebHttpHandler : public kernel_module::WebUIHttpHandler { // HTTP服务器基于civetweb库
public:
    bool handlePost(CivetServer* server, struct mg_connection* conn, const std::string& path, const std::string& body) override {
        printf("[module_hide_data_dir] POST request\nPath: %s\nBody: %s\n", path.c_str(), body.c_str());

        std::string resp;
        if(path == "/getCandidatesPkgJson") resp = json_array_from_set(get_app_list());
        else if(path == "/getTargetPkgJson") kernel_module::read_string_disk_storage("target_pkg_json", resp);
        else if(path == "/setTargetPkgJson") resp = onSetTargetPkgJson(body);
        kernel_module::webui::send_text(conn, 200, resp);
        return true;
    }

private:
    std::string onSetTargetPkgJson(const std::string& body) {
        std::string resp = is_ok(kernel_module::write_string_disk_storage("target_pkg_json", body.c_str())) ? "OK" : "FAILED";
        std::string boot_session;
        kernel_module::read_string_disk_storage("boot_session", boot_session);
        if(boot_session == boot_session_utils::read_boot_session()) {
            int32_t pid = 0;
            kernel_module::read_int32_disk_storage("pid", pid);
            if(pid > 0 && is_pid_root(pid)) {
                kill(pid, SIGUSR1);
            }
        }
        return resp;
    }
};

// SKRoot 模块名片
SKROOT_MODULE_NAME("防设备标记&自动清理")
SKROOT_MODULE_VERSION("4.0.2")
SKROOT_MODULE_DESC("需要手动添加包名")
SKROOT_MODULE_AUTHOR("SKRoot & 蜃 & Cycle1337")
SKROOT_MODULE_UUID32("Vk0EFJTuG2aBLQqc6WLHVPHnhfiZ8VKG")
SKROOT_MODULE_ON_UNINSTALL(module_on_uninstall)
SKROOT_MODULE_WEB_UI(MyWebHttpHandler)
SKROOT_MODULE_UPDATE_JSON("https://abcz316.github.io/SKRoot-linuxKernelRoot/module_fake_device/cycle1337_bypass_device_flag_update.json")