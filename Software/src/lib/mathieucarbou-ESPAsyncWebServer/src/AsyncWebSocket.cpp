/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

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
#include "AsyncWebSocket.h"
#include "Arduino.h"

#include <cstring>

#include <libb64/cencode.h>

#if defined(ESP32)
  #if ESP_IDF_VERSION_MAJOR < 5
    #include "BackPort_SHA1Builder.h"
  #else
    #include <SHA1Builder.h>
  #endif
  #include <rom/ets_sys.h>
#elif defined(TARGET_RP2040) || defined(ESP8266)
  #include <Hash.h>
#endif

using namespace asyncsrv;

size_t webSocketSendFrameWindow(AsyncClient* client) {
  if (!client || !client->canSend())
    return 0;
  size_t space = client->space();
  if (space < 9)
    return 0;
  return space - 8;
}

size_t webSocketSendFrame(AsyncClient* client, bool final, uint8_t opcode, bool mask, uint8_t* data, size_t len) {
  if (!client || !client->canSend()) {
    // Serial.println("SF 1");
    return 0;
  }
  size_t space = client->space();
  if (space < 2) {
    // Serial.println("SF 2");
    return 0;
  }
  uint8_t mbuf[4] = {0, 0, 0, 0};
  uint8_t headLen = 2;
  if (len && mask) {
    headLen += 4;
    mbuf[0] = rand() % 0xFF;
    mbuf[1] = rand() % 0xFF;
    mbuf[2] = rand() % 0xFF;
    mbuf[3] = rand() % 0xFF;
  }
  if (len > 125)
    headLen += 2;
  if (space < headLen) {
    // Serial.println("SF 2");
    return 0;
  }
  space -= headLen;

  if (len > space)
    len = space;

  uint8_t* buf = (uint8_t*)malloc(headLen);
  if (buf == NULL) {
    // os_printf("could not malloc %u bytes for frame header\n", headLen);
    //  Serial.println("SF 3");
    return 0;
  }

  buf[0] = opcode & 0x0F;
  if (final)
    buf[0] |= 0x80;
  if (len < 126)
    buf[1] = len & 0x7F;
  else {
    buf[1] = 126;
    buf[2] = (uint8_t)((len >> 8) & 0xFF);
    buf[3] = (uint8_t)(len & 0xFF);
  }
  if (len && mask) {
    buf[1] |= 0x80;
    memcpy(buf + (headLen - 4), mbuf, 4);
  }
  if (client->add((const char*)buf, headLen) != headLen) {
    // os_printf("error adding %lu header bytes\n", headLen);
    free(buf);
    // Serial.println("SF 4");
    return 0;
  }
  free(buf);

  if (len) {
    if (len && mask) {
      size_t i;
      for (i = 0; i < len; i++)
        data[i] = data[i] ^ mbuf[i % 4];
    }
    if (client->add((const char*)data, len) != len) {
      // os_printf("error adding %lu data bytes\n", len);
      //  Serial.println("SF 5");
      return 0;
    }
  }
  if (!client->send()) {
    // os_printf("error sending frame: %lu\n", headLen+len);
    //  Serial.println("SF 6");
    return 0;
  }
  // Serial.println("SF");
  return len;
}

/*
 *    AsyncWebSocketMessageBuffer
 */

AsyncWebSocketMessageBuffer::AsyncWebSocketMessageBuffer(const uint8_t* data, size_t size)
    : _buffer(std::make_shared<std::vector<uint8_t>>(size)) {
  if (_buffer->capacity() < size) {
    _buffer->reserve(size);
  } else {
    std::memcpy(_buffer->data(), data, size);
  }
}

AsyncWebSocketMessageBuffer::AsyncWebSocketMessageBuffer(size_t size)
    : _buffer(std::make_shared<std::vector<uint8_t>>(size)) {
  if (_buffer->capacity() < size) {
    _buffer->reserve(size);
  }
}

bool AsyncWebSocketMessageBuffer::reserve(size_t size) {
  if (_buffer->capacity() >= size)
    return true;
  _buffer->reserve(size);
  return _buffer->capacity() >= size;
}

/*
 * Control Frame
 */

class AsyncWebSocketControl {
  private:
    uint8_t _opcode;
    uint8_t* _data;
    size_t _len;
    bool _mask;
    bool _finished;

  public:
    AsyncWebSocketControl(uint8_t opcode, const uint8_t* data = NULL, size_t len = 0, bool mask = false)
        : _opcode(opcode), _len(len), _mask(len && mask), _finished(false) {
      if (data == NULL)
        _len = 0;
      if (_len) {
        if (_len > 125)
          _len = 125;

        _data = (uint8_t*)malloc(_len);

        if (_data == NULL)
          _len = 0;
        else
          memcpy(_data, data, len);
      } else
        _data = NULL;
    }

    ~AsyncWebSocketControl() {
      if (_data != NULL)
        free(_data);
    }

    bool finished() const { return _finished; }
    uint8_t opcode() { return _opcode; }
    uint8_t len() { return _len + 2; }
    size_t send(AsyncClient* client) {
      _finished = true;
      return webSocketSendFrame(client, true, _opcode & 0x0F, _mask, _data, _len);
    }
};

/*
 * AsyncWebSocketMessage Message
 */

AsyncWebSocketMessage::AsyncWebSocketMessage(AsyncWebSocketSharedBuffer buffer, uint8_t opcode, bool mask) : _WSbuffer{buffer},
                                                                                                             _opcode(opcode & 0x07),
                                                                                                             _mask{mask},
                                                                                                             _status{_WSbuffer ? WS_MSG_SENDING : WS_MSG_ERROR} {
}

void AsyncWebSocketMessage::ack(size_t len, uint32_t time) {
  (void)time;
  _acked += len;
  if (_sent >= _WSbuffer->size() && _acked >= _ack) {
    _status = WS_MSG_SENT;
  }
  // ets_printf("A: %u\n", len);
}

size_t AsyncWebSocketMessage::send(AsyncClient* client) {
  if (!client)
    return 0;

  if (_status != WS_MSG_SENDING)
    return 0;
  if (_acked < _ack) {
    return 0;
  }
  if (_sent == _WSbuffer->size()) {
    if (_acked == _ack)
      _status = WS_MSG_SENT;
    return 0;
  }
  if (_sent > _WSbuffer->size()) {
    _status = WS_MSG_ERROR;
    // ets_printf("E: %u > %u\n", _sent, _WSbuffer->length());
    return 0;
  }

  size_t toSend = _WSbuffer->size() - _sent;
  size_t window = webSocketSendFrameWindow(client);

  if (window < toSend) {
    toSend = window;
  }

  _sent += toSend;
  _ack += toSend + ((toSend < 126) ? 2 : 4) + (_mask * 4);

  // ets_printf("W: %u %u\n", _sent - toSend, toSend);

  bool final = (_sent == _WSbuffer->size());
  uint8_t* dPtr = (uint8_t*)(_WSbuffer->data() + (_sent - toSend));
  uint8_t opCode = (toSend && _sent == toSend) ? _opcode : (uint8_t)WS_CONTINUATION;

  size_t sent = webSocketSendFrame(client, final, opCode, _mask, dPtr, toSend);
  _status = WS_MSG_SENDING;
  if (toSend && sent != toSend) {
    // ets_printf("E: %u != %u\n", toSend, sent);
    _sent -= (toSend - sent);
    _ack -= (toSend - sent);
  }
  // ets_printf("S: %u %u\n", _sent, sent);
  return sent;
}

/*
 * Async WebSocket Client
 */
const char* AWSC_PING_PAYLOAD = "ESPAsyncWebServer-PING";
const size_t AWSC_PING_PAYLOAD_LEN = 22;

AsyncWebSocketClient::AsyncWebSocketClient(AsyncWebServerRequest* request, AsyncWebSocket* server)
    : _tempObject(NULL) {
  _client = request->client();
  _server = server;
  _clientId = _server->_getNextId();
  _status = WS_CONNECTED;
  _pstate = 0;
  _lastMessageTime = millis();
  _keepAlivePeriod = 0;
  _client->setRxTimeout(0);
  _client->onError([](void* r, AsyncClient* c, int8_t error) { (void)c; ((AsyncWebSocketClient*)(r))->_onError(error); }, this);
  _client->onAck([](void* r, AsyncClient* c, size_t len, uint32_t time) { (void)c; ((AsyncWebSocketClient*)(r))->_onAck(len, time); }, this);
  _client->onDisconnect([](void* r, AsyncClient* c) { ((AsyncWebSocketClient*)(r))->_onDisconnect(); delete c; }, this);
  _client->onTimeout([](void* r, AsyncClient* c, uint32_t time) { (void)c; ((AsyncWebSocketClient*)(r))->_onTimeout(time); }, this);
  _client->onData([](void* r, AsyncClient* c, void* buf, size_t len) { (void)c; ((AsyncWebSocketClient*)(r))->_onData(buf, len); }, this);
  _client->onPoll([](void* r, AsyncClient* c) { (void)c; ((AsyncWebSocketClient*)(r))->_onPoll(); }, this);
  delete request;
  memset(&_pinfo, 0, sizeof(_pinfo));
}

AsyncWebSocketClient::~AsyncWebSocketClient() {
  {
    std::lock_guard<std::mutex> lock(_lock);
    _messageQueue.clear();
    _controlQueue.clear();
  }
  _server->_handleEvent(this, WS_EVT_DISCONNECT, NULL, NULL, 0);
}

void AsyncWebSocketClient::_clearQueue() {
  while (!_messageQueue.empty() && _messageQueue.front().finished())
    _messageQueue.pop_front();
}

void AsyncWebSocketClient::_onAck(size_t len, uint32_t time) {
  _lastMessageTime = millis();

  std::lock_guard<std::mutex> lock(_lock);

  if (!_controlQueue.empty()) {
    auto& head = _controlQueue.front();
    if (head.finished()) {
      len -= head.len();
      if (_status == WS_DISCONNECTING && head.opcode() == WS_DISCONNECT) {
        _controlQueue.pop_front();
        _status = WS_DISCONNECTED;
        if (_client)
          _client->close(true);
        return;
      }
      _controlQueue.pop_front();
    }
  }

  if (len && !_messageQueue.empty()) {
    _messageQueue.front().ack(len, time);
  }

  _clearQueue();

  _runQueue();
}

void AsyncWebSocketClient::_onPoll() {
  if (!_client)
    return;

  std::unique_lock<std::mutex> lock(_lock);
  if (_client && _client->canSend() && (!_controlQueue.empty() || !_messageQueue.empty())) {
    _runQueue();
  } else if (_keepAlivePeriod > 0 && (millis() - _lastMessageTime) >= _keepAlivePeriod && (_controlQueue.empty() && _messageQueue.empty())) {
    lock.unlock();
    ping((uint8_t*)AWSC_PING_PAYLOAD, AWSC_PING_PAYLOAD_LEN);
  }
}

void AsyncWebSocketClient::_runQueue() {
  // all calls to this method MUST be protected by a mutex lock!
  if (!_client)
    return;

  _clearQueue();

  if (!_controlQueue.empty() && (_messageQueue.empty() || _messageQueue.front().betweenFrames()) && webSocketSendFrameWindow(_client) > (size_t)(_controlQueue.front().len() - 1)) {
    _controlQueue.front().send(_client);
  } else if (!_messageQueue.empty() && _messageQueue.front().betweenFrames() && webSocketSendFrameWindow(_client)) {
    _messageQueue.front().send(_client);
  }
}

bool AsyncWebSocketClient::queueIsFull() const {
  std::lock_guard<std::mutex> lock(_lock);
  return (_messageQueue.size() >= WS_MAX_QUEUED_MESSAGES) || (_status != WS_CONNECTED);
}

size_t AsyncWebSocketClient::queueLen() const {
  std::lock_guard<std::mutex> lock(_lock);
  return _messageQueue.size();
}

bool AsyncWebSocketClient::canSend() const {
  std::lock_guard<std::mutex> lock(_lock);
  return _messageQueue.size() < WS_MAX_QUEUED_MESSAGES;
}

bool AsyncWebSocketClient::_queueControl(uint8_t opcode, const uint8_t* data, size_t len, bool mask) {
  if (!_client)
    return false;

  std::lock_guard<std::mutex> lock(_lock);

  _controlQueue.emplace_back(opcode, data, len, mask);

  if (_client && _client->canSend())
    _runQueue();

  return true;
}

bool AsyncWebSocketClient::_queueMessage(AsyncWebSocketSharedBuffer buffer, uint8_t opcode, bool mask) {
  if (!_client || buffer->size() == 0 || _status != WS_CONNECTED)
    return false;

  std::lock_guard<std::mutex> lock(_lock);

  if (_messageQueue.size() >= WS_MAX_QUEUED_MESSAGES) {
    if (closeWhenFull) {
      _status = WS_DISCONNECTED;

      if (_client)
        _client->close(true);
      log_e("Too many messages queued: closing connection");

    } else {
      log_e("Too many messages queued: discarding new message");
    }

    return false;
  }

  _messageQueue.emplace_back(buffer, opcode, mask);

  if (_client && _client->canSend())
    _runQueue();

  return true;
}

void AsyncWebSocketClient::close(uint16_t code, const char* message) {
  if (_status != WS_CONNECTED)
    return;

  _status = WS_DISCONNECTING;

  if (code) {
    uint8_t packetLen = 2;
    if (message != NULL) {
      size_t mlen = strlen(message);
      if (mlen > 123)
        mlen = 123;
      packetLen += mlen;
    }
    char* buf = (char*)malloc(packetLen);
    if (buf != NULL) {
      buf[0] = (uint8_t)(code >> 8);
      buf[1] = (uint8_t)(code & 0xFF);
      if (message != NULL) {
        memcpy(buf + 2, message, packetLen - 2);
      }
      _queueControl(WS_DISCONNECT, (uint8_t*)buf, packetLen);
      free(buf);
      return;
    }
  }
  _queueControl(WS_DISCONNECT);
}

bool AsyncWebSocketClient::ping(const uint8_t* data, size_t len) {
  return _status == WS_CONNECTED && _queueControl(WS_PING, data, len);
}

void AsyncWebSocketClient::_onError(int8_t) {
  // Serial.println("onErr");
}

void AsyncWebSocketClient::_onTimeout(uint32_t time) {
  if (!_client)
    return;
  // Serial.println("onTime");
  (void)time;
  _client->close(true);
}

void AsyncWebSocketClient::_onDisconnect() {
  // Serial.println("onDis");
  _client = nullptr;
}

void AsyncWebSocketClient::_onData(void* pbuf, size_t plen) {
  _lastMessageTime = millis();
  uint8_t* data = (uint8_t*)pbuf;
  while (plen > 0) {
    if (!_pstate) {
      const uint8_t* fdata = data;

      _pinfo.index = 0;
      _pinfo.final = (fdata[0] & 0x80) != 0;
      _pinfo.opcode = fdata[0] & 0x0F;
      _pinfo.masked = (fdata[1] & 0x80) != 0;
      _pinfo.len = fdata[1] & 0x7F;

      // log_d("WS[%" PRIu32 "]: _onData: %" PRIu32, _clientId, plen);
      // log_d("WS[%" PRIu32 "]: _status = %" PRIu32, _clientId, _status);
      // log_d("WS[%" PRIu32 "]: _pinfo: index: %" PRIu64 ", final: %" PRIu8 ", opcode: %" PRIu8 ", masked: %" PRIu8 ", len: %" PRIu64, _clientId, _pinfo.index, _pinfo.final, _pinfo.opcode, _pinfo.masked, _pinfo.len);

      data += 2;
      plen -= 2;

      if (_pinfo.len == 126 && plen >= 2) {
        _pinfo.len = fdata[3] | (uint16_t)(fdata[2]) << 8;
        data += 2;
        plen -= 2;

      } else if (_pinfo.len == 127 && plen >= 8) {
        _pinfo.len = fdata[9] | (uint16_t)(fdata[8]) << 8 | (uint32_t)(fdata[7]) << 16 | (uint32_t)(fdata[6]) << 24 | (uint64_t)(fdata[5]) << 32 | (uint64_t)(fdata[4]) << 40 | (uint64_t)(fdata[3]) << 48 | (uint64_t)(fdata[2]) << 56;
        data += 8;
        plen -= 8;
      }

      if (_pinfo.masked && plen >= 4) { // if ws.close() is called, Safari sends a close frame with plen 2 and masked bit set. We must not decrement plen which is already 0.
        memcpy(_pinfo.mask, data, 4);
        data += 4;
        plen -= 4;
      }
    }

    const size_t datalen = std::min((size_t)(_pinfo.len - _pinfo.index), plen);
    const auto datalast = data[datalen];

    if (_pinfo.masked) {
      for (size_t i = 0; i < datalen; i++)
        data[i] ^= _pinfo.mask[(_pinfo.index + i) % 4];
    }

    if ((datalen + _pinfo.index) < _pinfo.len) {
      _pstate = 1;

      if (_pinfo.index == 0) {
        if (_pinfo.opcode) {
          _pinfo.message_opcode = _pinfo.opcode;
          _pinfo.num = 0;
        }
      }
      if (datalen > 0)
        _server->_handleEvent(this, WS_EVT_DATA, (void*)&_pinfo, data, datalen);

      _pinfo.index += datalen;
    } else if ((datalen + _pinfo.index) == _pinfo.len) {
      _pstate = 0;
      if (_pinfo.opcode == WS_DISCONNECT) {
        if (datalen) {
          uint16_t reasonCode = (uint16_t)(data[0] << 8) + data[1];
          char* reasonString = (char*)(data + 2);
          if (reasonCode > 1001) {
            _server->_handleEvent(this, WS_EVT_ERROR, (void*)&reasonCode, (uint8_t*)reasonString, strlen(reasonString));
          }
        }
        if (_status == WS_DISCONNECTING) {
          _status = WS_DISCONNECTED;
          if (_client)
            _client->close(true);
        } else {
          _status = WS_DISCONNECTING;
          if (_client)
            _client->ackLater();
          _queueControl(WS_DISCONNECT, data, datalen);
        }
      } else if (_pinfo.opcode == WS_PING) {
        _server->_handleEvent(this, WS_EVT_PING, NULL, NULL, 0);
        _queueControl(WS_PONG, data, datalen);
      } else if (_pinfo.opcode == WS_PONG) {
        if (datalen != AWSC_PING_PAYLOAD_LEN || memcmp(AWSC_PING_PAYLOAD, data, AWSC_PING_PAYLOAD_LEN) != 0)
          _server->_handleEvent(this, WS_EVT_PONG, NULL, NULL, 0);
      } else if (_pinfo.opcode < WS_DISCONNECT) { // continuation or text/binary frame
        _server->_handleEvent(this, WS_EVT_DATA, (void*)&_pinfo, data, datalen);
        if (_pinfo.final)
          _pinfo.num = 0;
        else
          _pinfo.num += 1;
      }
    } else {
      // os_printf("frame error: len: %u, index: %llu, total: %llu\n", datalen, _pinfo.index, _pinfo.len);
      // what should we do?
      break;
    }

    // restore byte as _handleEvent may have added a null terminator i.e., data[len] = 0;
    if (datalen)
      data[datalen] = datalast;

    data += datalen;
    plen -= datalen;
  }
}

size_t AsyncWebSocketClient::printf(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  size_t len = vsnprintf(nullptr, 0, format, arg);
  va_end(arg);

  if (len == 0)
    return 0;

  char* buffer = new char[len + 1];

  if (!buffer)
    return 0;

  va_start(arg, format);
  len = vsnprintf(buffer, len + 1, format, arg);
  va_end(arg);

  bool enqueued = text(buffer, len);
  delete[] buffer;
  return enqueued ? len : 0;
}


namespace {
  AsyncWebSocketSharedBuffer makeSharedBuffer(const uint8_t* message, size_t len) {
    auto buffer = std::make_shared<std::vector<uint8_t>>(len);
    std::memcpy(buffer->data(), message, len);
    return buffer;
  }
}

bool AsyncWebSocketClient::text(AsyncWebSocketMessageBuffer* buffer) {
  bool enqueued = false;
  if (buffer) {
    enqueued = text(std::move(buffer->_buffer));
    delete buffer;
  }
  return enqueued;
}

bool AsyncWebSocketClient::text(AsyncWebSocketSharedBuffer buffer) {
  return _queueMessage(buffer);
}

bool AsyncWebSocketClient::text(const uint8_t* message, size_t len) {
  return text(makeSharedBuffer(message, len));
}

bool AsyncWebSocketClient::text(const char* message, size_t len) {
  return text((const uint8_t*)message, len);
}

bool AsyncWebSocketClient::text(const char* message) {
  return text(message, strlen(message));
}

bool AsyncWebSocketClient::text(const String& message) {
  return text(message.c_str(), message.length());
}


bool AsyncWebSocketClient::binary(AsyncWebSocketMessageBuffer* buffer) {
  bool enqueued = false;
  if (buffer) {
    enqueued = binary(std::move(buffer->_buffer));
    delete buffer;
  }
  return enqueued;
}

bool AsyncWebSocketClient::binary(AsyncWebSocketSharedBuffer buffer) {
  return _queueMessage(buffer, WS_BINARY);
}

bool AsyncWebSocketClient::binary(const uint8_t* message, size_t len) {
  return binary(makeSharedBuffer(message, len));
}

bool AsyncWebSocketClient::binary(const char* message, size_t len) {
  return binary((const uint8_t*)message, len);
}

bool AsyncWebSocketClient::binary(const char* message) {
  return binary(message, strlen(message));
}

bool AsyncWebSocketClient::binary(const String& message) {
  return binary(message.c_str(), message.length());
}


IPAddress AsyncWebSocketClient::remoteIP() const {
  if (!_client)
    return IPAddress((uint32_t)0U);

  return _client->remoteIP();
}

uint16_t AsyncWebSocketClient::remotePort() const {
  if (!_client)
    return 0;

  return _client->remotePort();
}

/*
 * Async Web Socket - Each separate socket location
 */

void AsyncWebSocket::_handleEvent(AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (_eventHandler != NULL) {
    _eventHandler(this, client, type, arg, data, len);
  }
}

AsyncWebSocketClient* AsyncWebSocket::_newClient(AsyncWebServerRequest* request) {
  _clients.emplace_back(request, this);
  _handleEvent(&_clients.back(), WS_EVT_CONNECT, request, NULL, 0);
  return &_clients.back();
}

bool AsyncWebSocket::availableForWriteAll() {
  return std::none_of(std::begin(_clients), std::end(_clients), [](const AsyncWebSocketClient& c) { return c.queueIsFull(); });
}

bool AsyncWebSocket::availableForWrite(uint32_t id) {
  const auto iter = std::find_if(std::begin(_clients), std::end(_clients), [id](const AsyncWebSocketClient& c) { return c.id() == id; });
  if (iter == std::end(_clients))
    return true;
  return !iter->queueIsFull();
}

size_t AsyncWebSocket::count() const {
  return std::count_if(std::begin(_clients), std::end(_clients), [](const AsyncWebSocketClient& c) { return c.status() == WS_CONNECTED; });
}

AsyncWebSocketClient* AsyncWebSocket::client(uint32_t id) {
  const auto iter = std::find_if(_clients.begin(), _clients.end(), [id](const AsyncWebSocketClient& c) { return c.id() == id && c.status() == WS_CONNECTED; });
  if (iter == std::end(_clients))
    return nullptr;

  return &(*iter);
}

void AsyncWebSocket::close(uint32_t id, uint16_t code, const char* message) {
  if (AsyncWebSocketClient* c = client(id))
    c->close(code, message);
}

void AsyncWebSocket::closeAll(uint16_t code, const char* message) {
  for (auto& c : _clients)
    if (c.status() == WS_CONNECTED)
      c.close(code, message);
}

void AsyncWebSocket::cleanupClients(uint16_t maxClients) {
  if (count() > maxClients)
    _clients.front().close();

  for (auto iter = std::begin(_clients); iter != std::end(_clients);) {
    if (iter->shouldBeDeleted())
      iter = _clients.erase(iter);
    else
      iter++;
  }
}

bool AsyncWebSocket::ping(uint32_t id, const uint8_t* data, size_t len) {
  AsyncWebSocketClient* c = client(id);
  return c && c->ping(data, len);
}

AsyncWebSocket::SendStatus AsyncWebSocket::pingAll(const uint8_t* data, size_t len) {
  size_t hit = 0;
  size_t miss = 0;
  for (auto& c : _clients)
    if (c.status() == WS_CONNECTED && c.ping(data, len))
      hit++;
    else
      miss++;
  return hit == 0 ? DISCARDED : (miss == 0 ? ENQUEUED : PARTIALLY_ENQUEUED);
}

bool AsyncWebSocket::text(uint32_t id, const uint8_t* message, size_t len) {
  AsyncWebSocketClient* c = client(id);
  return c && c->text(makeSharedBuffer(message, len));
}
bool AsyncWebSocket::text(uint32_t id, const char* message, size_t len) {
  return text(id, (const uint8_t*)message, len);
}
bool AsyncWebSocket::text(uint32_t id, const char* message) {
  return text(id, message, strlen(message));
}
bool AsyncWebSocket::text(uint32_t id, const String& message) {
  return text(id, message.c_str(), message.length());
}

bool AsyncWebSocket::text(uint32_t id, AsyncWebSocketMessageBuffer* buffer) {
  bool enqueued = false;
  if (buffer) {
    enqueued = text(id, std::move(buffer->_buffer));
    delete buffer;
  }
  return enqueued;
}
bool AsyncWebSocket::text(uint32_t id, AsyncWebSocketSharedBuffer buffer) {
  AsyncWebSocketClient* c = client(id);
  return c && c->text(buffer);
}

AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const uint8_t* message, size_t len) {
  return textAll(makeSharedBuffer(message, len));
}
AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const char* message, size_t len) {
  return textAll((const uint8_t*)message, len);
}
AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const char* message) {
  return textAll(message, strlen(message));
}
AsyncWebSocket::SendStatus AsyncWebSocket::textAll(const String& message) {
  return textAll(message.c_str(), message.length());
}
AsyncWebSocket::SendStatus AsyncWebSocket::textAll(AsyncWebSocketMessageBuffer* buffer) {
  AsyncWebSocket::SendStatus status = DISCARDED;
  if (buffer) {
    status = textAll(std::move(buffer->_buffer));
    delete buffer;
  }
  return status;
}

AsyncWebSocket::SendStatus AsyncWebSocket::textAll(AsyncWebSocketSharedBuffer buffer) {
  size_t hit = 0;
  size_t miss = 0;
  for (auto& c : _clients)
    if (c.status() == WS_CONNECTED && c.text(buffer))
      hit++;
    else
      miss++;
  return hit == 0 ? DISCARDED : (miss == 0 ? ENQUEUED : PARTIALLY_ENQUEUED);
}

bool AsyncWebSocket::binary(uint32_t id, const uint8_t* message, size_t len) {
  AsyncWebSocketClient* c = client(id);
  return c && c->binary(makeSharedBuffer(message, len));
}
bool AsyncWebSocket::binary(uint32_t id, const char* message, size_t len) {
  return binary(id, (const uint8_t*)message, len);
}
bool AsyncWebSocket::binary(uint32_t id, const char* message) {
  return binary(id, message, strlen(message));
}
bool AsyncWebSocket::binary(uint32_t id, const String& message) {
  return binary(id, message.c_str(), message.length());
}


bool AsyncWebSocket::binary(uint32_t id, AsyncWebSocketMessageBuffer* buffer) {
  bool enqueued = false;
  if (buffer) {
    enqueued = binary(id, std::move(buffer->_buffer));
    delete buffer;
  }
  return enqueued;
}
bool AsyncWebSocket::binary(uint32_t id, AsyncWebSocketSharedBuffer buffer) {
  AsyncWebSocketClient* c = client(id);
  return c && c->binary(buffer);
}

AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(const uint8_t* message, size_t len) {
  return binaryAll(makeSharedBuffer(message, len));
}
AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(const char* message, size_t len) {
  return binaryAll((const uint8_t*)message, len);
}
AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(const char* message) {
  return binaryAll(message, strlen(message));
}
AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(const String& message) {
  return binaryAll(message.c_str(), message.length());
}

AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(AsyncWebSocketMessageBuffer* buffer) {
  AsyncWebSocket::SendStatus status = DISCARDED;
  if (buffer) {
    status = binaryAll(std::move(buffer->_buffer));
    delete buffer;
  }
  return status;
}
AsyncWebSocket::SendStatus AsyncWebSocket::binaryAll(AsyncWebSocketSharedBuffer buffer) {
  size_t hit = 0;
  size_t miss = 0;
  for (auto& c : _clients)
    if (c.status() == WS_CONNECTED && c.binary(buffer))
      hit++;
    else
      miss++;
  return hit == 0 ? DISCARDED : (miss == 0 ? ENQUEUED : PARTIALLY_ENQUEUED);
}

size_t AsyncWebSocket::printf(uint32_t id, const char* format, ...) {
  AsyncWebSocketClient* c = client(id);
  if (c) {
    va_list arg;
    va_start(arg, format);
    size_t len = c->printf(format, arg);
    va_end(arg);
    return len;
  }
  return 0;
}

size_t AsyncWebSocket::printfAll(const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  size_t len = vsnprintf(nullptr, 0, format, arg);
  va_end(arg);

  if (len == 0)
    return 0;

  char* buffer = new char[len + 1];

  if (!buffer)
    return 0;

  va_start(arg, format);
  len = vsnprintf(buffer, len + 1, format, arg);
  va_end(arg);

  AsyncWebSocket::SendStatus status = textAll(buffer, len);
  delete[] buffer;
  return status == DISCARDED ? 0 : len;
}

const char __WS_STR_CONNECTION[] PROGMEM = {"Connection"};
const char __WS_STR_UPGRADE[] PROGMEM = {"Upgrade"};
const char __WS_STR_ORIGIN[] PROGMEM = {"Origin"};
const char __WS_STR_COOKIE[] PROGMEM = {"Cookie"};
const char __WS_STR_VERSION[] PROGMEM = {"Sec-WebSocket-Version"};
const char __WS_STR_KEY[] PROGMEM = {"Sec-WebSocket-Key"};
const char __WS_STR_PROTOCOL[] PROGMEM = {"Sec-WebSocket-Protocol"};
const char __WS_STR_ACCEPT[] PROGMEM = {"Sec-WebSocket-Accept"};
const char __WS_STR_UUID[] PROGMEM = {"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"};

#define WS_STR_UUID_LEN 36

#define WS_STR_CONNECTION FPSTR(__WS_STR_CONNECTION)
#define WS_STR_UPGRADE    FPSTR(__WS_STR_UPGRADE)
#define WS_STR_ORIGIN     FPSTR(__WS_STR_ORIGIN)
#define WS_STR_COOKIE     FPSTR(__WS_STR_COOKIE)
#define WS_STR_VERSION    FPSTR(__WS_STR_VERSION)
#define WS_STR_KEY        FPSTR(__WS_STR_KEY)
#define WS_STR_PROTOCOL   FPSTR(__WS_STR_PROTOCOL)
#define WS_STR_ACCEPT     FPSTR(__WS_STR_ACCEPT)
#define WS_STR_UUID       FPSTR(__WS_STR_UUID)

bool AsyncWebSocket::canHandle(AsyncWebServerRequest* request) const {
  return _enabled && request->isWebSocketUpgrade() && request->url().equals(_url);
}

void AsyncWebSocket::handleRequest(AsyncWebServerRequest* request) {
  if (!request->hasHeader(WS_STR_VERSION) || !request->hasHeader(WS_STR_KEY)) {
    request->send(400);
    return;
  }
  if (_handshakeHandler != nullptr) {
    if (!_handshakeHandler(request)) {
      request->send(401);
      return;
    }
  }
  const AsyncWebHeader* version = request->getHeader(WS_STR_VERSION);
  if (version->value().toInt() != 13) {
    AsyncWebServerResponse* response = request->beginResponse(400);
    response->addHeader(WS_STR_VERSION, T_13);
    request->send(response);
    return;
  }
  const AsyncWebHeader* key = request->getHeader(WS_STR_KEY);
  AsyncWebServerResponse* response = new AsyncWebSocketResponse(key->value(), this);
  if (request->hasHeader(WS_STR_PROTOCOL)) {
    const AsyncWebHeader* protocol = request->getHeader(WS_STR_PROTOCOL);
    // ToDo: check protocol
    response->addHeader(WS_STR_PROTOCOL, protocol->value());
  }
  request->send(response);
}

AsyncWebSocketMessageBuffer* AsyncWebSocket::makeBuffer(size_t size) {
  AsyncWebSocketMessageBuffer* buffer = new AsyncWebSocketMessageBuffer(size);
  if (buffer->length() != size) {
    delete buffer;
    return nullptr;
  } else {
    return buffer;
  }
}

AsyncWebSocketMessageBuffer* AsyncWebSocket::makeBuffer(const uint8_t* data, size_t size) {
  AsyncWebSocketMessageBuffer* buffer = new AsyncWebSocketMessageBuffer(data, size);
  if (buffer->length() != size) {
    delete buffer;
    return nullptr;
  } else {
    return buffer;
  }
}

/*
 * Response to Web Socket request - sends the authorization and detaches the TCP Client from the web server
 * Authentication code from https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSockets.cpp#L480
 */

AsyncWebSocketResponse::AsyncWebSocketResponse(const String& key, AsyncWebSocket* server) {
  _server = server;
  _code = 101;
  _sendContentLength = false;

  uint8_t hash[20];
  char buffer[33];

#if defined(ESP8266) || defined(TARGET_RP2040)
  sha1(key + WS_STR_UUID, hash);
#else
  String k;
  k.reserve(key.length() + WS_STR_UUID_LEN);
  k.concat(key);
  k.concat(WS_STR_UUID);
  SHA1Builder sha1;
  sha1.begin();
  sha1.add((const uint8_t*)k.c_str(), k.length());
  sha1.calculate();
  sha1.getBytes(hash);
#endif
  base64_encodestate _state;
  base64_init_encodestate(&_state);
  int len = base64_encode_block((const char*)hash, 20, buffer, &_state);
  len = base64_encode_blockend((buffer + len), &_state);
  addHeader(WS_STR_CONNECTION, WS_STR_UPGRADE);
  addHeader(WS_STR_UPGRADE, T_WS);
  addHeader(WS_STR_ACCEPT, buffer);
}

void AsyncWebSocketResponse::_respond(AsyncWebServerRequest* request) {
  if (_state == RESPONSE_FAILED) {
    request->client()->close(true);
    return;
  }
  String out;
  _assembleHead(out, request->version());
  request->client()->write(out.c_str(), _headLength);
  _state = RESPONSE_WAIT_ACK;
}

size_t AsyncWebSocketResponse::_ack(AsyncWebServerRequest* request, size_t len, uint32_t time) {
  (void)time;

  if (len)
    _server->_newClient(request);

  return 0;
}
