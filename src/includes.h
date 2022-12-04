#pragma once

#if defined(_WIN32)
# pragma comment(lib, "Ws2_32.lib")
# define _WINSOCK_DEPRECATED_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
# pragma warning(disable : 6011 6387)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
# include <WinSock2.h>
  
# include <stdio.h>
#else
# include <pthread.h>
# include <unistd.h>
# include <string.h>
# include <stdint.h>
# include <errno.h>
# include <dirent.h> 

# include <sys/time.h>
# include <sys/types.h>
# include <sys/socket.h>
  
# include <netinet/in.h>
# include <netinet/tcp.h>
#endif

#include "defines.h"
#include "crc32.h"

#if defined(_WIN64)
# include "windows/directory.h"
#else
# include "linux/directory.h"
#endif