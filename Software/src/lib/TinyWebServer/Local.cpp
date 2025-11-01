#ifdef LOCAL
#include "TinyWebServer.h"

#include <memory>
#include <stdio.h>
// //#include <sys/socket.h>
// #include <netinet/tcp.h>
// #include <unistd.h>
#include <string.h>
// #include <netinet/in.h>
#include <time.h>
// #include <mcheck.h>

extern "C" {
    extern void *__libc_malloc(size_t size);
    extern void *__libc_free(void *ptr);

    char malloc_log[104857600] = {0};
    char* malloc_log_ptr = malloc_log;

    void* malloc (size_t size) {
        // void *caller = __builtin_return_address(0);
        // if (malloc_hook_active)
        //     return my_malloc_hook(size, caller);
        //puts("malloc called\n");
        //printf("malloc called with size %zu\n", size);
        // print to mallog_loc
        if (malloc_log_ptr + size >= malloc_log + sizeof(malloc_log)) {
            DEBUG_PRINTF("malloc log full, cannot allocate more memory\n");
            return nullptr;
        }
        // Store the allocation in the log
        void* ptr = __libc_malloc(size);
        malloc_log_ptr += sprintf(malloc_log_ptr, "malloc(%zu) at %p\n", size, ptr);
        return ptr;
    }

    void free(void *ptr) {
        // print to mallog_loc
        malloc_log_ptr += sprintf(malloc_log_ptr, "free(%p)\n", ptr);
        __libc_free(ptr);
    }
}

unsigned long millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

uint64_t millis64(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
}

long random(long max) {
    return rand() % max;
}

class String {
public:
    String() {
    }
    ~String() {
        if (_buf) {
            //tws_log_printf("deallocating buf! %p\n", _buf);
            delete[] _buf;
        }
    }
    String(const char* str) {
        _buf = new char[BUFLEN];
        DEBUG_PRINTF("allocating buf! %p\n", _buf);
        strncpy(_buf, str, BUFLEN);
    }
    // copy constructor
    String(const String &other) {
        if (other._buf) {
            _buf = new char[BUFLEN];
            DEBUG_PRINTF("copying string!\n");
            strncpy(_buf, other._buf, BUFLEN);
        } else {
            _buf = nullptr;
        }
    }
    // move constructor
    String(String &&other) noexcept {
        _buf = other._buf;
        other._buf = nullptr; // Transfer ownership
        DEBUG_PRINTF("moving string! %p\n", _buf);
    }
    void concat(const char* str) {
        if (_buf == nullptr) {
            _buf = new char[BUFLEN];
            _buf[0] = '\0'; // Initialize as an empty string
        }
        strncat(_buf, str, BUFLEN - strlen(_buf) - 1);
    }
    int length() const {
        return _buf ? strlen(_buf) : 0;
    }
    void clear() {
        if (_buf) {
            _buf[0] = '\0'; // Clear the string
        }
    }
    const char* c_str() const {
        return _buf ? _buf : "";
    }
private:
    static const int BUFLEN = 1048576;
    char* _buf = nullptr;
};

//String strings[TinyWebServer::MAX_REQUESTS];

class StringLike {
public:
    StringLike(String &str) : _str(str), _length(_str.length()) {}
    StringLike(const char *chr) : _chr(chr), _length(strlen(chr)) {}
    StringLike(const char *chr, int length) : _chr(chr) , _length(length) {}

    const char* c_str() const {
        return _chr ? _chr : _str.c_str();
    }
    int length() const {
        return _length;
    }
private:
    String _str;
    const char *_chr = nullptr;
    int _length = 0;
};

TwsRequestWriterCallbackFunction StringListWriter(std::shared_ptr<std::vector<StringLike>> &response) {
    return [response = std::move(response)](TwsRequest &req, int alreadyWritten) {
        if(alreadyWritten==0) {
            DEBUG_PRINTF("first String is at %p\n", &(response->at(0)));
        }
        for(auto &str : *response) {
            int length = str.length();
            if(alreadyWritten >= length) {
                alreadyWritten -= length;
                DEBUG_PRINTF("Skipping already written part, alreadyWritten is now %d\n", alreadyWritten);
                continue; // Skip already written parts
            }

            DEBUG_PRINTF("Going to write: [%s] from offset %d\n", str.c_str(), alreadyWritten);

            const uint32_t remaining = length - alreadyWritten;
            if(req.write_direct(str.c_str() + alreadyWritten, remaining) < remaining) {
                // Done for now
                DEBUG_PRINTF("done for now\n");
                return;
            }
            alreadyWritten = 0; // Reset alreadyWritten for the next string
        }  

        req.finish();
    };
}

TwsRequestWriterCallbackFunction StringWriter(std::shared_ptr<String> &response) {
    return [response](TwsRequest &req, int alreadyWritten) {
        const int remaining = response->length() - alreadyWritten;
        if(remaining <= 0) {
            DEBUG_PRINTF("TWS finished %d %d\n", alreadyWritten, response->length());
            req.finish(); // No more data to write, finish the request
            return;
        }
        int wrote = req.write_direct(response->c_str() + alreadyWritten, remaining);
        if(wrote>=remaining) {
            req.finish();
        }
    };
}

TwsRequestWriterCallbackFunction CharBufWriter(const char* buf, int len) {
    return [buf, len](TwsRequest &req, int alreadyWritten) {
        const int remaining = len - alreadyWritten;
        if(remaining <= 0) {
            req.finish(); // No more data to write, finish the request
            return;
        }
        req.write_direct(buf + alreadyWritten, remaining);
    };
}

// TwsRequestWriterCallbackFunction IndexedStringWriter() {
//     return [](TwsRequest &req, int alreadyWritten) {
//         auto response = &strings[req.get_slot_id()];
//         const int remaining = response->length() - alreadyWritten;
//         if(remaining <= 0) {
//             tws_log_printf("TWS finished %d %d\n", alreadyWritten, response->length());
//             req.finish(); // No more data to write, finish the request
//             return;
//         }
//         int wrote = req.write_direct(response->c_str() + alreadyWritten, remaining);
//         if(wrote>=remaining) {
//             req.finish();
//         }
//     };
// }

/*
TwsRequestHandlerEntry *default_handlers[] = {
    new TwsRequestHandlerEntry("/", TWS_HTTP_GET, [](TwsRequest& request) {
        tws_log_printf("creating response\n");
        auto response = std::make_shared<String>();
        //auto response = String();
        //response->reserve(5000);
        response->concat("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\n"
                    "Yes this is a jolly good test.\n");
        response->concat(malloc_log);
        request.set_writer_callback(StringWriter(response));
    }),
    new TwsRequestHandlerEntry("/yah", TWS_HTTP_GET, [](TwsRequest& request) {
        int client_id = request.get_slot_id();
        strings[client_id].clear();
        auto response = &strings[client_id];
        response->concat("HTTP/1.1 200 OK\r\n"
                    "Connection: close\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\n"
                    "Yes this is a jolly good test.\n");
        response->concat(malloc_log);
        request.set_writer_callback(IndexedStringWriter());
    }),
    new TwsRequestHandlerEntry("/moo", TWS_HTTP_GET, [](TwsRequest& request) {
        request.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<form method=\"post\" enctype=\"multipart/form-data\">\n"
                    "<input type=\"file\" name=\"file\" />\n"
                    "<input type=\"submit\" value=\"Upload\" />\n"
                    "</form>\n");
        request.finish();
    }),
    new TwsRequestHandlerEntry("/upload", TWS_HTTP_GET, [](TwsRequest& request) {
        request.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<input type=\"file\" onchange=\""
            "const c = new FormData;"
            "let l = this.files[0];"
            "let i = new XMLHttpRequest;"
            "i.open('POST', '/ota/upload');"
            "c.append('file', l, l.name);"
            "i.send(c);"
            "\" />\n");

        request.finish();
    }),
    new TwsMultipartUploadHandler("/ota/upload", [](TwsRequest& request) {
        //tws_log_printf("request yeah\n");
        request.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nFile uploaded successfully.\n");
        request.finish();
    }, [](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> bool {
        tws_log_printf("Received upload: index: %zu, len: %zu\n", index, len);
        return false;
    }),
    new TwsRequestHandlerEntry("OLD/ota/upload", TWS_HTTP_POST, [](TwsRequest& request) {
        tws_log_printf("request yeah\n");


    }, MultipartPostHandler([](TwsRequest& request, size_t index, uint8_t *data, size_t len) -> bool {
        tws_log_printf("Received upload: index: %zu, len: %zu\n", index, len);
        return false;
        // if (final) {
        //     request.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nFile uploaded successfully.\n");
        // } else {
        //     request.write("HTTP/1.1 100 Continue\r\n\r\n");
        // }
        // request.finish();
    })),
    new TwsRequestHandlerEntry("/auth", TWS_HTTP_GET, [](TwsRequest& request) {
        if(last_authed_connection[request.get_slot_id()] == request.get_connection_id()) {
            request.write("HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "You are authenticated!\n");
        } else {
            request.write("HTTP/1.1 401 Unauthorized\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/plain\r\n"
                        "WWW-Authenticate: Basic realm=\"TinyWebServer\"\r\n"
                        "\r\n"
                        "This is a protected resource.\n");
        }

        request.finish();
    }, nullptr, nullptr, [](TwsRequest &request, const char *line, int len, bool final) {
        // Handle headers
        if(strcmp(line, "Authorization: Basic YXV0aDphdXRo")==0) {
            tws_log_printf("Successful auth!\n");
            last_authed_connection[request.get_slot_id()] = request.get_connection_id();
        }
        //tws_log_printf("Received header: %s\n", line);
    }),
    new TwsRequestHandlerEntry(nullptr, 0, nullptr) // Sentinel entry to mark the end of the array
};*/

typedef TwsWrappedHandler2<BasicAuth, BasicAuth> AuthedHandler;

#include "frontend.h"
const char* HTTP_RESPONSE_GZIP = "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Encoding: gzip\r\n"
                        "\r\n";

TwsRoute indexHandler("*", new TwsRequestHandlerFunc([](TwsRequest &request) {
    request.write_fully(HTTP_RESPONSE_GZIP, strlen(HTTP_RESPONSE_GZIP));
    request.set_writer_callback(CharBufWriter((const char*)html_data, sizeof(html_data)));

    // //tws_log_printf("Received request on /\n");
    // request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nHello from TinyWebServer!\n");
    // //request.set_writer_callback(StringWriter(strings[request.get_slot_id()]));
    // //String response = String();
    // auto response = std::make_shared<String>();
    // char text_buf[1000000] = {0};
    // FILE *f = fopen("/home/jonny/tmp/Battery-Emulator/LICENSE", "r");
    // if(fread(text_buf, 1, sizeof(text_buf) - 1, f) < 0) {
    //     strncpy(text_buf, "Failed to read file\n", sizeof(text_buf) - 1);
    // }
    // fclose(f);
    // response->concat(text_buf);
    // request.set_writer_callback(StringWriter(response));
    //request.finish();
}));
DigestAuthSessionManager sessions;
Md5DigestAuth authHandler(&indexHandler, [](const char *username, char *output) -> int {
    if(strcmp(username, "auth")==0) {
        memcpy(output, "0169de8171b73f43b80a560059a38aa9", 32); // auth::auth
        return 32;
    }
    if(strcmp(username, "foo")==0) {
        memcpy(output, "e0a109b991367f513dfa73bbae05fb07", 32); // foo::bar
        return 32;
    }
    if(strcmp(username, "ping")==0) {
        memcpy(output, "9e659e97072e03958a07f326163bfc52", 32); // ping::pong
        return 32;
    }
    return 0;
}, &sessions);

TwsRoute uploadHandler("/upload", new TwsRequestHandlerFunc([](TwsRequest& request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<input type=\"file\" onchange=\""
        "const c = new FormData;"
        "let l = this.files[0];"
        "let i = new XMLHttpRequest;"
        "i.open('POST', '/ota/upload');"
        "c.append('file', l, l.name);"
        //"c.append('foo', 'bar');"
        "i.send(c);"
        "\" />\n");

    request.finish();
}));
//BasicAuth authHandler2(uploadHandler);


TwsRoute uploadHandler2("/ota/upload", 
    new TwsRequestHandlerFunc([](TwsRequest& request) {
        //tws_log_printf("Finished request on /ota/upload\n");
        request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nFile uploaded successfully.\n");
        request.finish();
    })
);
MultipartUploadHandler uploadWotsit(&uploadHandler2, new TwsFileUploadHandlerFunc([](TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) {
    DEBUG_PRINTF("Received upload: key: %s, filename: %s, index: %zu, len: %zu, end: %zu, final: %d\n", key, filename, index, len, index+len, final);
    // FILE *myfile = fopen("/tmp/blah.bin", "a");
    // fwrite(data, 1, len, myfile);
    // fclose(myfile);
}));
//    BasicAuth authHandler3(&uploadHandler2);

// TwsRoute eOtaUploadHandler("/ota/upload", 
//     new TwsRequestHandlerFunc([](TwsRequest& request) {
//         //tws_log_printf("Finished request on /ota/upload\n");
//         request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nOK");
//         request.finish();
//     })
// );
// EOtaUpload eOtaUpload(&eOtaUploadHandler);

String mystr("I'm a string!");

TwsRoute stringHandler("/strings", new TwsRequestHandlerFunc([](TwsRequest &request) {
    auto strings = std::make_shared<std::vector<StringLike>>();
    strings->reserve(5);
    strings->push_back(StringLike("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n"));
    strings->push_back(StringLike("here's a string\n"));
    strings->push_back(StringLike("and another one\n"));
    strings->push_back(StringLike("and yet another\n"));
    DEBUG_PRINTF("pushing String\n");
    strings->push_back(StringLike(mystr));
    DEBUG_PRINTF("creating SLW\n");
    request.set_writer_callback(StringListWriter(strings));
}));

TwsRoute settingsHandler("/settings", new TwsRequestHandlerFunc([](TwsRequest &request) {
    request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nSettings go here\n");
    request.finish();
}));
TwsPostBufferingRequestHandler settingsBufferingHandler(&settingsHandler, [](TwsRequest &request, uint8_t *data, size_t len) {
    // Handle posted settings data here
    DEBUG_PRINTF("Received settings data: len: %zu\n", len);
    DEBUG_PRINTF("Data: %.*s\n", (int)len, data);
    // Just echo it back for now
    // request.write_fully("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n");
    // request.write_direct(data, len);
    // request.finish();
});

//EOtaStart eOtaHandler("/ota/start");
TwsRoute *handlers[] = {
    &settingsHandler,
    &indexHandler,
    &uploadHandler,
    &stringHandler,
    //&eOtaUploadHandler,
    //uploadHandler2,
    //eOtaHandler,
    // Add more handlers as needed
    nullptr,
};

int main() {
    //mtrace();

    // blah.onUpload = new TwsFileUploadHandlerFunc([](TwsRequest &request, const char *key, const char *filename, size_t index, uint8_t *data, size_t len, bool final) {
    //     tws_log_printf("Received upload: key: %s, filename: %s, index: %zu, len: %zu, final: %d\n", key, filename, index, len, final);
    // });
    // handler.onRequest = new TwsRequestHandlerFunc([](TwsRequest &request) {
    //     tws_log_printf("Received request on /\n");
    //     request.write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nHello from TinyWebServer!\n");
    //     request.finish();
    // });
    //BasicAuth authHandler(handler);

    

    TinyWebServer tws(12345, handlers);
    tws.open_listen_port();

    while (true) {
        tws.poll();
        //tws.tick();
        //usleep(50000); // Sleep for 50ms
    }

    return 0;
}
#endif