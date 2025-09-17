#include "TinyWebServer.h"

uint64_t millis64(void);
long random(long max);

//template<uint64_t (*millis64)()>
bool DigestAuthSessionManager::check(Nonce nonce, uint32_t nc) {
    uint64_t now = millis64();
    auto it = sessions.find(nonce);
    if(it == sessions.end()) {
        // New session
        clear_old_sessions();
        sessions[nonce] = {now, nc};
        return true;
    } else {
        // Existing session, check nonce count
        if(now - it->second.last_seen > timeout_ms) {
            // Session expired
            sessions.erase(it);
            return false;
        } else {
            // Session active, check nc
            if(nc > it->second.count) {
                it->second.last_seen = now;
                it->second.count = nc;
                return true;
            } else {
                // Replay attack or out-of-order
                return false;
            }
        }
    }
}

void DigestAuthSessionManager::clear_old_sessions() {
    uint64_t now = millis64();
    for(auto it = sessions.begin(); it != sessions.end(); ) {
        if(now - it->second.last_seen > timeout_ms) {
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}

//class DigestAuthCounter {
//   private:
//     std::map<char const*, int> counts;

template <typename HASH_CONTEXT, int HASH_TYPE>
DigestAuth<HASH_CONTEXT, HASH_TYPE>::DigestAuth(TwsHandler *handler, GetPasswordHashFunc getPasswordHash, DigestAuthSessionManager *sessionManager) : TwsStatefulHandler<DigestAuthState<HASH_CONTEXT>>(handler), getPasswordHash(getPasswordHash), sessionManager(sessionManager) {
    nextPostBody = handler->onPostBody;
    nextHeader = handler->onHeader;
    nextPartialHeader = handler->onPartialHeader;
    nextRequest = handler->onRequest;

    handler->onPostBody = this;
    handler->onHeader = this;
    handler->onPartialHeader = this;
    handler->onRequest = this;
}

template <typename HASH_CONTEXT, int HASH_TYPE>
void DigestAuth<HASH_CONTEXT, HASH_TYPE>::handleHeader(TwsRequest &request, const char *line, int len) {
    const char *line_ptr = line;
    while(len > 0) {
        // All headers goo via the partial handler
        int consumed = this->handlePartialHeader(request, line_ptr, len, true);
        if(consumed <= 0) {
            // It didn't like the full header at all? give up!
            break;
        }
        // The handler might not have consumed the whole header, so advance the
        // pointer and go around again.
        line_ptr += consumed;
        len -= consumed;
    }

    if(nextHeader) {
        nextHeader->handleHeader(request, line, len);
    }
}

enum {
    PARSING_HEADER_NAME = 0,
    PARSING_AUTH_TYPE,
    PARSING_KEY,
    PARSING_USERNAME,
    PARSING_NONCE,
    PARSING_URI,
    PARSING_RESPONSE,
    PARSING_NC,
    PARSING_CNONCE,
};

enum {
    HASHED_HA1 = 1,
    HASHED_NONCE,
    HASHED_NC,
};

template <typename HASH_CONTEXT, int HASH_TYPE>
int DigestAuth<HASH_CONTEXT, HASH_TYPE>::handlePartialHeader(TwsRequest &request, const char *line, int len, bool final) {
    auto &state = this->get_state(request);

    // Temporary buffer for the hex representation of hash output.
    // Needs to be long enough for the longest hash (SHA-256)
    char hash_output_hex[64];

    // By default we consume the whole buffer.
    int consumed = len;

    auto parse_into_hash = [&](auto hash, char delimiter, const char* suffix) -> bool {
        // Feed data into the supplied hasher up until the delimiter.
        // If a suffix is supplied, feed that into the hash at the end.
        char *value_end = strchr((char*)line, delimiter);
        if(value_end) {
            // Found ending delimiter
            int value_len = value_end - line;
            hash->update(line, value_len);
            if(suffix && *suffix) {
                hash->update(suffix, strlen(suffix));
            }
            consumed = value_len + 1; // +1 to include the delimiter
            return true;
        } else {
            hash->update(line, len);
        }
        return false;
    };

    auto check_auth = [&](const char hash_hex[]) {
        // Check whether we successfully authenticated.
        printf("Checking auth: %.*s : %.*s\n", state.r.getHashLengthHex(), state.response, state.r.getHashLengthHex(), hash_hex);
        if(strncmp(state.response, hash_hex, state.r.getHashLengthHex()) == 0) {
            state.authed = true;
        } else {
            state.authed = false;
        }
        state.response[0] = '\0'; // reset response after checking
    };

    switch(state.parse_state) {
    case PARSING_HEADER_NAME: // header name
        if(strncasecmp(line, "Authorization:", 14) == 0) {
            state.parse_state = PARSING_AUTH_TYPE;
            state.response[0] = '\0'; // reset response
            hash_output_hex[0] = '\0'; // reset hash output hex
            consumed = 14;

            // Start calculating HA2
            state.ha2.init();
            state.ha2.update("GET:", 4); // TODO: use actual method

            // Start calculating R
            state.r.init();

            goto trim;
        }
        // Probably a different header, skip it.
        return 0; 
    case PARSING_AUTH_TYPE: // auth type
        if(strncasecmp(line, "Digest ", 7) == 0) {
            state.parse_state = PARSING_KEY;
            consumed = 7;
        }
        goto trim;
    case PARSING_KEY: // key (of a key=value pair)
        if(strncasecmp(line, "username=\"", 10) == 0) {
            state.parse_state = PARSING_USERNAME;
            consumed = 10;
        } else if(state.hash_state == HASHED_HA1 && strncasecmp(line, "nonce=\"", 7) == 0) {
            state.parse_state = PARSING_NONCE; // parse nonce
            consumed = 7;
        } else if(strncasecmp(line, "uri=\"", 5) == 0) {
            state.parse_state = PARSING_URI; // parse uri
            consumed = 5;
        } else if(strncasecmp(line, "response=\"", 10) == 0) {
            state.parse_state = PARSING_RESPONSE; // parse response
            consumed = 10;
        } else if(state.hash_state == HASHED_NONCE && strncasecmp(line, "nc=", 3) == 0) {
            state.parse_state = PARSING_NC; // parse nc
            consumed = 3;
        } else if(strncasecmp(line, "cnonce=\"", 8) == 0) {
            state.parse_state = PARSING_CNONCE; // parse cnonce
            consumed = 8;
        } else {
            // skip past the next comma
            const char *comma = strchr(line, ',');
            if(comma) {
                consumed = comma - line + 1; // +1 to include the comma
            }
            goto trim;
        }
        goto skip_trim;
    case PARSING_USERNAME: // username
        {
            char *username_end = strchr((char*)line, '"');
            if(username_end) {
                consumed = username_end - line + 1; // +1 to include the closing quote
                state.parse_state = PARSING_KEY;

                *username_end = '\0'; // terminate the string

                int ha1_len = getPasswordHash(line, hash_output_hex);
                state.r.update(hash_output_hex, ha1_len);
                state.r.update(":", 1);
                state.hash_state = HASHED_HA1; // {ha1}:
                state.parse_state = PARSING_KEY;
            } else {
                // wait for a complete string

                // shortcut nothing-consumed-check
                return 0;
            }
        }
        goto trim;
    case PARSING_NONCE: // nonce
        if(len>=NONCE_LENGTH && sessionManager) {
            // Store the nonce for checking with the session manager later
            memcpy(state.nonce.data(), line, NONCE_LENGTH);
        }
        if(parse_into_hash(&state.r, '"', ":")) {
            state.parse_state = PARSING_KEY;
            state.hash_state = HASHED_NONCE; // {ha1}:{nonce}:
            goto trim;
        }
        goto skip_trim;
    case PARSING_URI: // uri
        if(parse_into_hash(&state.ha2, '"', "")) {
            state.parse_state = PARSING_KEY;
            goto trim;
        }
        goto skip_trim;
    case PARSING_RESPONSE:
        {
            char *response_end = strchr((char*)line, '"');
            if(response_end) {
                consumed = response_end - line + 1; // +1 to include the closing quote
                state.parse_state = PARSING_KEY;

                if(state.response[0] == '\0') {
                    // We're receiving the response param before we've calculated the hash,
                    // so store and we'll check later.
                    memcpy(state.response, line, state.r.getHashLengthHex());
                } else {
                    check_auth(line);
                }
            } else {
                // wait for a complete string

                // shortcut nothing-consumed-check
                return 0;
            }
        }
        goto trim;   
    case PARSING_NC: // nc
        if(len>=8 && sessionManager) {
            // Should always be 8 hex digits
            char nc_str[9];
            memcpy(nc_str, line, 8);
            nc_str[8] = '\0';
            uint32_t nc = strtoul(nc_str, nullptr, 16);
            if(!sessionManager->check(state.nonce, nc)) {
                state.denied = true;
            }
        }

        if(parse_into_hash(&state.r, ',', ":")) {
            state.parse_state = PARSING_KEY;
            state.hash_state = HASHED_NC; // {ha1}:{nonce}:{nc}:
            goto trim;
        }
        goto skip_trim;
    case PARSING_CNONCE: // cnonce
        // Special case for cURL which annoyingly sends nc after cnonce (despite
        // it needing to be before it in the hash).
        if(state.hash_state == HASHED_NONCE) {
            // Hope that the full nc is later on in the buffer
            char* nc_start = strstr((char*)line, "\", nc=");
            if(nc_start) {
                nc_start += 6;

                char* nc_end = strchr(nc_start, ',');
                if(nc_end) {
                    state.r.update(nc_start, nc_end - nc_start);
                    state.r.update(":", 1);
                    state.hash_state = HASHED_NC; // {ha1}:{nonce}:{nc}:
                }
            }
        }

        if(state.hash_state == HASHED_NC && parse_into_hash(&state.r, '"', "")) {
            state.parse_state = PARSING_KEY;

            // Finish calculating ha2 and convert to hex.
            state.ha2.toHex(hash_output_hex);

            // Finish calculating r and convert to hex
            state.r.update(":auth:", 6);
            state.r.update(hash_output_hex, state.ha2.getHashLengthHex());
            state.r.toHex(hash_output_hex);

            if(state.response[0] == '\0') {
                // We haven't received the response param yet, so store the hash
                // there instead and we'll compare later.
                memcpy(state.response, hash_output_hex, state.r.getHashLengthHex());
            } else {
                check_auth(hash_output_hex);
            }
            goto trim;
        }
        goto skip_trim;
    default:
        DEBUG_PRINTF("erk rest of header: %.*s\n", len, line);
        DEBUG_PRINTF("erk consuming %d\n", consumed);
    }

trim:
    // line is nul-terminated so this is safe
    while(line[consumed] == ' ' || line[consumed] == '\t') {
        consumed++;
    }
skip_trim:

    //printf("state is now %d, consumed %d\n", state.parse_state, consumed);
    if(consumed==0) {
        DEBUG_PRINTF("erk, no consumed bytes, state is %d\n", state.parse_state);
    }

    if(consumed==len && final) state.parse_state = PARSING_HEADER_NAME; // reset state after final

    return consumed;
}

template <typename HASH_CONTEXT, int HASH_TYPE>
bool DigestAuth<HASH_CONTEXT, HASH_TYPE>::denyIfUnauthed(TwsRequest &request) {
    auto &state = this->get_state(request);
    if(!state.authed || state.denied) {
        request.write_fully("HTTP/1.1 401 Unauthorized\r\n"
                                "Connection: close\r\n"
                                "WWW-Authenticate: Digest "
                                "qop=\"auth\", "
                                "algorithm=");
        if(HASH_TYPE==0) { // MD5
            request.write_fully("MD5, nonce=\"");
        } else { // SHA-256
            request.write_fully("SHA-256, nonce=\"");
        }
        char buf[NONCE_LENGTH+1];
        for(int i=0; i<NONCE_LENGTH; i++) {
            buf[i] = "0123456789abcdefghijklmnopqrstuv"[random(32)];
        }
        buf[NONCE_LENGTH] = '\0';
        request.write_fully(buf);
        request.write_fully("\"\r\n\r\n");
                                
        //                         MD5, "
        //                         "nonce=\"");

        // if(HASH_TYPE==0) { // MD5
        //     request.write_fully("HTTP/1.1 401 Unauthorized\r\n"
        //                         "Connection: close\r\n"
        //                         "WWW-Authenticate: Digest "
        //                         "qop=\"auth\", "
        //                         "algorithm=MD5, "
        //                         "nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\"\r\n"
        //                         "\r\n\n");
        // } else { // SHA-256
        //     request.write_fully("HTTP/1.1 401 Unauthorized\r\n"
        //                         "Connection: close\r\n"
        //                         "WWW-Authenticate: Digest "
        //                         "qop=\"auth\", "
        //                         "algorithm=SHA-256, "
        //                         "nonce=\"7ypf/xlj9XXwfDPEoM4URrv/xwf94BcCAzFZH4GiTo0v\"\r\n"
        //                         "\r\n\n");        
        // }
        request.finish();
        return true;
    }
    return false;
}

template <typename HASH_CONTEXT, int HASH_TYPE>
bool DigestAuth<HASH_CONTEXT, HASH_TYPE>::handlePostBody(TwsRequest &request, size_t index, uint8_t *data, size_t len) {
    // Handle post body if needed
    if(denyIfUnauthed(request)) {
        return true;
    }

    if(nextPostBody) {
        return nextPostBody->handlePostBody(request, index, data, len);
    }
    return true;
}

template <typename HASH_CONTEXT, int HASH_TYPE>
void DigestAuth<HASH_CONTEXT, HASH_TYPE>::handleRequest(TwsRequest &request) {
    if(denyIfUnauthed(request)) {
        return;
    }

    if(nextRequest) {
        nextRequest->handleRequest(request);
    }
}

// Only instantiating the one you're using saves .text ?
template class DigestAuth<Md5Hash, 0>;
//template class DigestAuth<Sha256Hash, 1>;


