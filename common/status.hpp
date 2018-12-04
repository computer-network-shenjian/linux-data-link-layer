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
const int E_PEER_DISCONNECTED = -9;
const int E_WRONG_BYTE = -10;
const int E_RECV = -11;


const int E_LOG_ISOPENED = -20;
const int E_LOG_OPEN = -21;
const int E_PIPE_INIT = -22;
const int E_FORK = -23;
const int E_DATALINK_SELECT = -24;
const int E_PIPE_READ = -25;
const int E_PIPE_WRITE = -26;