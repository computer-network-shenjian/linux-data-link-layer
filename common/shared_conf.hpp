// Configurations for shared libraries.

//#define 	SIGCKERR	SIGRTMIN+1  //cksum_err
#define 	SIGFRARV	SIGRTMIN+2	//frame_arrival
#define 	SIGNLREADY	SIGRTMIN+3	//network_layer_ready
#define 	SIGENANL	SIGRTMIN+4	//enable_network_layer
#define 	SIGDISNL	SIGRTMIN+5	//disble_network_layer

#define  MAX_SEQ     7
#define inc(k) if(k<MAX_SEQ) k++; else k=0;

typedef unsigned int seq_nr;    //send seq


const unsigned int	LEN_PKG_DATA 	= 1036;
const unsigned int	LEN_PKG_NODATA 	= 12;

const unsigned int 	TCP_LISTEN_NUM 	= 10;

const unsigned int 	RAW_DATA_SIZE 	= 1024;

const char all_zero[LEN_PKG_DATA+1] = {0};

// Datalink layer protocols
enum DProtocol{
    test = 0,
    utopia = 1,
    simple_stop_and_wait = 2,
    noisy_stop_and_wait = 3,
    one_bit_sliding = 4,
    back_n = 5,
    selective_repeat = 6
};

// Pipe read and pipe write.
enum Pipe_RW{
    p_read = 0,
    p_write = 1
};

typedef enum {
    frame_arrival,
    cksum_err,
    timeout,
    network_layer_ready,
    ack_timeout
}event_type;

typedef enum {
    data,
    ack,
    nak
}frame_kind;  //frame types: data/ack/nak

struct packet{
    unsigned char data[RAW_DATA_SIZE] = {0};
};

struct frame{
    frame_kind kind;
    seq_nr     seq;
    seq_nr     ack;
    packet     info;
};

extern int sig_cksum_err;
extern int sig_frame_arrival;
extern int sig_network_layer_ready;
extern int sig_enable_network_layer;
extern int sig_timeout;
extern int sig_ack_timeout;

#define SEND_FILE "rand_1.myfile"
#define RECV_FILE "recv_1.myfile"
