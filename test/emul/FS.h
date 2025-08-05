#ifndef FS_H
#define FS_H

#include <Stream.h>
#include <WString.h>
#include <time.h>

namespace fs {

#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

enum SeekMode { SeekSet = 0, SeekCur = 1, SeekEnd = 2 };

class File : public Stream {
 public:
  const char* name() const { return "foobar"; }
  int available() override { return 0; }
  int read() override {
    return -1;  // EOF
  }

  bool seek(uint32_t pos, SeekMode mode) {
    (void)pos;
    (void)mode;
    return false;  // Not supported
  }
  bool seek(uint32_t pos) { return seek(pos, SeekSet); }

  int peek() override { return -1; }

  size_t write(uint8_t b) { return 1; }

  operator bool() const { return true; }

  size_t read(uint8_t* buf, size_t size) { return 0; }

  void close() {}
  bool isDirectory() { return false; }

  size_t size() const { return 0; }

  time_t getLastWrite() {
    // Not supported
    return 0;
  }
};

class FS {
 public:
  bool exists(const char* path) { return false; }
  bool exists(const String& path) { return false; }

  File open(const char* path, const char* mode = FILE_READ, const bool create = false) {
    return open(String(path), mode, create);
  }
  File open(const String& path, const char* mode = FILE_READ, const bool create = false) { return File(); }
};

}  // namespace fs
#endif
