#pragma once
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "../module_base_err_def.h"
namespace kernel_module {
inline KModErr alloc_kernel_mem(const char* root_key, const std::vector<uint8_t>& initial_bytes, uint64_t& out_kaddr) {
    KModErr alloc_kernel_mem_and_fill(const char* root_key, const uint8_t*buf, uint32_t size, uint64_t& out_kaddr);
    return alloc_kernel_mem_and_fill(root_key, initial_bytes.data(), initial_bytes.size(), out_kaddr);
}

inline KModErr execute_kernel_asm_func(const char* root_key, const std::vector<uint8_t>& func_bytes, uint64_t& output_result) {
    KModErr execute_kernel_asm_func_with_buf(const char* root_key, const uint8_t*shellcode, uint32_t size, uint64_t& out_kaddr);
    return execute_kernel_asm_func_with_buf(root_key, func_bytes.data(), func_bytes.size(), output_result);
}

inline KModErr read_string_disk_storage(const char* root_key, const char* key, std::string& out) {
    thread_local std::string* tls_out = nullptr;
    auto cb = [](const char* out_str) {
        if (!tls_out || !out_str) return;
        (*tls_out) = out_str;
    };
    tls_out = &out;
    KModErr read_string_disk_storage_with_cb(const char* root_key, const char* key, void (*cb)(const char* out_str));
    KModErr err = read_string_disk_storage_with_cb(root_key, key, cb);
    tls_out = nullptr; 
    return err;
}

inline KModErr read_blob_disk_storage(const char* root_key, const char* key, std::vector<uint8_t>& out) {
    thread_local std::vector<uint8_t>* tls_out = nullptr;
    auto cb = [](const void* data, uint64_t size) {
        if (!tls_out || !data) return;
        const uint8_t* p = static_cast<const uint8_t*>(data);
        tls_out->assign(p, p + size);
    };
    tls_out = &out;
    KModErr read_blob_disk_storage_with_cb(const char* root_key, const char* key, void (*cb)(const void* data, uint64_t size));
    KModErr err = read_blob_disk_storage_with_cb(root_key, key, cb);
    tls_out = nullptr; 
    return err;
}

inline KModErr install_kernel_function_before_hook(const char* root_key, uint64_t kernel_func_addr, const std::vector<uint8_t>& my_shellcode_func) {
    KModErr install_kfunc_before_hook_with_buf(const char* root_key, uint64_t hook_kernel_addr, const void *shellcode, uint32_t shellcode_len);
    return install_kfunc_before_hook_with_buf(root_key, kernel_func_addr, my_shellcode_func.data(), my_shellcode_func.size());
}

inline KModErr install_kernel_function_after_hook(const char* root_key, uint64_t kernel_func_addr, const std::vector<uint8_t>& my_shellcode_func) {
    KModErr install_kfunc_after_hook_with_buf(const char* root_key, uint64_t hook_kernel_addr, const void *shellcode, uint32_t shellcode_len);
    return install_kfunc_after_hook_with_buf(root_key, kernel_func_addr, my_shellcode_func.data(), my_shellcode_func.size());
}
}