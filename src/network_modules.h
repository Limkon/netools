#ifndef NETWORK_MODULES_H
#define NETWORK_MODULES_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// --- 共享辅助函数声明 (在 network_tools.c 中实现) ---
char* wide_to_ansi(const wchar_t* wstr);
void gbk_to_wide(const char* gbk, wchar_t* buf, int bufLen);
int is_task_stopped(); // 检查全局停止信号

// --- IPv4 模块接口 ---
void ipv4_init_qqwry();
void ipv4_cleanup_qqwry();
// 获取归属地 (仅支持 IPv4)
void ipv4_get_location(const char* ipStr, wchar_t* outBuf, int outLen);
// 执行 IPv4 Ping
// 返回: 1=在线, 0=超时/错误. 填充 rtt 和 ttl
int ipv4_ping_host(unsigned long ip, int retry, int timeout, long* outRtt, int* outTtl);
// 执行 IPv4 TCP 探测
int ipv4_tcp_scan(unsigned long ip, int port, int timeout);
// 从文本中提取 IPv4
void ipv4_extract_search(const wchar_t* text, HWND hwnd, int showLocation);


// --- IPv6 模块接口 ---
// 执行 IPv6 Ping
int ipv6_ping_host(struct sockaddr_in6* dest, int retry, int timeout, long* outRtt, int* outTtl);
// 执行 IPv6 TCP 探测
int ipv6_tcp_scan(struct sockaddr_in6* dest, int port, int timeout);
// 从文本中提取 IPv6
void ipv6_extract_search(const wchar_t* text, HWND hwnd);

#endif // NETWORK_MODULES_H
