# This branch is intended for historical purposes only.

## Simple Chat on TCP socket with full-duplex / multiplex communication

**things that I marked with (?) indicate statements that I still have doubts about**

### Credits

**Big thanks to sir [@ammarfaizi2](https://github.com/ammarfaizi2) for any support and help :grin:**

**and other members on [GNU/Weeb](https://www.gnuweeb.org/)!**

### A few note
- So far, this program not stable and won't work properly on windows platform, known issue are:
  - [server] WSAPoll not notify or dispatch event on client disconnect - fixed on this [commit](https://github.com/RealYukiSan/chat_app/commit/8df8c5f5242b3951c208db6a4108c98437340412)
  - [server] excees padding on the reply packet from server to client - fixed on this [commit](https://github.com/RealYukiSan/chat_app/commit/fc0b01c)
  - [client] WaitForMultipleObjects never response to recv from file descriptor tcp socket
- Well known bug on linux platform:
  - [client] need to figure out how to copy buffer from stdin to stdout when '\r' printed
- [I/O Multiplexing](https://notes.shichao.io/unp/ch6)
- TIL windows cannot connect to 0.0.0.0 while linux can do it, on windows, address 0.0.0.0 only useful for listen on all interface 
- [good article](https://makefiletutorial.com/) about make and Makefile script
- use `SERVER_ADDR=0.0.0.0 make` to override default server address, [also](https://stackoverflow.com/a/26213410/22382954)
- sock_stream sock_dgram sock_raw is a part of network socket types
- setiap protokol seperti (IP_ICMP) mempunyai struktur data, fungsionalitas dan usecase masing-masing
- man 2 untuk syscall, man 3 untuk library, lihat lengkapnya pada command `man man`
- endian = bytes order, tidak digunakan untuk single byte
- network always use big endian, see [network byte ordering rfc 1700](https://en.wikipedia.org/wiki/Endianness#Networking), and [the note](https://t.me/c/1987506309/1354)

### Useful tools and technique
- raw tcp client: nc program
- strace to debug syscall
- netstat to see network connection statistic, e.g list used port
- manage process in a single terminal session:
  - monitoring process
    - use command `jobs` to list related information
  - maintain process
    - use `command &` = move process to background
    - use command `fg` = move process to foreground
    - use command `bg` = move process to background, re-run freezed program

```shell
^C interrupt, this signal can be adjusted according to the way program is programmed
^Z stop / freeze program
^D EOF
```
see `man sigaction` for more info

### New update

- handle multiple client
- broadcast from one to other client

### New note

- flexible array member or dynamic data on variable need to be stored as pointer and cannot stored in stack therefore use malloc
- selain untuk optimasi, alignment memory needed in order to fit some structure data, and can be used properly
- jika tidak di-align dengan benar dapat menyebabkan buffer overflow
- salah satu cara meng-align nya adalah dengan menggunakan `__attribute__((__packed__))`
- buffering itu buat menghemat [resource](https://t.me/GNUWeeb/840618) dan [system call](https://t.me/GNUWeeb/840589) (?)

auto flush occurs when: new line/lf, get input: `fgets` | `getchar` | `etc...`, and exit

here's some example:
```c
printf("> "); // by default line-buffered, so you need to add \n to automatic trigger flush
fflush(stdout); // or force flush fix the issue if the auto flush not performed
while (1);
```

#### FD - file descriptor
file descriptor is unique identifier that hold information related to I/O operation, for example [tcp socket](https://www.google.com/search?q=sock_raw&tbm=isch&ved=2ahUKEwik993gpMuBAxW7pukKHVMsD2YQ2-cCegQIABAA&oq=sock&gs_lcp=CgNpbWcQARgAMgQIIxAnMgQIIxAnMgoIABCKBRCxAxBDMgcIABCKBRBDMggIABCABBCxAzIHCAAQigUQQzIHCAAQigUQQzIFCAAQgAQyBQgAEIAEMgUIABCABDoGCAAQBxAeOgcIABATEIAEOggIABAFEB4QEzoGCAAQHhATUPAFWL8XYOAeaANwAHgAgAFGiAHFA5IBATiYAQCgAQGqAQtnd3Mtd2l6LWltZ8ABAQ&sclient=img&ei=gWIUZeSyKrvNpgfT2LywBg&bih=993&biw=958&rlz=1C1OKWM_enID1037ID1037#imgrc=qBNgNyqHcpiROM) and some widely-known standard I/O stream and FD: stdin, stdout, and stderr and the preprocessor symbols or predefined constant in macro: `STDIN_FILENO`, `STDOUT_FILENO`, `STDERR_FILENO` respectively

Diagram that shows variant of I/O device:

![I/O Example](https://i.stack.imgur.com/mcw90.jpg)

image from: [What does "address space" means when talking about IO devices?](https://softwareengineering.stackexchange.com/questions/359297/what-does-address-space-means-when-talking-about-io-devices)

also see the File descriptor from [wikipedia](https://en.wikipedia.org/wiki/File_descriptor) and [the Linux_System_Programming-man7 book](https://t.me/GNUWeeb/869147) page 3-4

### A few terms to note:
- buffer: some data that was held somewhere in memory
- I/O stream<sup>[1](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c)</sup>: wide-terms used for represents an input source or an output destination (?)
- flush: clear buffer and stream it to the specified FD, e.g `printf` tidak akan ditampilkan jika belum di-flush ke stdout dan masih disimpan dalam bentuk buffer
- buffering mode on stream<sup>[1](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c)</sup>: there's three types of buffering mode, unbuffered, line-buffered, and fullbuffered, each of mode have predefined constant in macro: `_IONBF`, `_IOLBF`, and `_IOFBF` respectively, see [`man setvbuf`](https://t.me/GNUWeeb/840558) for the details of the behaviour
- also `struct FILE *` is refer to stream and [doesn't literally refer to the actual file](https://stackoverflow.com/questions/38652953/what-does-stream-mean-in-c#:~:text=does%20NOT%20point%20to%20the%20actual%20file)
- There's high-level and low-level interface<sup>[2](https://stackoverflow.com/questions/15102992/what-is-the-difference-between-stdin-and-stdin-fileno#:~:text=67-,The%20interface,-.%20Like%20everyone%20else)</sup>:
  - the use of FD is often only used at low levels, for e.g poll syscall<sup>[3](https://stackoverflow.com/questions/49476088/why-in-this-case-the-stdin-fd-is-not-ready#:~:text=Polling%20works%20at%20the%20file%20descriptor%20level)</sup>
  - some function from standard libraries like fread, fwrite, and fprintf are higher level interface<sup>[4](https://stackoverflow.com/questions/15102992/what-is-the-difference-between-stdin-and-stdin-fileno#:~:text=the%20higher%20level%20interfaces%20like%20fread%2C%20fwrite%2C%20and%20fprintf)</sup>

### Example syscall for asynchronus I/O operation (?)

- select (old but implemented on all system)
- poll posix-only (included bsd)
  - some event on a file descriptor: documented on [`man 2 poll`](https://man7.org/linux/man-pages/man2/poll.2.html)
- epoll linux | queu bsd | iocp windows
- io_uring linux

### writeup: my internal dialogue with myself
note: I want someone who reads this to validate and verify my statement, so feel free to open a pull request if you discover any misconcept, misinterpretation, or misleading information

- usually in the network domain, we often use dynamic allocated variable to hold flexible array, cuz it will be very wasteful if the variable are fixed-size since it will force the size to be fixed even if the data is small, and fill the rest with null char as a padding (?)
- on the network, you are passing the actual value or data, not the reference of memory address XD
- why always passing the size as additional information in the member struct? I think it's like a trade-off.
  - maybe the analogy is more or less like, Why bother to execute a bunch of instructions instead of passing additional information that is not so large (does this mean for the sake of efficiency?)
- sizeof means the size of a defined data structure on a variable, not the size of the variable's value itself. Why? You might ask, That's how data structure and memory work (?)
  - and not to confuse with strlen, which only applied to strings in order to count the amount of char.

### Part 2

- How does the program interpret the network data? especially in terms of memory. I curious how to debug this process with gdb but didn't have any clue to get started :(
  - this curiousity comes from [the buffer overflow vuln on the server](https://gist.githubusercontent.com/alviroiskandar/c95b95b2a0d7c13913f0ce5c12215340/raw/d52f14f7bbf8e7f1b5a4429cabb224860cb25947/server.c.txt#:~:text=disini%20ada%20bug%20buffer%20overflow)
  - I think the highlighted thing in this context would be:
    - the way processors read/fetch data, how operator and operand are used at the machine-code (assembly) level
    - how could packed and aligned memory [affect the program behavior](https://developer.ibm.com/articles/pa-dalign/#:~:text=the%20following%20scenarios%2C%20in%20increasing%20order%20of%20severity%2C%20are%20all%20possible%3A)?
