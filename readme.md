## Simple Chat on TCP socket with half-duplex communication

### A few note

- sock_stream sock_dgram sock_raw is a part of socket types
- use netstat to see network connection statistic, like used port
- setiap protokol seperti (IP_ICMP) mempunyai struktur data, fungsionalitas dan usecase masing-masing
- man 2 untuk syscall, man 3 untuk library, lihat lengkapnya pada command `man man`

endian = akhiran dari bytes di memory?

network always use big endian

using raw tcp client: nc program

### New update

- handle multiple client
- broadcast from one to other client

### New note

- dynamic data on variable need to be stored as pointer and cannot stored in stack therefore use malloc
- selain untuk optimasi, alignment memory needed in order to fit some structure data, and can be used properly
- jika tidak di-align dengan benar dapat menyebabkan buffer overflow