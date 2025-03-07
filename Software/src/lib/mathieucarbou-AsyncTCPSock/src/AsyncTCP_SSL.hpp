#ifndef ASYNCTCP_SSL_HPP
#define ASYNCTCP_SSL_HPP

#ifdef ASYNC_TCP_SSL_ENABLED

#include <AsyncTCP.h>

#define AsyncSSLClient AsyncClient
#define AsyncSSLServer AsyncServer

#define ASYNC_TCP_SSL_VERSION             "AsyncTCPSock SSL shim v0.0.1"

#else
#error Compatibility shim requires ASYNC_TCP_SSL_ENABLED to be defined!
#endif

#endif