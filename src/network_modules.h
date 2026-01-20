#ifndef NETWORK_MODULES_H
#define NETWORK_MODULES_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// --- 共享辅助函数声明 ---
char* wide_to_ansi(const wchar_t* wstr);
void gbk_to_wide(const char* gbk, wchar_t* buf, int bufLen);
int is_task_stopped(); 

// --- IPv4 模块 ---
void ipv4_init_qqwry();
void ipv4_cleanup_qqwry();
void ipv4_get_location(const char* ipStr, wchar_t* outBuf, int outLen);
int ipv4_ping_host(unsigned long ip, int retry, int timeout, long* outRtt, int* outTtl);
int ipv4_tcp_scan(unsigned long ip, int port, int timeout);
void ipv4_extract_search(const wchar_t* text, HWND hwnd, int showLocation);

// --- IPv6 模块 ---
int ipv6_ping_host(struct sockaddr_in6* dest, int retry, int timeout, long* outRtt, int* outTtl);
int ipv6_tcp_scan(struct sockaddr_in6* dest, int port, int timeout);
void ipv6_extract_search(const wchar_t* text, HWND hwnd);

// --- [新增] 域名模块接口 ---
// 从文本中提取域名 (例如: example.com, www.google.com)
void domain_extract_search(const wchar_t* text, HWND hwnd);

#endif // NETWORK_MODULES_H
