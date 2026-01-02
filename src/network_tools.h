#ifndef NETWORK_TOOLS_H
#define NETWORK_TOOLS_H

// 包含顺序必须正确
#include <winsock2.h>
#include <windows.h>
#include <tchar.h> // 支持宽字符宏

// 消息定义
#define WM_USER_LOG     (WM_USER + 100) 
#define WM_USER_RESULT  (WM_USER + 101) 
#define WM_USER_FINISH  (WM_USER + 102) 

typedef enum {
    TASK_PING = 1,
    TASK_SCAN,
    TASK_SINGLE_SCAN,
    TASK_EXTRACT
} TaskType;

// 参数结构体改为宽字符
typedef struct {
    HWND hwndNotify;
    wchar_t* targetInput;  // 宽字符输入
    wchar_t* portsInput;   // 宽字符端口
    int retryCount;
    int timeoutMs;
} ThreadParams;

// 代理管理 (输入为宽字符，内部转 ANSI)
int proxy_set_system(const wchar_t* ip, int port);
int proxy_unset_system();
void proxy_init_backup(); 

// 线程入口
unsigned int __stdcall thread_ping(void* arg);
unsigned int __stdcall thread_port_scan(void* arg);
unsigned int __stdcall thread_single_scan(void* arg);
unsigned int __stdcall thread_extract_ip(void* arg);

void free_thread_params(ThreadParams* params);

#endif // NETWORK_TOOLS_H
