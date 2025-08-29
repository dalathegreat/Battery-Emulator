#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdint.h>
#include "emul_fdset.h"
#include "ip_addr.h"

typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;

typedef s8_t err_t;

#define CONFIG_LWIP_TCP_SND_BUF_DEFAULT 5760
#define TCP_SND_BUF CONFIG_LWIP_TCP_SND_BUF_DEFAULT

#ifndef _WIN32
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#else
typedef int _ssize_t;

#ifndef _SSIZE_T_DECLARED
typedef _ssize_t ssize_t;
#define _SSIZE_T_DECLARED
#endif

typedef emul_fd_set fd_set;

inline void FD_SET(int fd, fd_set* set) {
  if (set) {
    set->sockets.push_back(fd);
  }
}

inline void FD_CLR(int fd, fd_set* set) {
  if (set) {
    set->sockets.remove(fd);
  }
}

inline void FD_ZERO(fd_set* set) {
  if (set) {
    set->sockets.clear();
  }
}

inline bool FD_ISSET(int fd, fd_set* set) {
  if (set) {
    for (int socket : set->sockets) {
      if (socket == fd) {
        return true;
      }
    }
  }
  return false;
}

#define _FOPEN (-1)         /* from sys/file.h, kernel use only */
#define _FREAD 0x0001       /* read enabled */
#define _FWRITE 0x0002      /* write enabled */
#define _FAPPEND 0x0008     /* append (writes guaranteed at the end) */
#define _FMARK 0x0010       /* internal; mark during gc() */
#define _FDEFER 0x0020      /* internal; defer for next gc pass */
#define _FASYNC 0x0040      /* signal pgrp when data ready */
#define _FSHLOCK 0x0080     /* BSD flock() shared lock present */
#define _FEXLOCK 0x0100     /* BSD flock() exclusive lock present */
#define _FCREAT 0x0200      /* open with file create */
#define _FTRUNC 0x0400      /* open with truncation */
#define _FEXCL 0x0800       /* error on open if file exists */
#define _FNBIO 0x1000       /* non blocking I/O (sys5 style) */
#define _FSYNC 0x2000       /* do all writes synchronously */
#define _FNONBLOCK 0x4000   /* non blocking I/O (POSIX style) */
#define _FNDELAY _FNONBLOCK /* non blocking I/O (4.2 style) */
#define _FNOCTTY 0x8000     /* don't assign a ctty on this open */
#if defined(__CYGWIN__)
#define _FBINARY 0x10000
#define _FTEXT 0x20000
#endif
#define _FNOINHERIT 0x40000
#define _FDIRECT 0x80000
#define _FNOFOLLOW 0x100000
#define _FDIRECTORY 0x200000
#define _FEXECSRCH 0x400000
#if defined(__CYGWIN__)
#define _FTMPFILE 0x800000
#define _FNOATIME 0x1000000
#define _FPATH 0x2000000
#endif

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

#define F_DUPFD 0 /* Duplicate fildes */
#define F_GETFD 1 /* Get fildes flags (close on exec) */
#define F_SETFD 2 /* Set fildes flags (close on exec) */
#define F_GETFL 3 /* Get file flags */
#define F_SETFL 4 /* Set file flags */

#define O_RDONLY 0 /* +1 == FREAD */
#define O_WRONLY 1 /* +1 == FWRITE */
#define O_RDWR 2   /* +1 == FREAD|FWRITE */
#define O_APPEND _FAPPEND
#define O_CREAT _FCREAT
#define O_TRUNC _FTRUNC
#define O_EXCL _FEXCL
#define O_SYNC _FSYNC
/*	O_NDELAY	_FNDELAY 	set in include/fcntl.h */
/*	O_NDELAY	_FNBIO 		set in include/fcntl.h */
#define O_NONBLOCK _FNONBLOCK
#define O_NOCTTY _FNOCTTY

typedef struct timeval {
  long tv_sec;
  long tv_usec;
} TIMEVAL, *PTIMEVAL, *LPTIMEVAL;

int emul_select(int __n, emul_fd_set* __readfds, emul_fd_set* __writefds, emul_fd_set* __exceptfds,
                struct timeval* __timeout);
#define select(n, readfds, writefds, exceptfds, timeout) emul_select((n), (readfds), (writefds), (exceptfds), (timeout))

int fcntl(int, int, ...);

#define IPPROTO_IP 0
#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#if LWIP_IPV6
#define IPPROTO_IPV6 41
#define IPPROTO_ICMPV6 58
#endif /* LWIP_IPV6 */
#define IPPROTO_UDPLITE 136
#define IPPROTO_RAW 255

#define SOL_SOCKET 0xfff /* options for socket level */

#define AF_UNSPEC 0
#define AF_INET 2
#if LWIP_IPV6
#define AF_INET6 10
#else /* LWIP_IPV6 */
#define AF_INET6 AF_UNSPEC
#endif /* LWIP_IPV6 */
#define PF_INET AF_INET
#define PF_INET6 AF_INET6
#define PF_UNSPEC AF_UNSPEC

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3

#define SO_DEBUG 0x0001       /* Unimplemented: turn on debugging info recording */
#define SO_ACCEPTCONN 0x0002  /* socket has had listen() */
#define SO_DONTROUTE 0x0010   /* Unimplemented: just use interface addresses */
#define SO_USELOOPBACK 0x0040 /* Unimplemented: bypass hardware when possible */
#define SO_LINGER 0x0080      /* linger on close if data present */
#define SO_DONTLINGER ((int)(~SO_LINGER))
#define SO_OOBINLINE 0x0100    /* Unimplemented: leave received OOB data in line */
#define SO_REUSEPORT 0x0200    /* Unimplemented: allow local address & port reuse */
#define SO_SNDBUF 0x1001       /* Unimplemented: send buffer size */
#define SO_RCVBUF 0x1002       /* receive buffer size */
#define SO_SNDLOWAT 0x1003     /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT 0x1004     /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO 0x1005     /* send timeout */
#define SO_RCVTIMEO 0x1006     /* receive timeout */
#define SO_ERROR 0x1007        /* get error status and clear */
#define SO_TYPE 0x1008         /* get socket type */
#define SO_CONTIMEO 0x1009     /* Unimplemented: connect timeout */
#define SO_NO_CHECK 0x100a     /* don't create UDP checksum */
#define SO_BINDTODEVICE 0x100b /* bind to device */

#define TCP_NODELAY 0x01   /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE 0x02 /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE 0x03  /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL 0x04 /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT 0x05   /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */

typedef u32_t socklen_t;
typedef u8_t sa_family_t;
typedef u16_t in_port_t;
typedef u32_t in_addr_t;

int setsockopt(int s, int level, int optname, const void* opval, socklen_t optlen);
int getsockopt(int s, int level, int optname, void* opval, socklen_t* optlen);
int getpeername(int s, struct sockaddr* name, socklen_t* namelen);
int getsockname(int s, struct sockaddr* name, socklen_t* namelen);

int emul_socket(int domain, int type, int protocol);
#define socket(d, t, p) emul_socket(d, t, p)

//int socket(int domain,int type,int protocol);
//int emul_close_socket(int s);
//#define close(fildes) emul_close_socket(fildes)

int close(int s);
int connect(int s, const struct sockaddr* name, socklen_t namelen);

int emul_bind(int s, uint32_t address, int port);
#define bind(s, name, namelen) emul_bind(s, ((sockaddr_in*)name)->sin_addr.s_addr, ((sockaddr_in*)name)->sin_port)

int listen(int s, int backlog);
int accept(int s, struct sockaddr* addr, socklen_t* addrlen);

struct sockaddr_storage {
  u8_t s2_len;
  sa_family_t ss_family;
  char s2_data1[2];
  u32_t s2_data2[3];
#if LWIP_IPV6
  u32_t s2_data3[3];
#endif /* LWIP_IPV6 */
};

struct in_addr {
  in_addr_t s_addr;
};

struct sockaddr_in {
  u8_t sin_len;
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
  char sin_zero[SIN_ZERO_LEN];
};

unsigned short htons(unsigned short x);
unsigned short ntohs(unsigned short x);
//unsigned short htonl(unsigned short x);
//unsigned short ntohl(unsigned short x);

#define EPERM 1   /* Not owner */
#define ENOENT 2  /* No such file or directory */
#define ESRCH 3   /* No such process */
#define EINTR 4   /* Interrupted system call */
#define EIO 5     /* I/O error */
#define ENXIO 6   /* No such device or address */
#define E2BIG 7   /* Arg list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9   /* Bad file number */
#define ECHILD 10 /* No children */
#define EAGAIN 11 /* No more processes */
#define ENOMEM 12 /* Not enough space */
#define EACCES 13 /* Permission denied */
#define EFAULT 14 /* Bad address */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ENOTBLK 15 /* Block device required */
#endif
#define EBUSY 16   /* Device or resource busy */
#define EEXIST 17  /* File exists */
#define EXDEV 18   /* Cross-device link */
#define ENODEV 19  /* No such device */
#define ENOTDIR 20 /* Not a directory */
#define EISDIR 21  /* Is a directory */
#define EINVAL 22  /* Invalid argument */
#define ENFILE 23  /* Too many open files in system */
#define EMFILE 24  /* File descriptor value too large */
#define ENOTTY 25  /* Not a character device */
#define ETXTBSY 26 /* Text file busy */
#define EFBIG 27   /* File too large */
#define ENOSPC 28  /* No space left on device */
#define ESPIPE 29  /* Illegal seek */
#define EROFS 30   /* Read-only file system */
#define EMLINK 31  /* Too many links */
#define EPIPE 32   /* Broken pipe */
#define EDOM 33    /* Mathematics argument out of domain of function */
#define ERANGE 34  /* Result too large */
#define ENOMSG 35  /* No message of desired type */
#define EIDRM 36   /* Identifier removed */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ECHRNG 37   /* Channel number out of range */
#define EL2NSYNC 38 /* Level 2 not synchronized */
#define EL3HLT 39   /* Level 3 halted */
#define EL3RST 40   /* Level 3 reset */
#define ELNRNG 41   /* Link number out of range */
#define EUNATCH 42  /* Protocol driver not attached */
#define ENOCSI 43   /* No CSI structure available */
#define EL2HLT 44   /* Level 2 halted */
#endif
//#define	EDEADLK 45	/* Deadlock */
//#define	ENOLCK 46	/* No lock */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define EBADE 50     /* Invalid exchange */
#define EBADR 51     /* Invalid request descriptor */
#define EXFULL 52    /* Exchange full */
#define ENOANO 53    /* No anode */
#define EBADRQC 54   /* Invalid request code */
#define EBADSLT 55   /* Invalid slot */
#define EDEADLOCK 56 /* File locking deadlock error */
#define EBFONT 57    /* Bad font file fmt */
#endif
#define ENOSTR 60  /* Not a stream */
#define ENODATA 61 /* No data (for no delay io) */
#define ETIME 62   /* Stream ioctl timeout */
#define ENOSR 63   /* No stream resources */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ENONET 64  /* Machine is not on the network */
#define ENOPKG 65  /* Package not installed */
#define EREMOTE 66 /* The object is remote */
#endif
#define ENOLINK 67 /* Virtual circuit is gone */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define EADV 68   /* Advertise error */
#define ESRMNT 69 /* Srmount error */
#define ECOMM 70  /* Communication error on send */
#endif
#define EPROTO 71    /* Protocol error */
#define EMULTIHOP 74 /* Multihop attempted */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ELBIN 75   /* Inode is remote (not really error) */
#define EDOTDOT 76 /* Cross mount point (not really error) */
#endif
#define EBADMSG 77 /* Bad message */
#define EFTYPE 79  /* Inappropriate file type or format */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ENOTUNIQ 80 /* Given log. name not unique */
#define EBADFD 81   /* f.d. invalid for this operation */
#define EREMCHG 82  /* Remote address changed */
#define ELIBACC 83  /* Can't access a needed shared lib */
#define ELIBBAD 84  /* Accessing a corrupted shared lib */
#define ELIBSCN 85  /* .lib section in a.out corrupted */
#define ELIBMAX 86  /* Attempting to link in too many libs */
#define ELIBEXEC 87 /* Attempting to exec a shared library */
#endif
//#define ENOSYS 88	/* Function not implemented */
#ifdef __CYGWIN__
#define ENMFILE 89 /* No more files */
#endif
//#define ENOTEMPTY 90	/* Directory not empty */
//#define ENAMETOOLONG 91	/* File or path name too long */
#define ELOOP 92         /* Too many symbolic links */
#define EOPNOTSUPP 95    /* Operation not supported on socket */
#define EPFNOSUPPORT 96  /* Protocol family not supported */
#define ECONNRESET 104   /* Connection reset by peer */
#define ENOBUFS 105      /* No buffer space available */
#define EAFNOSUPPORT 106 /* Address family not supported by protocol family */
#define EPROTOTYPE 107   /* Protocol wrong type for socket */
#define ENOTSOCK 108     /* Socket operation on non-socket */
#define ENOPROTOOPT 109  /* Protocol not available */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ESHUTDOWN 110 /* Can't send after socket shutdown */
#endif
#define ECONNREFUSED 111    /* Connection refused */
#define EADDRINUSE 112      /* Address already in use */
#define ECONNABORTED 113    /* Software caused connection abort */
#define ENETUNREACH 114     /* Network is unreachable */
#define ENETDOWN 115        /* Network interface is not configured */
#define ETIMEDOUT 116       /* Connection timed out */
#define EHOSTDOWN 117       /* Host is down */
#define EHOSTUNREACH 118    /* Host is unreachable */
#define EINPROGRESS 119     /* Connection already in progress */
#define EALREADY 120        /* Socket already connected */
#define EDESTADDRREQ 121    /* Destination address required */
#define EMSGSIZE 122        /* Message too long */
#define EPROTONOSUPPORT 123 /* Unknown protocol */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ESOCKTNOSUPPORT 124 /* Socket type not supported */
#endif
#define EADDRNOTAVAIL 125 /* Address not available */
#define ENETRESET 126     /* Connection aborted by network */
#define EISCONN 127       /* Socket is already connected */
#define ENOTCONN 128      /* Socket is not connected */
#define ETOOMANYREFS 129
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define EPROCLIM 130
#define EUSERS 131
#endif
#define EDQUOT 132
#define ESTALE 133
#define ENOTSUP 134 /* Not supported */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ENOMEDIUM 135 /* No medium (in tape drive) */
#endif
#ifdef __CYGWIN__
#define ENOSHARE 136   /* No such host or network path */
#define ECASECLASH 137 /* Filename exists with different case */
#endif
//#define EILSEQ 138		/* Illegal byte sequence */
#define EOVERFLOW 139       /* Value too large for defined data type */
#define ECANCELED 140       /* Operation canceled */
#define ENOTRECOVERABLE 141 /* State not recoverable */
#define EOWNERDEAD 142      /* Previous owner died */
#ifdef __LINUX_ERRNO_EXTENSIONS__
#define ESTRPIPE 143 /* Streams pipe error */
#endif
#define EWOULDBLOCK EAGAIN /* Operation would block */

struct linger {
  int l_onoff;  /* option on/off */
  int l_linger; /* linger time in seconds */
};
#endif

typedef enum {
  /** No error, everything OK. */
  ERR_OK = 0,
  /** Out of memory error.     */
  ERR_MEM = -1,
  /** Buffer error.            */
  ERR_BUF = -2,
  /** Timeout.                 */
  ERR_TIMEOUT = -3,
  /** Routing problem.         */
  ERR_RTE = -4,
  /** Operation in progress    */
  ERR_INPROGRESS = -5,
  /** Illegal value.           */
  ERR_VAL = -6,
  /** Operation would block.   */
  ERR_WOULDBLOCK = -7,
  /** Address in use.          */
  ERR_USE = -8,
  /** Already connecting.      */
  ERR_ALREADY = -9,
  /** Conn already established.*/
  ERR_ISCONN = -10,
  /** Not connected.           */
  ERR_CONN = -11,
  /** Low-level netif error    */
  ERR_IF = -12,

  /** Connection aborted.      */
  ERR_ABRT = -13,
  /** Connection reset.        */
  ERR_RST = -14,
  /** Connection closed.       */
  ERR_CLSD = -15,
  /** Illegal argument.        */
  ERR_ARG = -16
} err_enum_t;

ssize_t lwip_write(int s, const void* dataptr, size_t size);
ssize_t lwip_read(int s, void* mem, size_t len);

typedef void (*dns_found_callback)(const char* name, const ip_addr_t* ipaddr, void* callback_arg);
extern "C" err_t dns_gethostbyname(const char* hostname, ip_addr_t* addr, dns_found_callback found, void* callback_arg);

#endif
