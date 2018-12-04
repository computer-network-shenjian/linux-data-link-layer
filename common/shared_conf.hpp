// Configurations for shared libraries.


const unsigned int	LEN_PKG_DATA 	= 1036;
const unsigned int	LEN_PKG_NODATA 	= 12;

const unsigned int 	TCP_LISTEN_NUM 	= 10;

const unsigned int 	RAW_DATA_SIZE 	= 1024;

const char all_zero[LEN_PKG_DATA] = {0};

// Datalink layer protocols
enum DProtocol{
    test = 0,
    utopia = 1,
    simplex_stop_and_wait = 2,
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

