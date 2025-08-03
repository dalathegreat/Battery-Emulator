#ifndef EMUL_FDSET_H
#define EMUL_FDSET_H

#include <list>

struct emul_fd_set {
  std::list<int> sockets;
};

#endif
