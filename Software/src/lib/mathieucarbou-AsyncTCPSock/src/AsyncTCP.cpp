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

#include "Arduino.h"

#include "AsyncTCP.h"
#include "esp_task_wdt.h"

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>

#include <lwip/dns.h>

#include <list>

#undef close
#undef connect
#undef write
#undef read

static TaskHandle_t _asyncsock_service_task_handle = NULL;
static SemaphoreHandle_t _asyncsock_mutex = NULL;

typedef std::list<AsyncSocketBase *>::iterator sockIterator;

void _asynctcpsock_task(void *);

#define ASYNCTCPSOCK_POLL_INTERVAL 125

#define MAX_PAYLOAD_SIZE    1360

// Since the only task reading from these sockets is the asyncTcpPSock task
// and all socket clients are serviced sequentially, only one read buffer
// is needed, and it can therefore be statically allocated
static uint8_t _readBuffer[MAX_PAYLOAD_SIZE];

// Start async socket task
static bool _start_asyncsock_task(void)
{
    if (!_asyncsock_service_task_handle) {
        log_i("Creating asyncTcpSock task running in core %d (-1 for any available core)...", CONFIG_ASYNC_TCP_RUNNING_CORE);
        xTaskCreateUniversal(
            _asynctcpsock_task,
            CONFIG_ASYNC_TCP_TASK_NAME,
            CONFIG_ASYNC_TCP_STACK,
            NULL,
            CONFIG_ASYNC_TCP_TASK_PRIORITY,
            &_asyncsock_service_task_handle,
            CONFIG_ASYNC_TCP_RUNNING_CORE);
        if (!_asyncsock_service_task_handle) return false;
    }
    return true;
}

// Actual asynchronous socket task
void _asynctcpsock_task(void *)
{
    auto & _socketBaseList = AsyncSocketBase::_getSocketBaseList();

    while (true) {
        sockIterator it;
        fd_set sockSet_r;
        fd_set sockSet_w;
        int max_sock = 0;

        std::list<AsyncSocketBase *> sockList;

        xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);

        // Collect all of the active sockets into socket set. Half-destroyed
        // connections should have set _socket to -1 and therefore should not
        // end up in the sockList.
        FD_ZERO(&sockSet_r); FD_ZERO(&sockSet_w);
        for (it = _socketBaseList.begin(); it != _socketBaseList.end(); it++) {
            if ((*it)->_socket != -1) {
#ifdef CONFIG_LWIP_MAX_SOCKETS
                if (!(*it)->_isServer() || _socketBaseList.size() < CONFIG_LWIP_MAX_SOCKETS) {
#endif
                    FD_SET((*it)->_socket, &sockSet_r);
                    if (max_sock <= (*it)->_socket) max_sock = (*it)->_socket + 1;
#ifdef CONFIG_LWIP_MAX_SOCKETS
                }
#endif
                if ((*it)->_pendingWrite()) {
                    FD_SET((*it)->_socket, &sockSet_w);
                    if (max_sock <= (*it)->_socket) max_sock = (*it)->_socket + 1;
                }
                (*it)->_selected = true;
            }
        }

        // Wait for activity on all monitored sockets
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = ASYNCTCPSOCK_POLL_INTERVAL * 1000;

        xSemaphoreGiveRecursive(_asyncsock_mutex);

        int r = select(max_sock, &sockSet_r, &sockSet_w, NULL, &tv);

        xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);

        // Check all sockets to see which ones are active
        uint32_t nActive = 0;
        if (r > 0) {
            // Collect and notify all writable sockets. Half-destroyed connections
            // should have set _socket to -1 and therefore should not end up in
            // the sockList.
            for (it = _socketBaseList.begin(); it != _socketBaseList.end(); it++) {
                if ((*it)->_selected && FD_ISSET((*it)->_socket, &sockSet_w)) {
                    sockList.push_back(*it);
                }
            }
            for (it = sockList.begin(); it != sockList.end(); it++) {
#if CONFIG_ASYNC_TCP_USE_WDT
                if (esp_task_wdt_add(NULL) != ESP_OK) {
                    log_e("Failed to add async task to WDT");
                }
#endif
                if ((*it)->_sockIsWriteable()) {
                    (*it)->_sock_lastactivity = millis();
                    nActive++;
                }
#if CONFIG_ASYNC_TCP_USE_WDT
                if (esp_task_wdt_delete(NULL) != ESP_OK) {
                    log_e("Failed to remove loop task from WDT");
                }
#endif
            }
            sockList.clear();

            // Collect and notify all readable sockets. Half-destroyed connections
            // should have set _socket to -1 and therefore should not end up in
            // the sockList.
            for (it = _socketBaseList.begin(); it != _socketBaseList.end(); it++) {
                if ((*it)->_selected && FD_ISSET((*it)->_socket, &sockSet_r)) {
                    sockList.push_back(*it);
                }
            }
            for (it = sockList.begin(); it != sockList.end(); it++) {
#if CONFIG_ASYNC_TCP_USE_WDT
                if (esp_task_wdt_add(NULL) != ESP_OK) {
                    log_e("Failed to add async task to WDT");
                }
#endif
                (*it)->_sock_lastactivity = millis();
                (*it)->_sockIsReadable();
                nActive++;
#if CONFIG_ASYNC_TCP_USE_WDT
                if (esp_task_wdt_delete(NULL) != ESP_OK) {
                    log_e("Failed to remove loop task from WDT");
                }
#endif
            }
            sockList.clear();
        }

        // Collect and notify all sockets waiting for DNS completion
        for (it = _socketBaseList.begin(); it != _socketBaseList.end(); it++) {
            // Collect socket that has finished resolving DNS (with or without error)
            if ((*it)->_isdnsfinished) {
                sockList.push_back(*it);
            }
        }
        for (it = sockList.begin(); it != sockList.end(); it++) {
#if CONFIG_ASYNC_TCP_USE_WDT
            if(esp_task_wdt_add(NULL) != ESP_OK){
                log_e("Failed to add async task to WDT");
            }
#endif
            (*it)->_isdnsfinished = false;
            (*it)->_sockDelayedConnect();
#if CONFIG_ASYNC_TCP_USE_WDT
            if(esp_task_wdt_delete(NULL) != ESP_OK){
                log_e("Failed to remove loop task from WDT");
            }
#endif
        }
        sockList.clear();

        xSemaphoreGiveRecursive(_asyncsock_mutex);

        // Collect and run activity poll on all pollable sockets
        xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
        for (it = _socketBaseList.begin(); it != _socketBaseList.end(); it++) {
            (*it)->_selected = false;
            if (millis() - (*it)->_sock_lastactivity >= ASYNCTCPSOCK_POLL_INTERVAL) {
                (*it)->_sock_lastactivity = millis();
                sockList.push_back(*it);
            }
        }

        // Run activity poll on all pollable sockets
        for (it = sockList.begin(); it != sockList.end(); it++) {
#if CONFIG_ASYNC_TCP_USE_WDT
            if(esp_task_wdt_add(NULL) != ESP_OK){
                log_e("Failed to add async task to WDT");
            }
#endif
            (*it)->_sockPoll();
#if CONFIG_ASYNC_TCP_USE_WDT
            if(esp_task_wdt_delete(NULL) != ESP_OK){
                log_e("Failed to remove loop task from WDT");
            }
#endif
        }
        sockList.clear();

        xSemaphoreGiveRecursive(_asyncsock_mutex);
    }

    vTaskDelete(NULL);
    _asyncsock_service_task_handle = NULL;
}

AsyncSocketBase::AsyncSocketBase()
{
    if (_asyncsock_mutex == NULL) _asyncsock_mutex = xSemaphoreCreateRecursiveMutex();

    _sock_lastactivity = millis();
    _selected = false;

    // Add this base socket to the monitored list
    auto & _socketBaseList = AsyncSocketBase::_getSocketBaseList();
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _socketBaseList.push_back(this);
    xSemaphoreGiveRecursive(_asyncsock_mutex);
}

std::list<AsyncSocketBase *> & AsyncSocketBase::_getSocketBaseList(void)
{
    // List of monitored socket objects
    static std::list<AsyncSocketBase *> _socketBaseList;
    return _socketBaseList;
}

AsyncSocketBase::~AsyncSocketBase()
{
    // Remove this base socket from the monitored list
    auto & _socketBaseList = AsyncSocketBase::_getSocketBaseList();
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _socketBaseList.remove(this);
    xSemaphoreGiveRecursive(_asyncsock_mutex);
}


AsyncClient::AsyncClient(int sockfd)
: _connect_cb(0)
, _connect_cb_arg(0)
, _discard_cb(0)
, _discard_cb_arg(0)
, _sent_cb(0)
, _sent_cb_arg(0)
, _error_cb(0)
, _error_cb_arg(0)
, _recv_cb(0)
, _recv_cb_arg(0)
, _timeout_cb(0)
, _timeout_cb_arg(0)
, _rx_last_packet(0)
, _rx_since_timeout(0)
, _ack_timeout(ASYNC_MAX_ACK_TIME)
, _connect_port(0)
#if ASYNC_TCP_SSL_ENABLED
, _root_ca_len(0)
, _root_ca(NULL)
, _cli_cert_len(0)
, _cli_cert(NULL)
, _cli_key_len(0)
, _cli_key(NULL)
, _secure(false)
, _handshake_done(true)
, _psk_ident(0)
, _psk(0)
, _sslctx(NULL)
#endif // ASYNC_TCP_SSL_ENABLED
, _writeSpaceRemaining(TCP_SND_BUF)
, _conn_state(0)
{
    _write_mutex = xSemaphoreCreateMutex();
    if (sockfd != -1) {
        fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFL, 0 ) | O_NONBLOCK );

        // Updating state visible to asyncTcpSock task
        xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
        _conn_state = 4;
        _socket = sockfd;
        _rx_last_packet = millis();
        xSemaphoreGiveRecursive(_asyncsock_mutex);
    }
}

AsyncClient::~AsyncClient()
{
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    if (_socket != -1) _close();
    _removeAllCallbacks();
    vSemaphoreDelete(_write_mutex);
    _write_mutex = NULL;
    xSemaphoreGiveRecursive(_asyncsock_mutex);
}

void AsyncClient::setRxTimeout(uint32_t timeout){
    _rx_since_timeout = timeout;
}

uint32_t AsyncClient::getRxTimeout(){
    return _rx_since_timeout;
}

uint32_t AsyncClient::getAckTimeout(){
    return _ack_timeout;
}

void AsyncClient::setAckTimeout(uint32_t timeout){
    _ack_timeout = timeout;
}

void AsyncClient::setNoDelay(bool nodelay){
    if (_socket == -1) return;

    int flag = nodelay;
    int res = setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
    if(res < 0) {
        log_e("fail on fd %d, errno: %d, \"%s\"", _socket, errno, strerror(errno));
    }
}

bool AsyncClient::getNoDelay(){
    if (_socket == -1) return false;

    int flag = 0;
    socklen_t size = sizeof(int);
    int res = getsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, &size);
    if(res < 0) {
        log_e("fail on fd %d, errno: %d, \"%s\"", _socket, errno, strerror(errno));
    }
    return flag;
}

/*
 * Callback Setters
 * */

void AsyncClient::onConnect(AcConnectHandler cb, void* arg){
    _connect_cb = cb;
    _connect_cb_arg = arg;
}

void AsyncClient::onDisconnect(AcConnectHandler cb, void* arg){
    _discard_cb = cb;
    _discard_cb_arg = arg;
}

void AsyncClient::onAck(AcAckHandler cb, void* arg){
    _sent_cb = cb;
    _sent_cb_arg = arg;
}

void AsyncClient::onError(AcErrorHandler cb, void* arg){
    _error_cb = cb;
    _error_cb_arg = arg;
}

void AsyncClient::onData(AcDataHandler cb, void* arg){
    _recv_cb = cb;
    _recv_cb_arg = arg;
}

void AsyncClient::onTimeout(AcTimeoutHandler cb, void* arg){
    _timeout_cb = cb;
    _timeout_cb_arg = arg;
}

void AsyncClient::onPoll(AcConnectHandler cb, void* arg){
    _poll_cb = cb;
    _poll_cb_arg = arg;
}

bool AsyncClient::connected(){
    if (_socket == -1) {
        return false;
    }
    return _conn_state == 4;
}

bool AsyncClient::freeable(){
    if (_socket == -1) {
        return true;
    }
    return _conn_state == 0 || _conn_state > 4;
}

uint32_t AsyncClient::getRemoteAddress() {
    if(_socket == -1) {
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    getpeername(_socket, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;

    return s->sin_addr.s_addr;
}

uint16_t AsyncClient::getRemotePort() {
    if(_socket == -1) {
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    getpeername(_socket, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;

    return ntohs(s->sin_port);
}

uint32_t AsyncClient::getLocalAddress() {
    if(_socket == -1) {
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    getsockname(_socket, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;

    return s->sin_addr.s_addr;
}

uint16_t AsyncClient::getLocalPort() {
    if(_socket == -1) {
        return 0;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof addr;
    getsockname(_socket, (struct sockaddr*)&addr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;

    return ntohs(s->sin_port);
}

IPAddress AsyncClient::remoteIP() {
    return IPAddress(getRemoteAddress());
}

uint16_t AsyncClient::remotePort() {
    return getRemotePort();
}

IPAddress AsyncClient::localIP() {
    return IPAddress(getLocalAddress());
}

uint16_t AsyncClient::localPort() {
    return getLocalPort();
}


#if ASYNC_TCP_SSL_ENABLED
bool AsyncClient::connect(IPAddress ip, uint16_t port, bool secure)
#else
bool AsyncClient::connect(IPAddress ip, uint16_t port)
#endif // ASYNC_TCP_SSL_ENABLED
{
    if (_socket != -1) {
        log_w("already connected, state %d", _conn_state);
        return false;
    }

    if(!_start_asyncsock_task()){
        log_e("failed to start task");
        return false;
    }

#if ASYNC_TCP_SSL_ENABLED
    _secure = secure;
    _handshake_done = !secure;
#endif // ASYNC_TCP_SSL_ENABLED

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_e("socket: %d", errno);
        return false;
    }
    int r = fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFL, 0 ) | O_NONBLOCK );

    uint32_t ip_addr = ip;
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    memcpy(&(serveraddr.sin_addr.s_addr), &ip_addr, 4);
    serveraddr.sin_port = htons(port);

#ifdef EINPROGRESS
    #if EINPROGRESS != 119
    #error EINPROGRESS invalid
    #endif
#endif

    //Serial.printf("DEBUG: connect to %08x port %d using IP... ", ip_addr, port);
    errno = 0; r = ::connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    //Serial.printf("r=%d errno=%d\r\n", r, errno);
    if (r < 0 && errno != EINPROGRESS) {
        //Serial.println("\t(connect failed)");
        log_e("connect on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
        ::close(sockfd);
        return false;
    }

    // Updating state visible to asyncTcpSock task
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _conn_state = 2;
    _socket = sockfd;
    _rx_last_packet = millis();
    xSemaphoreGiveRecursive(_asyncsock_mutex);

    // Socket is now connecting. Should become writable in asyncTcpSock task
    //Serial.printf("\twaiting for connect finished on socket: %d\r\n", _socket);
    return true;
}

void _tcpsock_dns_found(const char * name, struct ip_addr * ipaddr, void * arg);
#if ASYNC_TCP_SSL_ENABLED
bool AsyncClient::connect(const char* host, uint16_t port, bool secure){
#else
bool AsyncClient::connect(const char* host, uint16_t port){
#endif // ASYNC_TCP_SSL_ENABLED
    ip_addr_t addr;
    
    if(!_start_asyncsock_task()){
      log_e("failed to start task");
      return false;
    }

    log_v("connect to %s port %d using DNS...", host, port);
    err_t err = dns_gethostbyname(host, &addr, (dns_found_callback)&_tcpsock_dns_found, this);
    if(err == ERR_OK) {
        log_v("\taddr resolved as %08x, connecting...", addr.u_addr.ip4.addr);
#if ASYNC_TCP_SSL_ENABLED
        _hostname = host;
        return connect(IPAddress(addr.u_addr.ip4.addr), port, secure);
#else
        return connect(IPAddress(addr.u_addr.ip4.addr), port);
#endif // ASYNC_TCP_SSL_ENABLED
    } else if(err == ERR_INPROGRESS) {
        log_v("\twaiting for DNS resolution");
        _conn_state = 1;
        _connect_port = port;
#if ASYNC_TCP_SSL_ENABLED
        _hostname = host;
        _secure = secure;
        _handshake_done = !secure;
#endif // ASYNC_TCP_SSL_ENABLED
        return true;
    }
    log_e("error: %d", err);
    return false;
}

// This function runs in the LWIP thread
void _tcpsock_dns_found(const char * name, struct ip_addr * ipaddr, void * arg)
{
    AsyncClient * c = (AsyncClient *)arg;
    if (ipaddr) {
        memcpy(&(c->_connect_addr), ipaddr, sizeof(struct ip_addr));
    } else {
        memset(&(c->_connect_addr), 0, sizeof(struct ip_addr));
    }

    // Updating state visible to asyncTcpSock task
    // MUST NOT take _asyncsock_mutex lock, risks a deadlock if task holding lock
    // attempts a LWIP network call.
    c->_isdnsfinished = true;

    // TODO: actually use name
}

// DNS resolving has finished. Check for error or connect
void AsyncClient::_sockDelayedConnect(void)
{
    if (_connect_addr.u_addr.ip4.addr) {
#if ASYNC_TCP_SSL_ENABLED
        connect(IPAddress(_connect_addr.u_addr.ip4.addr), _connect_port, _secure);
#else
        connect(IPAddress(_connect_addr.u_addr.ip4.addr), _connect_port);
#endif
    } else {
        _conn_state = 0;
        if(_error_cb) {
            _error_cb(_error_cb_arg, this, -55);
        }
        if(_discard_cb) {
            _discard_cb(_discard_cb_arg, this);
        }
    }
}

#if ASYNC_TCP_SSL_ENABLED
int AsyncClient::_runSSLHandshakeLoop(void)
{
    int res = 0;

    while (!_handshake_done) {
        res = _sslctx->runSSLHandshake();
        if (res == 0) {
            // Handshake successful
            _handshake_done = true;
        } else if (ASYNCTCP_TLS_CAN_RETRY(res)) {
            // Ran out of readable data or writable space on socket, must continue later
            break;
        } else {
            // SSL handshake for AsyncTCP does not inform SSL errors
            log_e("TLS setup failed with error %d, closing socket...", res);
            _close();
            // _sslctx should be NULL after this
            break;
        }
    }

    return res;
}
#endif

bool AsyncClient::_sockIsWriteable(void)
{
    int res;
    int sockerr;
    socklen_t len;
    bool activity = false;

    int sent_errno = 0;
    std::deque<notify_writebuf> notifylist;

    // Socket is now writeable. What should we do?
    switch (_conn_state) {
    case 2:
    case 3:
        // Socket has finished connecting. What happened?
        len = (socklen_t)sizeof(int);
        res = getsockopt(_socket, SOL_SOCKET, SO_ERROR, &sockerr, &len);
        if (res < 0) {
            _error(errno);
        } else if (sockerr != 0) {
            _error(sockerr);
        } else {
#if ASYNC_TCP_SSL_ENABLED
            if (_secure) {
                int res = 0;

                if (_sslctx == NULL) {
                    String remIP_str = remoteIP().toString();
                    const char * host_or_ip = _hostname.isEmpty()
                        ? remIP_str.c_str()
                        : _hostname.c_str();

                    _sslctx = new AsyncTCP_TLS_Context();
                    if (_root_ca != NULL) {
                        res = _sslctx->startSSLClient(_socket, host_or_ip,
                            (const unsigned char *)_root_ca, _root_ca_len,
                            (const unsigned char *)_cli_cert, _cli_cert_len,
                            (const unsigned char *)_cli_key, _cli_key_len);
                    } else if (_psk_ident != NULL) {
                        res = _sslctx->startSSLClient(_socket, host_or_ip,
                            _psk_ident, _psk);
                    } else {
                        res = _sslctx->startSSLClientInsecure(_socket, host_or_ip);
                    }

                    if (res != 0) {
                        // SSL setup for AsyncTCP does not inform SSL errors
                        log_e("TLS setup failed with error %d, closing socket...", res);
                        _close();
                        // _sslctx should be NULL after this
                    }
                }

                // _handshake_done is set to FALSE on connect() if encrypted connection
                if (_sslctx != NULL && res == 0) res = _runSSLHandshakeLoop();

                if (!_handshake_done) return ASYNCTCP_TLS_CAN_RETRY(res);

                // Fallthrough to ordinary successful connection
            }
#endif

            // Socket is now fully connected
            _conn_state = 4;
            activity = true;
            _rx_last_packet = millis();
            _ack_timeout_signaled = false;

            if(_connect_cb) {
                _connect_cb(_connect_cb_arg, this);
            }
        }
        break;
    case 4:
    default:
        // Socket can accept some new data...
        xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
        if (_writeQueue.size() > 0) {
            activity = _flushWriteQueue();
            _collectNotifyWrittenBuffers(notifylist, sent_errno);
        }
        xSemaphoreGive(_write_mutex);

        _notifyWrittenBuffers(notifylist, sent_errno);

        break;
    }

    return activity;
}

bool AsyncClient::_flushWriteQueue(void)
{
    bool activity = false;

    if (_socket == -1) return false;

    for (auto it = _writeQueue.begin(); it != _writeQueue.end(); it++) {
        // Abort iteration if error found while writing a buffer
        if (it->write_errno != 0) break;

        // Skip over head buffers already fully written
        if (it->written >= it->length) continue;

        bool keep_writing = true;
        do {
            uint8_t * p = it->data + it->written;
            size_t n = it->length - it->written;
            errno = 0;
            ssize_t r;

#if ASYNC_TCP_SSL_ENABLED
            if (_sslctx != NULL) {
                r = _sslctx->write(p, n);
                if (ASYNCTCP_TLS_CAN_RETRY(r)) {
                    r = -1;
                    errno = EAGAIN;
                } else if (ASYNCTCP_TLS_EOF(r)) {
                    r = -1;
                    errno = EPIPE;
                } else if (r < 0) {
                    if (errno == 0) errno = EIO;
                }
            } else {
#endif
                r = lwip_write(_socket, p, n);
#if ASYNC_TCP_SSL_ENABLED
            }
#endif

            if (r >= 0) {
                // Written some data into the socket
                it->written += r;
                _writeSpaceRemaining += r;
                activity = true;

                if (it->written >= it->length) {
                    it->written_at = millis();
                    if (it->owned) ::free(it->data);
                    it->data = NULL;
                }
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket is full, could not write anything
                keep_writing = false;
            } else {
                // A write error happened that should be reported
                it->write_errno = errno;
                keep_writing = false;
                log_e("socket %d lwip_write() failed errno=%d", _socket, it->write_errno);
            }
        } while (keep_writing && it->written < it->length);
    }

    return activity;
}

// This method MUST be called with _write_mutex held
void AsyncClient::_collectNotifyWrittenBuffers(std::deque<notify_writebuf> & notifyqueue, int & write_errno)
{
    write_errno = 0;
    notifyqueue.clear();

    while (_writeQueue.size() > 0) {
        if (_writeQueue.front().write_errno != 0) {
            write_errno = _writeQueue.front().write_errno;
            return;
        }

        if (_writeQueue.front().written >= _writeQueue.front().length) {
            // Collect information on fully-written buffer, and stash it into notify queue
            if (_writeQueue.front().written_at > _rx_last_packet) {
                _rx_last_packet = _writeQueue.front().written_at;
            }
            if (_writeQueue.front().owned && _writeQueue.front().data != NULL) ::free(_writeQueue.front().data);

            notify_writebuf noti;
            noti.length = _writeQueue.front().length;
            noti.delay = _writeQueue.front().written_at - _writeQueue.front().queued_at;
            _writeQueue.pop_front();
            notifyqueue.push_back(noti);
        } else {
            // Found first not-fully-written buffer, stop here
            return;
        }
    }
}

void AsyncClient::_notifyWrittenBuffers(std::deque<notify_writebuf> & notifyqueue, int write_errno)
{
    while (notifyqueue.size() > 0) {
        if (notifyqueue.front().length > 0 && _sent_cb) {
            _sent_cb(_sent_cb_arg, this, notifyqueue.front().length, notifyqueue.front().delay);
        }
        notifyqueue.pop_front();
    }

    if (write_errno != 0) _error(write_errno);
}

void AsyncClient::_sockIsReadable(void)
{
    _rx_last_packet = millis();
    errno = 0;
    ssize_t r;

#if ASYNC_TCP_SSL_ENABLED
    if (_sslctx != NULL) {
        if (!_handshake_done) {
            // Handshake process has stopped for want of data, must be
            // continued here for connection to complete.
            _runSSLHandshakeLoop();

            // If handshake was successful, this will be recognized when the socket
            // next becomes writable. No other read operation should be done here.
            return;
        } else {
            r = _sslctx->read(_readBuffer, MAX_PAYLOAD_SIZE);
            if (ASYNCTCP_TLS_CAN_RETRY(r)) {
                r = -1;
                errno = EAGAIN;
            } else if (ASYNCTCP_TLS_EOF(r)) {
                // Simulate "successful" end-of-stream condition
                r = 0;
            } else if (r < 0) {
                if (errno == 0) errno = EIO;
            }
        }
    } else {
#endif
        r = lwip_read(_socket, _readBuffer, MAX_PAYLOAD_SIZE);
#if ASYNC_TCP_SSL_ENABLED
    }
#endif

    if (r > 0) {
        if(_recv_cb) {
            _recv_cb(_recv_cb_arg, this, _readBuffer, r);
        }
    } else if (r == 0) {
        // A successful read of 0 bytes indicates remote side closed connection
        _close();
    } else if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Do nothing, will try later
        } else {
            _error(errno);
        }
    }
}

void AsyncClient::_sockPoll(void)
{
    if (!connected()) return;

    // The AsyncClient::send() call may be invoked from tasks other than "asyncTcpSock"
    // and may have written buffers via _flushWriteQueue(), but the ack callbacks have
    // not been called yet, nor buffers removed from the write queue. For consistency,
    // written buffers are now acked here.
    std::deque<notify_writebuf> notifylist;
    int sent_errno = 0;
    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    if (_writeQueue.size() > 0) {
        _collectNotifyWrittenBuffers(notifylist, sent_errno);
    }
    xSemaphoreGive(_write_mutex);

    _notifyWrittenBuffers(notifylist, sent_errno);

    /* Connection migh be closed after ACK notification. */
    if (!connected()) return;

    uint32_t now = millis();

    // ACK Timeout - simulated by write queue staleness
    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    if (_writeQueue.size() > 0 && !_ack_timeout_signaled && _ack_timeout) {
        uint32_t sent_delay = now - _writeQueue.front().queued_at;
        if (sent_delay >= _ack_timeout && _writeQueue.front().written_at == 0) {
            _ack_timeout_signaled = true;
            //log_w("ack timeout %d", pcb->state);
            xSemaphoreGive(_write_mutex);
            if(_timeout_cb)
                _timeout_cb(_timeout_cb_arg, this, sent_delay);
            return;
        }
    }
    xSemaphoreGive(_write_mutex);

    // RX Timeout? Check for readable socket before bailing out
    if (_rx_since_timeout && (now - _rx_last_packet) >= (_rx_since_timeout * 1000)) {
        fd_set sockSet_r;
        struct timeval tv;

        FD_ZERO(&sockSet_r);
        FD_SET(_socket, &sockSet_r);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int r = select(_socket + 1, &sockSet_r, NULL, NULL, &tv);
        if (r > 0) _rx_last_packet = now;
    }

    // RX Timeout
    if (_rx_since_timeout && (now - _rx_last_packet) >= (_rx_since_timeout * 1000)) {
        //log_w("rx timeout %d", pcb->state);
        _close();
        return;
    }
    // Everything is fine
    if(_poll_cb) {
        _poll_cb(_poll_cb_arg, this);
    }
}

void AsyncClient::_removeAllCallbacks(void)
{
    _connect_cb = NULL;
    _connect_cb_arg = NULL;
    _discard_cb = NULL;
    _discard_cb_arg = NULL;
    _sent_cb = NULL;
    _sent_cb_arg = NULL;
    _error_cb = NULL;
    _error_cb_arg = NULL;
    _recv_cb = NULL;
    _recv_cb_arg = NULL;
    _timeout_cb = NULL;
    _timeout_cb_arg = NULL;
    _poll_cb = NULL;
    _poll_cb_arg = NULL;
}

void AsyncClient::_close(void)
{
    //Serial.print("AsyncClient::_close: "); Serial.println(_socket);
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _conn_state = 0;
    ::close(_socket);
    _socket = -1;
#if ASYNC_TCP_SSL_ENABLED
    if (_sslctx != NULL) {
        delete _sslctx;
        _sslctx = NULL;
    }
#endif
    xSemaphoreGiveRecursive(_asyncsock_mutex);

    _clearWriteQueue();
    if (_discard_cb) _discard_cb(_discard_cb_arg, this);
}

void AsyncClient::_error(int8_t err)
{
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _conn_state = 0;
    ::close(_socket);
    _socket = -1;
#if ASYNC_TCP_SSL_ENABLED
    if (_sslctx != NULL) {
        delete _sslctx;
        _sslctx = NULL;
    }
#endif
    xSemaphoreGiveRecursive(_asyncsock_mutex);

    _clearWriteQueue();
    if (_error_cb) _error_cb(_error_cb_arg, this, err);
    if (_discard_cb) _discard_cb(_discard_cb_arg, this);
}

size_t AsyncClient::space()
{
    if (!connected()) return 0;
    return _writeSpaceRemaining;
}

size_t AsyncClient::add(const char* data, size_t size, uint8_t apiflags)
{
    queued_writebuf n_entry;

    if (!connected() || data == NULL || size <= 0) return 0;

    size_t room = space();
    if (!room) return 0;

    size_t will_send = (room < size) ? room : size;
    if (apiflags & ASYNC_WRITE_FLAG_COPY) {
        n_entry.data = (uint8_t *)malloc(will_send);
        if (n_entry.data == NULL) {
            return 0;
        }
        memcpy(n_entry.data, data, will_send);
        n_entry.owned = true;
    } else {
        n_entry.data = (uint8_t *)data;
        n_entry.owned = false;
    }
    n_entry.length = will_send;
    n_entry.written = 0;
    n_entry.queued_at = millis();
    n_entry.written_at = 0;
    n_entry.write_errno = 0;

    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    _writeQueue.push_back(n_entry);
    _writeSpaceRemaining -= will_send;
    _ack_timeout_signaled = false;
    xSemaphoreGive(_write_mutex);

    return will_send;
}

bool AsyncClient::send()
{
    if (!connected()) return false;

    fd_set sockSet_w;
    struct timeval tv;

    FD_ZERO(&sockSet_w);
    FD_SET(_socket, &sockSet_w);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // Write as much data as possible from queue if socket is writable
    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    int r = select(_socket + 1, NULL, &sockSet_w, NULL, &tv);
    if (r > 0) _flushWriteQueue();
    xSemaphoreGive(_write_mutex);
    return true;
}

bool AsyncClient::_pendingWrite(void)
{
    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    bool pending = ((_conn_state > 0 && _conn_state < 4) || _writeQueue.size() > 0);
    xSemaphoreGive(_write_mutex);
    return pending;
}

// In normal operation this should be a no-op. Will only free something in case
// of errors before all data was written.
void AsyncClient::_clearWriteQueue(void)
{
    xSemaphoreTake(_write_mutex, (TickType_t)portMAX_DELAY);
    while (_writeQueue.size() > 0) {
        if (_writeQueue.front().owned) {
            if (_writeQueue.front().data != NULL) ::free(_writeQueue.front().data);
        }
        _writeQueue.pop_front();
    }
    xSemaphoreGive(_write_mutex);
}

bool AsyncClient::free(){
    if (_socket == -1) return true;
    return (_conn_state == 0 || _conn_state > 4);
}

size_t AsyncClient::write(const char* data) {
    if(data == NULL) {
        return 0;
    }
    return write(data, strlen(data));
}

size_t AsyncClient::write(const char* data, size_t size, uint8_t apiflags) {
    size_t will_send = add(data, size, apiflags);
    if(!will_send || !send()) {
        return 0;
    }
    return will_send;
}

void AsyncClient::close(bool now)
{
    if (_socket != -1) _close();
}

int8_t AsyncClient::abort(){
    if (_socket != -1) {
        // Note: needs LWIP_SO_LINGER to be enabled in order to work, otherwise
        // this call is equivalent to close().
        struct linger l;
        l.l_onoff = 1;
        l.l_linger = 0;
        setsockopt(_socket, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
        _close();
    }
    return ERR_ABRT;
}

#if ASYNC_TCP_SSL_ENABLED
void AsyncClient::setRootCa(const char* rootca, const size_t len) {
    _root_ca = (char*)rootca;
    _root_ca_len = len;
}

void AsyncClient::setClientCert(const char* cli_cert, const size_t len) {
    _cli_cert = (char*)cli_cert;
    _cli_cert_len = len;
}

void AsyncClient::setClientKey(const char* cli_key, const size_t len) {
    _cli_key = (char*)cli_key;
    _cli_key_len = len;
}

void AsyncClient::setPsk(const char* psk_ident, const char* psk) {
    _psk_ident = psk_ident;
    _psk = psk;
}
#endif // ASYNC_TCP_SSL_ENABLED

const char * AsyncClient::errorToString(int8_t error){
    switch(error){
        case ERR_OK: return "OK";
        case ERR_MEM: return "Out of memory error";
        case ERR_BUF: return "Buffer error";
        case ERR_TIMEOUT: return "Timeout";
        case ERR_RTE: return "Routing problem";
        case ERR_INPROGRESS: return "Operation in progress";
        case ERR_VAL: return "Illegal value";
        case ERR_WOULDBLOCK: return "Operation would block";
        case ERR_USE: return "Address in use";
        case ERR_ALREADY: return "Already connected";
        case ERR_CONN: return "Not connected";
        case ERR_IF: return "Low-level netif error";
        case ERR_ABRT: return "Connection aborted";
        case ERR_RST: return "Connection reset";
        case ERR_CLSD: return "Connection closed";
        case ERR_ARG: return "Illegal argument";
        case -55: return "DNS failed";
        default: return "UNKNOWN";
    }
}
/*
const char * AsyncClient::stateToString(){
    switch(state()){
        case 0: return "Closed";
        case 1: return "Listen";
        case 2: return "SYN Sent";
        case 3: return "SYN Received";
        case 4: return "Established";
        case 5: return "FIN Wait 1";
        case 6: return "FIN Wait 2";
        case 7: return "Close Wait";
        case 8: return "Closing";
        case 9: return "Last ACK";
        case 10: return "Time Wait";
        default: return "UNKNOWN";
    }
}
*/



/*
  Async TCP Server
 */

AsyncServer::AsyncServer(IPAddress addr, uint16_t port)
: _port(port)
, _addr(addr)
, _noDelay(false)
, _connect_cb(0)
, _connect_cb_arg(0)
{}

AsyncServer::AsyncServer(uint16_t port)
: _port(port)
, _addr((uint32_t) IPADDR_ANY)
, _noDelay(false)
, _connect_cb(0)
, _connect_cb_arg(0)
{}

AsyncServer::~AsyncServer(){
    end();
}

void AsyncServer::onClient(AcConnectHandler cb, void* arg){
    _connect_cb = cb;
    _connect_cb_arg = arg;
}

void AsyncServer::begin()
{
    if (_socket != -1) return;

    if (!_start_asyncsock_task()) {
        log_e("failed to start task");
        return;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = (uint32_t) _addr;
    server.sin_port = htons(_port);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        ::close(sockfd);
        log_e("bind error: %d - %s", errno, strerror(errno));
        return;
    }

    static uint8_t backlog = 5;
    if (listen(sockfd , backlog) < 0) {
        ::close(sockfd);
        log_e("listen error: %d - %s", errno, strerror(errno));
        return;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Updating state visible to asyncTcpSock task
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    _socket = sockfd;
    xSemaphoreGiveRecursive(_asyncsock_mutex);
}

void AsyncServer::end()
{
    if (_socket == -1) return;
    xSemaphoreTakeRecursive(_asyncsock_mutex, (TickType_t)portMAX_DELAY);
    ::close(_socket);
    _socket = -1;
    xSemaphoreGiveRecursive(_asyncsock_mutex);
}

void AsyncServer::_sockIsReadable(void)
{
    //Serial.print("AsyncServer::_sockIsReadable: "); Serial.println(_socket);

    if (_connect_cb) {
        struct sockaddr_in client;
        size_t cs = sizeof(struct sockaddr_in);
        errno = 0; int accepted_sockfd = ::accept(_socket, (struct sockaddr *)&client, (socklen_t*)&cs);
        //Serial.printf("\t new sockfd=%d errno=%d\r\n", accepted_sockfd, errno);
        if (accepted_sockfd < 0) {
            log_e("accept error: %d - %s", errno, strerror(errno));
            return;
        }

        AsyncClient * c = new AsyncClient(accepted_sockfd);
        if (c) {
            c->setNoDelay(_noDelay);
            _connect_cb(_connect_cb_arg, c);
        }
    }
}

