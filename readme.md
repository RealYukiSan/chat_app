## Simple Chat on TCP socket with full-duplex communication

#### things that I marked with (?) indicate statements that I still have doubts about

### A few note

- sock_stream sock_dgram sock_raw is a part of socket types
- use netstat to see network connection statistic, like used port
- setiap protokol seperti (IP_ICMP) mempunyai struktur data, fungsionalitas dan usecase masing-masing
- man 2 untuk syscall, man 3 untuk library, lihat lengkapnya pada command `man man`

endian = akhiran dari bytes di memory (?)

network always use big endian

using raw tcp client: nc program

### New update

- handle multiple client
- broadcast from one to other client

### New note

- dynamic data on variable need to be stored as pointer and cannot stored in stack therefore use malloc
- selain untuk optimasi, alignment memory needed in order to fit some structure data, and can be used properly
- jika tidak di-align dengan benar dapat menyebabkan buffer overflow
- buffering itu buat menghemat [resource](https://t.me/GNUWeeb/840618) dan [system call](https://t.me/GNUWeeb/840589) (?)

#### FD - file descriptor
file descriptor is unique identifier that hold information related to I/O operation, for example [tcp socket](https://www.google.com/search?q=sock_raw&tbm=isch&ved=2ahUKEwik993gpMuBAxW7pukKHVMsD2YQ2-cCegQIABAA&oq=sock&gs_lcp=CgNpbWcQARgAMgQIIxAnMgQIIxAnMgoIABCKBRCxAxBDMgcIABCKBRBDMggIABCABBCxAzIHCAAQigUQQzIHCAAQigUQQzIFCAAQgAQyBQgAEIAEMgUIABCABDoGCAAQBxAeOgcIABATEIAEOggIABAFEB4QEzoGCAAQHhATUPAFWL8XYOAeaANwAHgAgAFGiAHFA5IBATiYAQCgAQGqAQtnd3Mtd2l6LWltZ8ABAQ&sclient=img&ei=gWIUZeSyKrvNpgfT2LywBg&bih=993&biw=958&rlz=1C1OKWM_enID1037ID1037#imgrc=qBNgNyqHcpiROM) and some widely-known standard FD: stdin, stdout, and stderr and The preprocessor symbols or predefined constant in macro: `STDIN_FILENO`, `STDOUT_FILENO`, `STDERR_FILENO` respectively

Diagram that shows variant I/O Operation:
![I/O Example](https://i.stack.imgur.com/mcw90.jpg)

image from: [What does "address space" means when talking about IO devices?](https://softwareengineering.stackexchange.com/questions/359297/what-does-address-space-means-when-talking-about-io-devices)

### A few terms to note:
- buffer: some data that was held somewhere in memory
- I/O Stream: wide-terms for represents an input source or an output destination (?)
- flush: write/move buffer to specified FD
- buffering mode: there's three types of buffering mode, unbuffered, line-buffered, and fullbuffered, each of mode have predefined constant in macro: `_IONBF`, `_IOLBF`, and `_IOFBF` respectively, see [`man setvbuf`](https://t.me/GNUWeeb/840558) for the details of the behaviour

### Example syscall for I/O (?)

- select (implemented on all system)
- poll posix-only (included bsd)
  - some event on a file descriptor: documented on [`man 2 poll`](https://man7.org/linux/man-pages/man2/poll.2.html)
- epoll linux | queu bsd | iocp windows
- io_uring linux