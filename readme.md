## Simple Chat on TCP socket with full-duplex communication

#### things that I marked with (?) indicate statements that I still have doubts about

### A few note

- sock_stream sock_dgram sock_raw is a part of network socket types
- use netstat to see network connection statistic, like used port
- setiap protokol seperti (IP_ICMP) mempunyai struktur data, fungsionalitas dan usecase masing-masing
- man 2 untuk syscall, man 3 untuk library, lihat lengkapnya pada command `man man`
- endian = akhiran dari bytes di memory (?)
- network always use big endian
- using raw tcp client: nc program

### New update

- handle multiple client
- broadcast from one to other client

### New note

- dynamic data on variable need to be stored as pointer and cannot stored in stack therefore use malloc
- selain untuk optimasi, alignment memory needed in order to fit some structure data, and can be used properly
- jika tidak di-align dengan benar dapat menyebabkan buffer overflow
- salah satu cara meng-align nya adalah dengan menggunakan `__attribute__((__packed__))`
- buffering itu buat menghemat [resource](https://t.me/GNUWeeb/840618) dan [system call](https://t.me/GNUWeeb/840589) (?)

auto flush occurs when: new line/lf, get input: `fgets` | `getchar` | `etc...`, exit
here's some example:
```c
printf("> ");
fflush(stdout); // force flush fix the issue if the auto flush not performed
while (1);
```

#### FD - file descriptor
file descriptor is unique identifier that hold information related to I/O operation, for example [tcp socket](https://www.google.com/search?q=sock_raw&tbm=isch&ved=2ahUKEwik993gpMuBAxW7pukKHVMsD2YQ2-cCegQIABAA&oq=sock&gs_lcp=CgNpbWcQARgAMgQIIxAnMgQIIxAnMgoIABCKBRCxAxBDMgcIABCKBRBDMggIABCABBCxAzIHCAAQigUQQzIHCAAQigUQQzIFCAAQgAQyBQgAEIAEMgUIABCABDoGCAAQBxAeOgcIABATEIAEOggIABAFEB4QEzoGCAAQHhATUPAFWL8XYOAeaANwAHgAgAFGiAHFA5IBATiYAQCgAQGqAQtnd3Mtd2l6LWltZ8ABAQ&sclient=img&ei=gWIUZeSyKrvNpgfT2LywBg&bih=993&biw=958&rlz=1C1OKWM_enID1037ID1037#imgrc=qBNgNyqHcpiROM) and some widely-known standard I/O stream and FD: stdin, stdout, and stderr and the preprocessor symbols or predefined constant in macro: `STDIN_FILENO`, `STDOUT_FILENO`, `STDERR_FILENO` respectively

Diagram that shows variant of I/O device:
![I/O Example](https://i.stack.imgur.com/mcw90.jpg)

image from: [What does "address space" means when talking about IO devices?](https://softwareengineering.stackexchange.com/questions/359297/what-does-address-space-means-when-talking-about-io-devices)

### A few terms to note:
- buffer: some data that was held somewhere in memory
- I/O stream<sup>[1](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c)</sup>: wide-terms for represents an input source or an output destination (?)
- flush: write/move buffer to specified FD
- buffering mode on stream<sup>[1](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c)</sup>: there's three types of buffering mode, unbuffered, line-buffered, and fullbuffered, each of mode have predefined constant in macro: `_IONBF`, `_IOLBF`, and `_IOFBF` respectively, see [`man setvbuf`](https://t.me/GNUWeeb/840558) for the details of the behaviour
- also `struct FILE *` is refer to stream and [doesn't literally to the actual file](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c#:~:text=does%20NOT%20point%20to%20the%20actual%20file)
- There's high-level and low-level interface<sup>[2](https://stackoverflow.com/questions/15102992/what-is-the-difference-between-stdin-and-stdin-fileno#:~:text=67-,The%20interface,-.%20Like%20everyone%20else)</sup>:
  - the use of FD is often only used at low levels, for e.g poll syscall<sup>[3](https://stackoverflow.com/questions/49476088/why-in-this-case-the-stdin-fd-is-not-ready#:~:text=Polling%20works%20at%20the%20file%20descriptor%20level)</sup>
  - some function from standard libraries like fread, fwrite, and fprintf are higher level interface<sup>[4](https://stackoverflow.com/questions/15102992/what-is-the-difference-between-stdin-and-stdin-fileno#:~:text=the%20higher%20level%20interfaces%20like%20fread%2C%20fwrite%2C%20and%20fprintf)</sup>

### Example syscall for asynchronus I/O operation (?)

- select (old but implemented on all system)
- poll posix-only (included bsd)
  - some event on a file descriptor: documented on [`man 2 poll`](https://man7.org/linux/man-pages/man2/poll.2.html)
- epoll linux | queu bsd | iocp windows
- io_uring linux