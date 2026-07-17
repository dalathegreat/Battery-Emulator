class MDNSClass {
 public:
  bool begin(const char* hostname) { return true; }
  void addService(const char* service, const char* protocol, uint16_t port) {}
};
static MDNSClass MDNS;
