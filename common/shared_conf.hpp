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

 #define SEND_FILE "rand_1024.myfile"
// #define SEND_FILE "rand_1.myfile"
// #define SEND_FILE "README.md"
#define RECV_FILE "recv_1.myfile"
