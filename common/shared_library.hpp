#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h> 
#include <sys/prctl.h>
#include <sys/socket.h> 

#include "status.hpp"
#include "shared_conf.hpp"
#include "Log.h"

using namespace std;
using namespace fly;

// gracefully perror and exit
inline void graceful(const char *s, int x) { perror(s); exit(x); }

// gracefully perror and return
#define graceful_return(s, x) {\
    perror((s));\
    LOG((Error)) << s << endl;\
    return((x)); }

int tcp_server_block(const int port = 20350);
// Intro: Initialization of a block TCP server.
// Function: Initialize a TCP block server socket, and accept peer connection.
// Precondition: port, default 20350.
// Postcondition:
    // 1. Success: return number of socket after success.
    // 2. Create socket error: return E_CREATE_SOCKET.
    // 3. Setsockopt error: return E_SETSOCKOPT.
    // 4. Bind error: return E_BIND.
    // 5. Listen error: return E_LISTEN.
    // 6. Accept error: return E_ACCEPT.

int tcp_client_block(const char *ip = "0.0.0.0", const int port = 20350);
// Intro: Initialization of a block TCP client.
// Function: Initialize a TCP block client socket, and accept peer connection.
// Precondition:
    // 1. ip, default 0.0.0.0.
    // 2. port, default 20350.
// Postcondition:
    // 1. Success: return number of socket after success.
    // 2. Create socket error: return E_CREATE_SOCKET.
    // 3. Peer IP wrong: return E_WRONG_IP.
    // 4. Connect error: return E_CONNECT.

Status physical_layer_send(const int socket, const char *buf_send, const bool is_data = true, const bool is_end = false);
// Intro: Send packages from Sender Physical Level to Receiver Physical Level.
// Caution: If last package is less than 1036 bytes, SDL should fill '\0' to exact 1036 bytes before send before transmit package to SPL.
// Function:
    // 1. TCP block, send 1036/12 bytes each time.
    // 2. After last package is sent, send a 1036-byte package with all '\0' to indicate the end of transmission.
// Precondition:
    // 1. Sender's TCP socket, block.
    // 2. Package to send, only first 1036/12 bytes will be processed.
    // 3. Package-is-data indicator, default true. If false, only process 12 bytes.
    // 4. End indicator, default false. If true, send a 1036-byte package with all '\0', and you can set const char *buf_send as NULL.
// Postcondition: status number.
    // 1. Transmission ongoing: return ALL_GOOD.
    // 2. Send error: return E_SEND.
    // 3. Peer disconnected: return E_PEER_DISCONNECTED.
    // 4. Wrong byte sent: return E_WRONG_BYTE.

Status physical_layer_recv(const int socket, char *buf_recv, const bool is_data = true);
// Intro: Receive packages to Receiver Physical Level from Sender Physical Level.
// Caution: If last package is not all empty, but with '\0' at the end, RDL should discard all '\0' at the end.
// Function:
    // 1. TCP block, receive 1036/12 bytes each time.
    // 2. The end of transmission occurs if a 1036-byte package with all '\0' is received.
// Precondition:
    // 1. Receiver's TCP socket, block.
    // 2. Buffer pointer, will memcpy 1036/12 received bytes to this pointer.
    // 3. Package-is-data indicator, default true. If false, only process 12 bytes.
// Postcondition: status number.
    // 1. Transmission ongoing: return ALL_GOOD.
    // 2. Transmission end: return TRANSMISSION_END.
    // 3. Recv error: return E_RECV.
    // 4. Peer disconnected: return E_PEER_DISCONNECTED.
    // 5. Wrong byte sent: return E_WRONG_BYTE. 

Status sender_datalink_layer(DProtocol protocol, int *pipe);

Status sender_datalink_layer_test(int *pipe);

Status sender_datalink_layer_utopia(int *pipe);

Status sender_physical_layer(int *pipe);

Status receiver_datalink_layer(DProtocol protocol);

Status receiver_datalink_layer_utopia(int *pipe);

Status receiver_physical_layer(int *pipe);

Status log_init(std::ofstream &log_stream, const std::string log_name, const Level level = Debug);
// Intro: Initialize logger with given log name and log level.
// Caution: when return, it's good to close the stream.
// Function: Initialize logger with given log name and log level, and set log_stream.
// Precondition:
    // 1. Log stream.
    // 2. Log name.
    // 3. Log level, default the lowest 'Debug'. If higher level is set, lower level information will not be output.
// Postcondition:
    // 1. All good: return ALL_GOOD, and set the log stream.
    // 2. Input ofstream log_stream has been opened before getting into this function: E_LOG_ISOPEN.
    // 3. Open log error: E_LOG_OPEN.