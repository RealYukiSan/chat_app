<details>
  <summary>version 1.0 (a little bit messy)</summary>

on server side:

- declare and initialize bind_addr
- declare and initialize bind_port
<details>
  <summary> func main: </summary>

  - declare server tcp file descriptor
  - call create socket and initialize tcp fd with return value from this func
  - checking error after calling create socket
  - start event loop and pass the tcp fd as param
  - close tcp fd and exit program
</details>

<details>
  <summary> func create socket that return tcp fd: </summary>

  - declare the server socket address with struct sockaddr_in
  - declare the ret variable for error handling
  - declare tcp file descriptor to handle socket
  - intialize the tcp fd with return value from calling socket syscall
    - passing address family IPv4 (AF_INET) constant as first param
    - passing socket type tcp (SOCK_STREAM) constant as second param
    - passing internet protocol type (IPPROTO_TCP) constant as third param
  - checking error after create socket and return -1 if error
  - initialize `socketaddr_in` with memset
  - assign AF_INET (address family for IPv4) constant to sin_family property from `socketaddr_in`
  - convert bind address string to binary - with help of func inet_pton from library
    - assign `socketaddr_in.sin_addr` by passing the `socketaddr_in.sin_addr` as third parameter to inet_pton
  - check for error from return value of inet_pton
  - if error, close the tcp fd and return -1
  - convert bind port's byte order to big endian with func htons and assign `socketaddr_in.sin_port` from return value of htons
  - binding with bind syscall
    - assign ret var from return value of bind
    - casting the struct `socketaddr_in` to generic struct `socketaddr`
  - check for error, if error close the tcp fd and return -1
  - listen to incoming request by calling listen syscall and assign ret var from return value of listen
  - check for error, if error close the tcp fd and return -1
  - return tcp fd
</details>

<details>
  <summary> func start event loop </summary>

  - declare ret var
  - loop with while
    - initialize ret var with return value from run_event_loop func
      - pass tcp fd as parameter of run_event_loop
    - check for an error or exit signal, and break if the conditions are met
</details>

<details>
  <summary> func run event loop </summary>

  - declare the client socket address with struct sockaddr_in
  - declare the address length with socklen_t
  - declare the client tcp fd
  - optional section
    - declare the client address in the form of string
    - declare client port
  - initialize address length with `sizeof(socklen_t)`
  - waiting for connection from client by accept func from syscall

    - casting the struct `socketaddr_in` to generic struct `socketaddr`
    - passing tcp fd as first param
    - passing generic sockaddr as second param
    - passing address length as third param

  - initialize client fd by return value from accept
  - check error by client fd and return -1 if error
  - optional section

    - use inet_ntop and ntohs to make the data readable on your machine/host
    - and printf the result of inet_ntop and ntohs

  - call receive data func
    - pass the client fd as param
  - close the client fd
  - return 0 if there's no error
</details>

<details>
  <summary> func receive data </summary>

  - declare pointer data with simple custom protocol from struct data
  - declare ret var for checking purposes
  - allocate pointer data with malloc
  - checking malloc error
  - do a looping with while
    - waiting message from client with recv syscall
    - initialize ret var with return value from recv
    - check error, break if error
    - check if client disconnected, break if disconnected
    - call interpret message (to interpret client message)
      - pass pointer data
    - call get input and send
      - pass client fd and pointer data
      - check for return value
        - if true then break the loop
  - free the data memory
</details>

on both-side:

- declare and initialize struct `data` as super simple protocol
<details>
  <summary> func interpret message </summary>
  
  - assign len with ordered byte from htons
  - add null char from msg to terminate string
  - then you be able to print the client msg
</details>

<details>
  <summary> func get input and send </summary>
  
  - declare len var to store msg length
  - call fgets to enter the message from stdin
    - pass msg as first param
    - and the size of msg as second param
    - and the source (stdin)
    - check the return value
      - if false (indicate EOF) return -1
  - count length msg with strlen and assign it to len var
  - check the last index of msg with \n char
    - assign last index of msg with null to terminate string
    - decrement len var
  - check if the msg is empty (len var eq 0)
    - return 0 (don't send data to dest)
  - if msg exit return -1
  - reorder byte of len with htons
  - call send data to dest
    - passing dest fd
    - passing pointer data
  - return 0 to continue iterate the loop
</details>

<details>
  <summary> func send data to dest </summary>
  
  - declare ret for checking
  - call send syscall
    - initialize ret with return value
    - pass the proper param
  - check if ret var indicate error
    - return -1 if error
  - return 0 if not
</details>

on client:

- declare and initialize server_addr
- declare and initialize server_port
<details>
  <summary> func main </summary>
  
  - kurang lebih sama kek server
</details>

<details>
  <summary> func create_socket_and_connect </summary>
  
  - kurang lebih sama kek server
  - but instead of create, bind, and listen. just create and connect
</details>

<details>
  <summary> func start_chat </summary>
  
  - event loop happens here
</details>

additional function from lib:

- inet_pton presentation to network | string to binary
- inet_ntop binary to string
- htons manipulate endianness

</details>

<details>
  <summary>version 2.0 (better format)</summary>

  ## This software consists of client and server, whereas the server act as (...) and client act as (...)

  ### how it works

  <details>
    <summary>general</summary>

  - define simple protocol to construct the packet.

  </details>

  <details>
    <summary>on the client side</summary>

  - setup client
    - create FD with socket
    - connect to the server
  - initialize needed data
  - start event loop
    - get input from user and store it
      - construct the packet to be sent 
      - send the packet to the server
    - receive message from other client through server
      - decode/transform/extact/whatever the packet
      - read it

  </details>

  <details>
    <summary>on the server side</summary>

  - setup server
    - create FD with socket
    - bind the port
    - start listen the incoming request
  - initialize needed data
  - start event loop
    - accept connection from the first request on queue
    - receive the payload data (packet message)
      - decode/transform/extact/whatever the packet
      - read it and store to history chat
      - send the packet to other clients (broadcast)
  </details>
  <hr/>
</details>

<details>
  <summary>version 3.0 (detail possible vuln and how to protect against it and error handling)</summary>

  - funfact: it's a little bit unique that both server and client dispatch the disconnect event on recv syscall, so we need to handle it to prevent infinite loop
  - somehow, if the client sends an invalid packet multiple times, it will cause a recv error (bad address). Therefore, immediately force the server to shutdown, so you need to close the sender connection to protect against it
  - there's a situation where short recv (unintentional, here's the [simulated](https://github.com/Reyuki-san/chat_app/commit/cedb2d431bbee662339807177f7687cedaa1ab8c) one) and sending multiple packets at once (intentional, here's the [simulated](https://github.com/Reyuki-san/chat_app/commit/15f07aefac60a4f2f95de73d92fe245ce2db0170) one) occur. In that case, we need to carefully keep track of packet that have been read during the recv call. Otherwise it will crash the server
  - add bound checking verification for message len member struct, since it can be fooled with arbitary value from the client's side, see [this message](https://t.me/GNUWeeb/854582) for more detail

  Thanks viro-senpai for [pointing out](https://gist.githubusercontent.com/alviroiskandar/c95b95b2a0d7c13913f0ce5c12215340/raw/d52f14f7bbf8e7f1b5a4429cabb224860cb25947/server.c.txt) points 2 and 3
</details>

<details>
  <summary>version 3.1 (optimization and best practice)</summary>
  
  ### key takeaways
  - recv will batch the sendto from the client if the size of more than one sendto fits the size of the parameter in recvfrom
    - but still wondering, How exactly did this happen? I mean, I think I misunderstood the mechanism since batching does not always work like that
  - memory management
    - memory layouting
    - choose the right data structure
</details>

<details>
  <summary>version 3.2 (additional explanation)</summary>

### Here is an example of how the package is expected

Commit SHA: [`9441e97`](https://github.com/Reyuki-san/chat_app/blob/9441e97b14d010224d1b735540f614a0b1b6c049)

```
MSG CONTENT = hey\0

HDR     + UINT 16 BITS  + MSG LEN       + RAW BYTE      = TOTAL EXPECTED LEN
4       + 2             + 4             + 8192          = 8202 BYTES
```

*Baru sadar `MSG LEN` tu dari mana ya? ohh ternyata LEN-nya dari MSG CONTENT, kayaknya keliru sih di commit SHA itu :v

Usually, you'll just use the occupied size instead of the raw byte. But for this cases, I just want try to proof [the overflow of memcpy](https://t.me/GNUWeeb/854582) before adding [boundary verification](https://t.me/GNUWeeb/854601).
But instead, I just got `recv: connection reset by peer` on the client-side T_T

I will assume the recv error on the client-side caused by Client disconnected log from the server-side
```
New client connected from 127.0.0.1:35604
4 8202 # first recv
8196 8202 # second recv
Client disconnected
```

But what caused the recv error to be triggered is still mysterious. maybe recv_len < expected len cause this error? see [the comment section on this commit](https://github.com/Reyuki-san/chat_app/blob/9441e97b14d010224d1b735540f614a0b1b6c049/client.c#L77)

### Visualization overlaps on memmove

```
+--------------------------------------+
|                Source                |
+---+---+---+---+----+---+----+--------+
| 0 | - | 4 | - | 6  | - | 28 |        |
+---+---+---+---+----+---+----+        |
|     4     |    2   |   22   |  EMPTY |
+-----------+--------+--------+        |
|    HDR    |       BODY      |        |
+-----------+-----------------+--------+
|                                      |
+--------------------------------------+
|              Destination             |
+---+---+---+---+----+---+----+---+----+
| 0 | - | 4 | - | 27 | - | 29 | - | 51 |
+---+---+---+---+----+---+----+---+----+
|     4     |   23   |   2    |   22   |
+-----------+--------+--------+--------+
|    HDR    |           BODY           |
+-----------+--------------------------+
|                                      |
+--------------------------------------+
| Destination: msg_id picked from union|
+--------------------------------------+
| Source: member msg picked from union |
+--------------------------------------+
| BUFFER MSG CONTENT:                  |
+--------------------------------------+
| long message lorem is\0              |
+---+---+---+---+----+---+----+---+----+
```

you can see `27 - 29` in `msg_id` is overlap with `6 - 28` in `msg` union member of `struct packet`
What doesn't make any sense to me is, if the memmove cause overlaps, why is the rest of the unhandled buffer still stored after null char? inside `cs->pkt.msg.data` at `broadcast_msg` function

</details>