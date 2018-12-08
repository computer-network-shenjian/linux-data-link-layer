#ifndef SHARED_LIBRARY_H
#define SHARED_LIBRARY_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>

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
#include <sys/wait.h>

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

/*****************************/
/*****  Network Layer   ******/
/*****************************/
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

unsigned int count_ending_zeros(const char * const data, unsigned int data_length = 1024);
// Intro: count the number of ending zeros of an array from position data_length.

Status SNL_test(int *pipefd, const pid_t datalink_pid);

Status RNL_test(int *pipefd);

Status sender_network_layer(int *pipefd, const pid_t datalink_pid);

Status receiver_network_layer(int *pipefd);


/*****************************/
/*****  Datalink Layer   *****/
/*****************************/
void handler_SIGFRARV(int sig);

void handler_SIGCKERR(int sig);

void wait_for_event(event_type &event);

Status from_network_layer(packet *p, int *pipefd);
//function:
//      SDL gets packet from SNL
//precondition:
//      packets are of 1024 bytes already (last bytes != '\0')
//postcondition:
        // 1.E_PIPE_READ        pipe read error when fetch data from Network Layer
        // 2.ALL_GOOD           no error

Status to_network_layer(packet *p, int *pipefd);
//function:
//      RDL send packet to RNL

Status from_physical_layer(frame *s, int *pipefd, bool is_data = true);
//function:
//      RDL gets frame from RPL

Status to_physical_layer(frame *s, int *pipefd, bool is_data = true);
//function:
//      SDL send packet to SPL
//precondition:
//      send frame is ready
//postcondition:
        // 1.E_PIPE_INIT        pipe init error
        // 2.E_FORK             error when forking SPL process
        // 3.TRANSMISSION_END   transimission end(returned by SPL)
        // 4.ALL_GOOD           no error
        // 5.other Error returns from function: sender_physical_layer

// timers
using T_time_seq_nr = typename std::pair<unsigned int, seq_nr>;

// how many ticks a frame lasts
const unsigned int tick_interval = 1; // TODO: how long is the actual ticking interval of data/ack packets?
const unsigned int INTERVAL = 500;

// activate clock ticking by pasting the code below
//
// #include <sys/time.h>        /* for setitimer */
// #include <unistd.h>        /* for pause */
// #include <signal.h>        /* for signal */
// if (signal(SIGALRM, ticking_handler) == SIG_ERR) {
//     perror("Unable to catch SIGALRM");
//     exit(1);
//   }
//   it_val.it_value.tv_sec =     INTERVAL/1000;
//   it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;    
//   it_val.it_interval = it_val.it_value;
//   if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
//     perror("error calling setitimer()");
//     exit(1);
//   }
// }

void ticking_handler(int sig);
// this handler is activated every clock tick

void _start_timer(seq_nr k);
void _stop_timer(seq_nr k);

void start_timer(seq_nr k);
// start a timer for frame k, implementing using a link list to register timers

void stop_timer(seq_nr k);

//function:
// stop timer of frame k

void start_ack_timer(void);
//function:
// start ACK timer

void stop_ack_timer(void);
//function:
// stop ACK timer

void enable_network_layer(void);
//function:
// enable network layer -> enable new network_layer_ready event

void disable_network_layer(void);
//function:
// disable network layer -> disable new network_layer_ready event

Status SDL_test(int *pipefd);

Status sender_datalink_layer(DProtocol protocol, int *pipefd);

Status SDL_utopia(int *pipefd);
//function:
//      sender datalink layer in protocol utopia
//precondition:
//      packets are of 1024 bytes already (last bytes != '\0')
//postcondition:
        // 1.E_PIPE_READ        pipe read error when fetch data from Network Layer
        // 2.E_PIPE_INIT        pipe init error
        // 3.E_FORK             error when forking SPL process
        // 4.TRANSMISSION_END   transimission end(returned by SPL)
        // 5.ALL_GOOD           no error
        // 6.other Error returns from function: sender_physical_layer
Status SDL_StopAndWait(int *pipefd);

Status RDL_StopAndWait(int *pipefd);

Status SDL_noisy_SAW(int *pipefd);

Status RDL_noisy_SAW(int *pipefd);

Status RDL_utopia(int *pipefd);

Status receiver_datalink_layer(DProtocol protocol, int *pipefd);

Status RDL_test(int *pipefd);

/*****************************/
/*****  Physical Layer   *****/
/*****************************/
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
//int tcp_client_block(const char *ip = "192.168.80.231", const int port = 20350);
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

Status tcp_send(const int socket, const char *buf_send, const bool is_data = true, const bool is_end = false);
// Intro: Send packages from Sender Physical Level to Receiver Physical Level.
// Caution: If last package is less than 1036 bytes, SDL should fill '\0' to exact 1036 bytes before send before transmit package to SPL.
// Function:
    // 1. TCP block, send 1036/12 bytes each time.
    // 2. After last package is sent, send a 1036-byte package with all '\0' to indicate the end of transmission.
// Precondition:
    // 1. Sender's TCP socket, block.
    // 2. Package to send, only first 1036/12 bytes will be processed.
    // 3. Package-is-data indicator, default true. If false, only process 12 bytes.
    // 4. End indicator, default false. If true, send a 1036-byte package with all '\0'.
// Postcondition: status number.
    // 1. Transmission ongoing: return ALL_GOOD.
    // 2. Send error: return E_SEND.
    // 3. Peer disconnected: return E_PEEROFF.
    // 4. Wrong byte sent: return E_WRONG_BYTE.

Status tcp_recv(const int socket, char *buf_recv, const bool is_data = true);
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
    // 4. Peer disconnected: return E_PEEROFF.
    // 5. Wrong byte sent: return E_WRONG_BYTE. 

Status sender_physical_layer(int *pipefd_down, int *pipefd_up);
// Intro: SPL, get data from SDL in pipe, and send it to RPL by TCP block socket.
//          pipefd_down: dataflow  datalink_layer -> physical_layer
//          pipefd_up:   dataflow  physical_layer -> datalink_layer
// Function:
    // 1. Open a TCP client socket.
    // 2. Loop receive 1036 bytes from SDL in pipe.
    // 3. Loop send 1036 bytes to RPL by TCP block socket.
// Precondition: pipe.
// Postcondition: status number.

Status receiver_physical_layer(int *pipefd_down, int *pipefd_up);
// Intro: RPL, receive data from SPL by TCP block socket, and send it to RDL in pipe.
//          pipefd_down: dataflow  datalink_layer -> physical_layer
//          pipefd_up:   dataflow  physical_layer -> datalink_layer
// Function:
    // 1. Open a TCP server socket.
    // 2. Loop receive 1036 bytes from SPL by TCP block socket.
    // 3. Loop send 1036 bytes to RDL in pipe, each with signal SIGFRARV(frame_arrival).
// Precondition: pipe.
// Postcondition: status number.

int ready_to_send(int socketfd);

int ready_to_recv(int socketfd);
#endif // SHARED_LIBRARY_H
