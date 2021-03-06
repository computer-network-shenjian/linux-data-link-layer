// status and status code definitions.
typedef int Status;

// status code definitions:

// status code not less than 0 means there is no error occured.
const int ALL_GOOD = 0;     // generic no error indicator.
const int TRANSMISSION_END = 1;

// status code less than 0 means there is an error occured.
const int E_CREATE_SOCKET = -1;
const int E_SETSOCKOPT = -2;
const int E_BIND = -3;
const int E_LISTEN = -4;
const int E_ACCEPT = -5;
const int E_WRONG_IP = -6;
const int E_CONNECT = -7;
const int E_SEND = -8;
const int E_PEEROFF = -9;
const int E_WRONG_BYTE = -10;
const int E_RECV = -11;
const int E_LOG_ISOPENED = -12;
const int E_LOG_OPEN = -13;
const int E_PIPE_INIT = -14;
const int E_FORK = -15;
const int E_DATALINK_SELECT = -16;
const int E_PIPE_READ = -17;
const int E_PIPE_WRITE = -18;
const int E_OPEN_FILE = -19;
const int E_SELECT = -20;
const int E_TIMEOUT = -21;
const int E_NOSEND = E_SEND;
const int E_NORECV = E_RECV;
const int E_SETTIMER = -22;
