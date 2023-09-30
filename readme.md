## Simple Chat on TCP socket with half-duplex communication

### A few note

- sock_stream sock_dgram sock_raw is a part of socket types
- use netstat to see network connection statistic, like used port
- setiap protokol seperti (IP_ICMP) mempunyai struktur data, fungsionalitas dan usecase masing-masing
- man 2 untuk syscall, man 3 untuk library, lihat lengkapnya pada command `man man`

endian = akhiran dari bytes di memory?

network always use big endian

using raw tcp client: nc program
