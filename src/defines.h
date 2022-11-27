#pragma once

#define PACKET_CONTENT_SIZE_MAX (1 << 21)
#define SESSIONS_COUNT_MAX      1024
#define SESSION_TTL_MAX         60 // seconds

#define INVALID_HANDLE_VALUE    ((HANDLE)(ulonglong)-1)
#define SOCKET_DATA_EMPTY       ((int)-1)
#define SOCKET_ERROR            ((int)-1)
#define CONNECTION_ERROR        SOCKET_ERROR

#define dprintf(...)		 printf(__VA_ARGS__)
#define dp_success(...)		 printf("[+] " __VA_ARGS__)
#define dp_error(...)		 printf("[-] " __VA_ARGS__)
#define dp_warning(...)		 printf("[!] " __VA_ARGS__)
#define dp_unknown(...)		 printf("[?] " __VA_ARGS__)

#define dp_status(expr, ...) \
	((expr) ? dp_success(__VA_ARGS__) : dp_error(__VA_ARGS__))

#define __min(a, b) (a < b ? a : b)
#define __max(a, b) (a > b ? a : b)

#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#define _countof(A) (sizeof(A)/sizeof((A)[0]))

#if defined(_WIN32)
#define _last_error_code GetLastError()
#define socket_close closesocket
#define sleep Sleep
#define sprintf_s_ sprintf_s
#define thread_create(arg0, arg1) \
	{ HANDLE hthread = CreateThread(NULL, NULL, (unsigned long(*)(void*))(arg0), arg1, NULL, NULL); CloseHandle(hthread); }
#else
#define _last_error_code errno
#define socket_close close
#define sleep usleep
#define sprintf_s_ snprintf
#define thread_create(arg0, arg1) \
	{ pthread_t pThread; pthread_create(&pThread, NULL, (void*(*)(void*))(arg0), arg1); }
#endif

typedef unsigned long long ulonglong;
typedef unsigned short     ushort;

typedef unsigned long long _socket;
//typedef int                _connection;
typedef void*              HANDLE;
typedef int                _client_id;

#define bool unsigned char

#define false 0
#define true  1

typedef enum
{
    _packet_file_none      = 0,
    _packet_file_download  = 2,
    _packet_file_upload    = 4,
    _packet_list_directory = 8,
} _packet_type;

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    size_t size;
    _packet_type type;
    int client_id;
    char path[256u];
} _packet;
#pragma pack(pop)

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

#define ETHERNET_MTU_SIZE						1500u
#define CTCP_PAYLOAD_SIZE						(ETHERNET_MTU_SIZE - sizeof(_ctcp_header))
#define CTCP_CONNECTIONS_MAX					5u
#define CTCP_CONNECTION_FRAMES_MAX				256u
#define CTCP_RECEIVE_PACKETS_COUNT_PER_FRAME	256u

#if defined(_DEBUG)
# define CTCP_CONNECTION_TIMEOUT_SECONDS_MAX	99999u
#else
# define CTCP_CONNECTION_TIMEOUT_SECONDS_MAX	4u
#endif

#if defined (_MSC_VER)
# define static_assert_(...) static_assert(__VA_ARGS__)
#else
# define static_assert_(...)
#endif

typedef unsigned long long  uint64;
typedef unsigned long	    uint32;
typedef uint64              _time;
typedef uint32				_guid;
typedef uint32				_index32;
typedef uint32				_count32;
typedef uint64				_flags;
typedef uint32				_checksum;
typedef uint32				_size;

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	_checksum checksum;
	_guid	  guid;
	uint32    payload_size;
	uint32    payload_size_total;
	union
	{
		struct
		{
			bool ACK : 1;
		};
		_flags flags;
	};
} _ctcp_header;

typedef struct
{
	_ctcp_header header;
	char payload[CTCP_PAYLOAD_SIZE];
} _ctcp_packet;

static_assert_(sizeof(_ctcp_packet) <= ETHERNET_MTU_SIZE, "packet size exceeds allowed transmission size");

typedef struct
{
	_count32	 count;
	_index32	 rindex;
	_index32	 windex;
	_ctcp_packet queue[CTCP_CONNECTION_FRAMES_MAX];
} _frames_queue;

typedef struct
{
	_guid		 guid;
	_socket		 socket;
	_time		 ping_last;
	_sockaddr_in sockaddr_in;
	_ctcp_packet packet;

	_frames_queue input;
	_frames_queue output;
} _connection;
#pragma pack(pop)
