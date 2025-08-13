#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

//#include "sockets.h"
#include <stdint.h>
#include <cstdarg>
#include <iostream>
#include "emul_fdset.h"

extern "C" {

int emul_socket(int domain, int type, int protocol) {
  return ::socket(domain, type, protocol);
}

int emul_bind(int s, uint32_t address, int port) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = port;
  addr.sin_addr.s_addr = htonl(address);

  return ::bind(s, (sockaddr*)&addr, sizeof(addr));
}

#ifdef _WIN32
int close(int fd) {
  // Try to call closesocket — if it's not a socket, fall back to _close
  int result = closesocket(fd);
  if (result == SOCKET_ERROR) {
    int err = WSAGetLastError();
    if (err == WSAENOTSOCK) {
      return 0;  // _close(fd); // it's a file descriptor
    } else {
      // some other socket error
      errno = err;  // or map to errno if needed
      return -1;
    }
  }
  return 0;
}
#endif

int fcntl(int fd, int cmd, ...) {
  va_list args;
  va_start(args, cmd);

  if (cmd == 4) {
    int arg = va_arg(args, int);
    if (arg == 0x4000) {
      u_long mode = 1;  // 1 to enable non-blocking socket
      ioctlsocket(fd, FIONBIO, &mode);
    }
  }

  // Use va_arg(args, int), etc.
  va_end(args);
  return 0;
}

typedef int _ssize_t;
typedef _ssize_t ssize_t;

ssize_t lwip_write(int s, const void* dataptr, size_t size) {
  return ::send(s, (const char*)dataptr, size, 0);
}

ssize_t lwip_read(int s, void* mem, size_t len) {
  auto recvStatus = ::recv(s, (char*)mem, len, 0);
  return recvStatus;
}

int emul_select(int __n, emul_fd_set* __readfds, emul_fd_set* __writefds, emul_fd_set* __exceptfds,
                struct timeval* __timeout) {
  struct fd_set readfds, writefds, exceptfds;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);

  if (__readfds) {
    for (int sock : __readfds->sockets) {
      FD_SET(sock, &readfds);
    }
  }

  if (__writefds) {
    for (int sock : __writefds->sockets) {
      FD_SET(sock, &writefds);
    }
  }

  if (__exceptfds) {
    for (int sock : __exceptfds->sockets) {
      FD_SET(sock, &exceptfds);
    }
  }

  auto retVal = select(__n, &readfds, &writefds, &exceptfds, __timeout);
  if (retVal < 0) {
    std::cerr << "select failed with error: " << strerror(errno) << std::endl;
    return retVal;
  }

  if (__readfds) {
    std::list<int> copy = __readfds->sockets;
    __readfds->sockets.clear();
    for (int sock : copy) {
      if (FD_ISSET(sock, &readfds)) {
        __readfds->sockets.push_back(sock);
      }
    }
  }

  if (__writefds) {
    std::list<int> copy = __writefds->sockets;
    __writefds->sockets.clear();
    for (int sock : copy) {
      if (FD_ISSET(sock, &writefds)) {
        __writefds->sockets.push_back(sock);
      }
    }
  }

  if (__exceptfds) {
    std::list<int> copy = __exceptfds->sockets;
    __exceptfds->sockets.clear();
    for (int sock : copy) {
      if (FD_ISSET(sock, &exceptfds)) {
        __exceptfds->sockets.push_back(sock);
      }
    }
  }

  return retVal;
}
}
