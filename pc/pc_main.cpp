#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <chrono>
#include <iostream>
#include <thread>

void setup();

int main(int argc, char** argv) {

  std::cout << "Starting Battery Emulator..." << std::endl;

#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

  setup();

  // Wait forever
  while (true) {
    // Simulate a long-running process
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
