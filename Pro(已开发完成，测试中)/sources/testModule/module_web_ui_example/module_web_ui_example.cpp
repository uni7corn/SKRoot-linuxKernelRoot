#include "kernel_module_kit_umbrella.h"

// SKRoot模块入口函数
int skroot_module_main(const char* root_key, const char* module_private_dir) {
    printf("[module_web_ui_example] starting... \n");
    printf("[module_web_ui_example] root_key len=%zu\n", strlen(root_key));
    printf("[module_web_ui_example] module_private_dir=%s\n", module_private_dir);

    //读取配置文件内容
    std::string value;
    KModErr err = kernel_module::read_string_disk_storage(root_key, "myKey", value);
    if(is_ok(err)) {
        printf("[module_web_ui_example] read storage succeed: value: %s\n", value.c_str());
    } else {
        printf("[module_web_ui_example] read storage failed: %s\n", to_string(err).c_str());
    }
    return 0;
}


// WebUI HTTP服务器回调函数
class MyWebHttpHandler : public kernel_module::WebUIHttpHandler { // HTTP服务器基于civetweb库
public:
    void onPrepareCreate(const char* root_key, const char* module_private_dir, uint32_t port) override {
        printf("[module_web_ui_example] MyHttpHandler root_key len=%zu\n", strlen(root_key));
        printf("[module_web_ui_example] MyHttpHandler module_private_dir=%s\n", module_private_dir);
        printf("[module_web_ui_example] MyHttpHandler port=%d\n", port);
        m_root_key = root_key;
    }

    bool handleGet(CivetServer* server, struct mg_connection* conn) override {
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string query = req_info->query_string ? req_info->query_string : "";
        printf("GET request\nPath: %s\nQuery: %s\n", path.c_str(), query.c_str());
        return false;
    }

    bool handlePost(CivetServer* server, struct mg_connection* conn) override {
        char buf[1024] = {0}; mg_read(conn, buf, sizeof(buf) - 1);
        const struct mg_request_info* req_info = mg_get_request_info(conn);
        std::string path = req_info->local_uri ? req_info->local_uri : "/";
        std::string body(buf);
        printf("POST request\nPath: %s\nBody: %s\n", path.c_str(), body.c_str());

        std::string resp;
        if(path == "/getPid") resp = std::to_string(getpid());
        else if(path == "/getUid") resp = std::to_string(getuid());
        else if(path == "/getValue") kernel_module::read_string_disk_storage(m_root_key.c_str(), "myKey", resp);
        else if(path == "/setValue") resp = 
                is_ok(kernel_module::write_string_disk_storage(m_root_key.c_str(), "myKey", body.c_str())) ? "OK" : "FAILED";
        
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
SKROOT_MODULE_NAME("演示模块WebUI页面");
SKROOT_MODULE_VERSION("1.0.0");
SKROOT_MODULE_DESC("演示在模块中使用WebUI页面");
SKROOT_MODULE_AUTHOR("SKRoot官方教程");
SKROOT_MODULE_UUID32("6080b19fb2db26c534af3051103f541f");
SKROOT_MODULE_WEB_UI(MyWebHttpHandler);