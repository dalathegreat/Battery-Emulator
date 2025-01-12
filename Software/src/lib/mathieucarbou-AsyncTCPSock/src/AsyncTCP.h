/*
  Reimplementation of an asynchronous TCP library for Espressif MCUs, using
  BSD sockets.

  Copyright (c) 2020 Alex Villacís Lasso.

  Original AsyncTCP API Copyright (c) 2016 Hristo Gochkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ASYNCTCP_H_
#define ASYNCTCP_H_

#include "../../../system_settings.h"
#include "../../../devboard/hal/hal.h"

#include "IPAddress.h"
#include "sdkconfig.h"
#include <functional>
#include <deque>
#include <list>
#if ASYNC_TCP_SSL_ENABLED
#include <ssl_client.h>
#include "AsyncTCP_TLS_Context.h"
#endif

extern "C" {
    #include "lwip/err.h"
    #include "lwip/sockets.h"
}

#define ASYNCTCP_VERSION "1.0.2-dev"
#define ASYNCTCP_VERSION_MAJOR 1
#define ASYNCTCP_VERSION_MINOR 2
#define ASYNCTCP_VERSION_REVISION 2
#define ASYNCTCP_FORK_mathieucarbou

//If core is not defined, then we are running in Arduino or PIO
#ifndef CONFIG_ASYNC_TCP_RUNNING_CORE
#define CONFIG_ASYNC_TCP_RUNNING_CORE WIFI_CORE
#define CONFIG_ASYNC_TCP_USE_WDT 0 //if enabled, adds between 33us and 200us per event
#endif
#ifndef CONFIG_ASYNC_TCP_STACK_SIZE
#define CONFIG_ASYNC_TCP_STACK_SIZE 16384  // 8192 * 2
#endif
#ifndef CONFIG_ASYNC_TCP_STACK
#define CONFIG_ASYNC_TCP_STACK CONFIG_ASYNC_TCP_STACK_SIZE
#endif
#ifndef CONFIG_ASYNC_TCP_PRIORITY
#define CONFIG_ASYNC_TCP_PRIORITY 3
#endif
#ifndef CONFIG_ASYNC_TCP_TASK_PRIORITY
#define CONFIG_ASYNC_TCP_TASK_PRIORITY TASK_CONNECTIVITY_PRIO
#endif
#ifndef CONFIG_ASYNC_TCP_TASK_NAME
#define CONFIG_ASYNC_TCP_TASK_NAME "asyncTcpSock"
#endif

class AsyncClient;

#ifndef CONFIG_ASYNC_TCP_MAX_ACK_TIME
#define CONFIG_ASYNC_TCP_MAX_ACK_TIME 5000
#endif
#ifndef ASYNC_MAX_ACK_TIME
#define ASYNC_MAX_ACK_TIME CONFIG_ASYNC_TCP_MAX_ACK_TIME
#endif
#define ASYNC_WRITE_FLAG_COPY 0x01 //will allocate new buffer to hold the data while sending (else will hold reference to the data given)
#define ASYNC_WRITE_FLAG_MORE 0x02 //will not send PSH flag, meaning that there should be more data to be sent before the application should react.
#define SSL_HANDSHAKE_TIMEOUT 5000 // timeout to complete SSL handshake

typedef std::function<void(void*, AsyncClient*)> AcConnectHandler;
typedef std::function<void(void*, AsyncClient*, size_t len, uint32_t time)> AcAckHandler;
typedef std::function<void(void*, AsyncClient*, int8_t error)> AcErrorHandler;
typedef std::function<void(void*, AsyncClient*, void *data, size_t len)> AcDataHandler;
//typedef std::function<void(void*, AsyncClient*, struct pbuf *pb)> AcPacketHandler;
typedef std::function<void(void*, AsyncClient*, uint32_t time)> AcTimeoutHandler;

class AsyncSocketBase
{
private:
    static std::list<AsyncSocketBase *> & _getSocketBaseList(void);

protected:
    int _socket = -1;
    bool _selected = false;
    bool _isdnsfinished = false;
    uint32_t _sock_lastactivity = 0;

    virtual void _sockIsReadable(void) {}     // Action to take on readable socket
    virtual bool _sockIsWriteable(void) { return false; }    // Action to take on writable socket
    virtual void _sockPoll(void) {}           // Action to take on idle socket activity poll
    virtual void _sockDelayedConnect(void) {} // Action to take on DNS-resolve finished

    virtual bool _pendingWrite(void) { return false; }  // Test if there is data pending to be written
    virtual bool _isServer(void) { return false; }      // Will a read from this socket result in one more client?

public:
    AsyncSocketBase(void);
    virtual ~AsyncSocketBase();

    friend void _asynctcpsock_task(void *);
};

class AsyncClient : public AsyncSocketBase
{
  public:
    AsyncClient(int sockfd = -1);
    ~AsyncClient();

#if ASYNC_TCP_SSL_ENABLED
    bool connect(IPAddress ip, uint16_t port, bool secure = false);
    bool connect(const char* host, uint16_t port,  bool secure = false);
    void setRootCa(const char* rootca, const size_t len);
    void setClientCert(const char* cli_cert, const size_t len);
    void setClientKey(const char* cli_key, const size_t len);
    void setPsk(const char* psk_ident, const char* psk);
#else
    bool connect(IPAddress ip, uint16_t port);
    bool connect(const char* host, uint16_t port);
#endif // ASYNC_TCP_SSL_ENABLED
    void close(bool now = false);

    int8_t abort();
    bool free();

    bool canSend() { return space() > 0; }
    size_t space();
    size_t add(const char* data, size_t size, uint8_t apiflags=ASYNC_WRITE_FLAG_COPY);//add for sending
    bool send();

    //write equals add()+send()
    size_t write(const char* data);
    size_t write(const char* data, size_t size, uint8_t apiflags=ASYNC_WRITE_FLAG_COPY); //only when canSend() == true

    uint8_t state() { return _conn_state; }
    bool connected();
    bool freeable();//disconnected or disconnecting

    uint32_t getAckTimeout();
    void setAckTimeout(uint32_t timeout);//no ACK timeout for the last sent packet in milliseconds

    uint32_t getRxTimeout();
    void setRxTimeout(uint32_t timeout);//no RX data timeout for the connection in seconds
    void setNoDelay(bool nodelay);
    bool getNoDelay();

    uint32_t getRemoteAddress();
    uint16_t getRemotePort();
    uint32_t getLocalAddress();
    uint16_t getLocalPort();

    //compatibility
    IPAddress remoteIP();
    uint16_t  remotePort();
    IPAddress localIP();
    uint16_t  localPort();

    void onConnect(AcConnectHandler cb, void* arg = 0);     //on successful connect
    void onDisconnect(AcConnectHandler cb, void* arg = 0);  //disconnected
    void onAck(AcAckHandler cb, void* arg = 0);             //ack received
    void onError(AcErrorHandler cb, void* arg = 0);         //unsuccessful connect or error
    void onData(AcDataHandler cb, void* arg = 0);           //data received
    void onTimeout(AcTimeoutHandler cb, void* arg = 0);     //ack timeout
    void onPoll(AcConnectHandler cb, void* arg = 0);        //every 125ms when connected

    // The following functions are just for API compatibility and do nothing
    size_t ack(size_t len)  { return len; }
    void ackLater() {}

    const char * errorToString(int8_t error);
//    const char * stateToString();

  protected:
    bool _sockIsWriteable(void);
    void _sockIsReadable(void);
    void _sockPoll(void);
    void _sockDelayedConnect(void);
    bool _pendingWrite(void);

  private:

    AcConnectHandler _connect_cb;
    void* _connect_cb_arg;
    AcConnectHandler _discard_cb;
    void* _discard_cb_arg;
    AcAckHandler _sent_cb;
    void* _sent_cb_arg;
    AcErrorHandler _error_cb;
    void* _error_cb_arg;
    AcDataHandler _recv_cb;
    void* _recv_cb_arg;
    AcTimeoutHandler _timeout_cb;
    void* _timeout_cb_arg;
    AcConnectHandler _poll_cb;
    void* _poll_cb_arg;

    uint32_t _rx_last_packet;
    uint32_t _rx_since_timeout;
    uint32_t _ack_timeout;

    // Used on asynchronous DNS resolving scenario - I do not want to connect()
    // from the LWIP thread itself.
    struct ip_addr _connect_addr;
    uint16_t _connect_port = 0;
    //const char * _connect_dnsname = NULL;

#if ASYNC_TCP_SSL_ENABLED
    size_t _root_ca_len;
    char* _root_ca;
    size_t _cli_cert_len;
    char* _cli_cert;
    size_t _cli_key_len;
    char* _cli_key;
    bool _secure;
    bool _handshake_done;
    const char* _psk_ident;
    const char* _psk;

    String _hostname;
    AsyncTCP_TLS_Context * _sslctx;
#endif // ASYNC_TCP_SSL_ENABLED

    // The following private struct represents a buffer enqueued with the add()
    // method. Each of these buffers are flushed whenever the socket becomes
    // writable
    typedef struct {
      uint8_t * data;     // Pointer to data queued for write
      uint32_t  length;   // Length of data queued for write
      uint32_t  written;  // Length of data written to socket so far
      uint32_t  queued_at;// Timestamp at which this data buffer was queued
      uint32_t  written_at; // Timestamp at which this data buffer was completely written
      int       write_errno;  // If != 0, errno value while writing this buffer
      bool      owned;    // If true, we malloc'ed the data and should be freed after completely written.
                          // If false, app owns the memory and should ensure it remains valid until acked
    } queued_writebuf;

    // Internal struct used to implement sent buffer notification
    typedef struct {
      uint32_t length;
      uint32_t delay;
    } notify_writebuf;

    // Queue of buffers to write to socket
    SemaphoreHandle_t _write_mutex;
    std::deque<queued_writebuf> _writeQueue;
    bool _ack_timeout_signaled = false;

    // Remaining space willing to queue for writing
    uint32_t _writeSpaceRemaining;

    // Simulation of connection state
    uint8_t _conn_state;

    void _error(int8_t err);
    void _close(void);
    void _removeAllCallbacks(void);
    bool _flushWriteQueue(void);
    void _clearWriteQueue(void);
    void _collectNotifyWrittenBuffers(std::deque<notify_writebuf> &, int &);
    void _notifyWrittenBuffers(std::deque<notify_writebuf> &, int);

#if ASYNC_TCP_SSL_ENABLED
    int _runSSLHandshakeLoop(void);
#endif

    friend void _tcpsock_dns_found(const char * name, struct ip_addr * ipaddr, void * arg);
};

#if ASYNC_TCP_SSL_ENABLED
typedef std::function<int(void* arg, const char *filename, uint8_t **buf)> AcSSlFileHandler;
#endif

class AsyncServer : public AsyncSocketBase
{
  public:
    AsyncServer(IPAddress addr, uint16_t port);
    AsyncServer(uint16_t port);
    ~AsyncServer();
    void onClient(AcConnectHandler cb, void* arg);
#if ASYNC_TCP_SSL_ENABLED
    // Dummy, so it compiles with ESP Async WebServer library enabled.
    void onSslFileRequest(AcSSlFileHandler cb, void* arg) {};
    void beginSecure(const char *cert, const char *private_key_file, const char *password) {};
#endif
    void begin();
    void end();

    void setNoDelay(bool nodelay) { _noDelay = nodelay; }
    bool getNoDelay() { return _noDelay; }
    uint8_t status();

  protected:
    uint16_t _port;
    IPAddress _addr;

    bool _noDelay;
    AcConnectHandler _connect_cb;
    void* _connect_cb_arg;

    // Listening socket is readable on incoming connection
    void _sockIsReadable(void);

    // Mark this class as a server
    bool _isServer(void) { return true; }
};


#endif /* ASYNCTCP_H_ */
