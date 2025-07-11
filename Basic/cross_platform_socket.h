#ifndef CROSS_PLATFORM_SOCKET_H
#define CROSS_PLATFORM_SOCKET_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET sock_t;
#define SOCK_INIT()    \
{ WSADATA wsaData; \
WSAStartup(MAKEWORD(2, 2), &wsaData); }
#define SOCK_CLEAN() WSACleanup()
#define SOCK_CLOSE closesocket
#define SOCK_ERROR WSAGetLastError()
#define SOCK_ERR_MSG "WSA Error"
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
typedef int sock_t;
#define SOCK_INIT() ((void)0)
#define SOCK_CLEAN() ((void)0)
#define SOCK_CLOSE close
#define SOCK_ERROR errno
#define SOCK_ERR_MSG strerror(errno)
#endif

#endif //CROSS_PLATFORM_SOCKET_H
