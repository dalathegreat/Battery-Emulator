g++ -o tws TinyWebServer.cpp DigestAuth.cpp MultipartUploadHandler.cpp BasicAuth.cpp Local.cpp -DLOCAL -Os -fstack-protector-all -fsanitize=address  -fsanitize-address-use-after-scope -lmbedtls -lmbedcrypto -Wall -Wno-deprecated-declarations

