#pragma once

#define PACKET_CONTENT_SIZE_MAX (1 << 21)
#define SESSIONS_COUNT_MAX      1024
#define SESSION_TTL_MAX         60 // seconds

#define INVALID_HANDLE_VALUE    ((HANDLE)(ulonglong)-1)
#define SOCKET_DATA_EMPTY       ((int)-1)
#define SOCKET_ERROR            ((int)-1)
#define CONNECTION_ERROR        SOCKET_ERROR

#define log(...) printf(__VA_ARGS__)

#if defined(_WIN32)
#define _last_error_code GetLastError()
#define socket_close() closesocket(sk)
#else
#define _last_error_code errno
#define socket_close close
#endif

typedef unsigned long long ulonglong;
typedef unsigned short     ushort;

typedef unsigned long long _socket;
typedef int                _connection;
typedef void*              HANDLE;

typedef char bool;

#define false 0
#define true  1

typedef enum
{
    _packet_file_none     = 0,
    _packet_file_download = 2,
    _packet_file_upload   = 4,
} _packet_type;

typedef struct
{
    size_t size;
    _packet_type type;
    char path[256u];
} _packet;

typedef struct {
    union {
        struct { unsigned char s_b1, s_b2, s_b3, s_b4; } S_un_b;
        struct { unsigned short s_w1, s_w2; } S_un_w;
        unsigned long S_addr;
    } S_un;
#define s_addr  S_un.S_addr /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2    // host on imp
#define s_net   S_un.S_un_b.s_b1    // network
#define s_imp   S_un.S_un_w.s_w2    // imp
#define s_impno S_un.S_un_b.s_b4    // imp #
#define s_lh    S_un.S_un_b.s_b3    // logical host
} _in_addr;

typedef struct
{
    short    sin_family;
    unsigned short sin_port;
    _in_addr sin_addr;
    char     sin_zero[8];
} _sockaddr_in;