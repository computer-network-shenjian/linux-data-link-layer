#include "shared_library.hpp"


bool sig_cksum_err              =   false;
bool sig_frame_arrival          =   false;
bool sig_network_layer_ready    =   false;
bool sig_enable_network_layer   =   false;
bool sig_timeout                =   false;
bool sig_ack_timeout            =   false;


/*****************************/
/*****  Network Layer   ******/
/*****************************/
Status log_init(std::ofstream &log_stream, const std::string log_name, const Level level) {
    // log_stream must not be opened before getting into this function.
    if (log_stream.is_open()) {
        return E_LOG_OPEN;
    }
    log_stream.open(log_name, ios::out|ios::trunc);
    if (!log_stream.is_open()) {
        return E_LOG_OPEN;
    }
    Log::get().setLogStream(log_stream);
    Log::get().setLevel(level);
    return ALL_GOOD;
}

Status sender_network_layer_test(int *pipefd, const pid_t datalink_pid) {
    close(pipefd[p_read]);   // write only

    // 1. send "hello, y'all! SNL is gonna test y'all!".
    char pipe_buf1[RAW_DATA_SIZE+1] = "hello, y'all! SNL is gonna test y'all!";
    if (write(pipefd[p_write], pipe_buf1, RAW_DATA_SIZE) < 0) {
        LOG(Error) << "[SNL] Pipe write from SNL to SDL error" << endl;
        return E_PIPE_WRITE;
    }

    LOG(Debug) << "[SNL] SNL sent to SDL: " << pipe_buf1 << endl;
    
    // 2. send "bye, y'all! SNL is gonna send all zero!".
    char pipe_buf2[RAW_DATA_SIZE+1] = "bye, y'all! SNL is gonna send all zero!";
    if (write(pipefd[p_write], pipe_buf2, RAW_DATA_SIZE) < 0) {
        LOG(Error) << "[SNL] Pipe write from SNL to SDL error" << endl;
        return E_PIPE_WRITE;
    }

    LOG(Debug) << "[SNL] SNL sent to SDL: " << pipe_buf2 << endl;

    // 3. send all zero.
    if (write(pipefd[p_write], all_zero, RAW_DATA_SIZE) < 0) {
        LOG(Error) << "[SNL] Pipe write from SNL to SDL error" << endl;
        return E_PIPE_WRITE;
    }

    LOG(Debug) << "[SNL] SNL sent to SDL all zero" << endl;

    int val_waitpid = waitpid(datalink_pid, NULL, 0);
    LOG(Debug) << "[SNL] val_waitpid\t" << val_waitpid << endl;
    LOG(Info) << "[SNL] SDL end detected" << endl;
    close(pipefd[p_write]);
    
    return ALL_GOOD;
}

Status receiver_network_layer_test(int *pipefd) {
    close(pipefd[p_write]);   // read only
    char pipe_buf[RAW_DATA_SIZE+1] = {0};

    while(1) {
        if (read(pipefd[p_read], pipe_buf, RAW_DATA_SIZE) < 0) {
            LOG(Error) << "[RNL] Pipe read from RDL to RNL error" << endl;
            return E_PIPE_READ;
        }
        LOG(Debug) << "[RNL] RNL read from RDL: " << pipe_buf << endl;
        if (0 == memcmp(pipe_buf, all_zero, RAW_DATA_SIZE)) {
            LOG(Info) << "[RNL] Transmission end detected" << endl;
            break;
        }
    }

    close(pipefd[p_read]);
    return ALL_GOOD;
}


/*****************************/
/*****  Datalink Layer   *****/
/*****************************/

void Handler_SIGFRARV(int sig) {
    sig_frame_arrival = 1;
}

void wait_for_event(event_type &event) {
    while(true){
        if(sig_frame_arrival){
            sig_frame_arrival = false;
            event = frame_arrival;
            break;
        }   
        if(sig_cksum_err){
            sig_cksum_err = false;
            event = cksum_err;
            break;
        }
        if(sig_timeout){
            sig_timeout = false;
            event = timeout;
            break;
        }
        if(sig_network_layer_ready){
            sig_network_layer_ready = false;
            event = network_layer_ready;
            break;
        }
        if(sig_ack_timeout){
            sig_ack_timeout = false;
            event = ack_timeout;
            break;
        }
        else
            continue;
    }
    return;
}

Status sender_datalink_layer_test(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    LOG(Info) << "[SDL] SDL start" << endl;

    int pipe_datalink_physical[2];
    if (pipe(pipe_datalink_physical) < 0) {
        LOG(Error) << "[SDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[SDL] pipe_datalink_physical init ok" << endl;
    }

    pid_t physical_pid = fork();

    if (physical_pid < 0) {
        LOG(Error) << "[SDL] fork unsuccessful" << endl;
        return E_FORK;        
    }
    else if (physical_pid == 0) {
        Status val_spl = sender_physical_layer(pipe_datalink_physical);
        if (val_spl < 0) {
            LOG(Error) << "[SPL] Error occured in SPL with code: " << val_spl << endl;
            return val_spl;
        }
        else {
            LOG(Info) << "[SPL] SPL end with success" << endl;
            return ALL_GOOD;
        }
    }
    else {
        close(pipefd[p_write]);    // read only
    }

    char pipe_buf[RAW_DATA_SIZE+1] = {0};
    char pipe_buf_send[LEN_PKG_DATA+1] = {0};
    char datalink_head[LEN_PKG_NODATA+1] = "000100020003"; 

    close(pipe_datalink_physical[p_read]);

    while (1) {
        if (read(pipefd[p_read], pipe_buf, RAW_DATA_SIZE) <= 0) {
            LOG(Error) << "[SDL] Pipe read from SNL to SDL error" << endl;
            return E_PIPE_READ;
        }
        LOG(Debug) << "[SDL] Get info from SNL: " << pipe_buf << endl;

        memcpy(pipe_buf_send, datalink_head, LEN_PKG_NODATA);
        memcpy(pipe_buf_send+LEN_PKG_NODATA, pipe_buf, RAW_DATA_SIZE);

        if (write(pipe_datalink_physical[p_write], pipe_buf_send, LEN_PKG_DATA) < 0) {
            LOG(Error) << "[SDL] Pipe write from SDL to SPL error" << endl;
            return E_PIPE_WRITE;            
        }
        if (0 == memcmp(pipe_buf, all_zero, RAW_DATA_SIZE)) {
            break;
        }
    }

    LOG(Info) << "[SDL] Transmission end detected" << endl;

    close(pipefd[p_read]);

    int val_waitpid = waitpid(physical_pid, NULL, 0);
    LOG(Debug) << "[SDL] val_waitpid\t" << val_waitpid << endl;
    LOG(Info) << "[SDL] SPL end detected" << endl;
    close(pipefd[p_write]);

    LOG(Info) << "[SDL] SDL end with success!" << endl;
    return ALL_GOOD;
}

Status sender_datalink_layer(DProtocol protocol, int *pipefd) {
    Status val;
    switch(protocol) {
        case(test): {
            LOG(Info) << "[SDL] Getting into SDL with protocol: " << "test" << endl;
            val = sender_datalink_layer_test(pipefd);
            LOG(Debug) << "[SDL] Return value of sender_datalink_layer_test\t" << val << endl;
            return val;
        }
        case(utopia): {
            LOG(Info) << "[SDL] Getting into SDL with protocol: utopia" << endl;
            val = sender_datalink_layer_utopia(pipefd);
            LOG(Debug) << "[SDL] Return value of sender_datalink_layer_utopia\t" << val << endl;
            return val;
        }
        default: {
            LOG(Error) << "[SDL] Datalink protocol selection error" << endl;
            return E_DATALINK_SELECT;
        }
    }
}

Status receiver_datalink_layer(DProtocol protocol, int*pipefd) {
    Status val;
    switch(protocol) {
        case(test): {
            LOG(Info) << "[RDL] Getting into RDL with protocol: " << "test" << endl;
             val = receiver_datalink_layer_test(pipefd);
            LOG(Debug) << "[RDL] Return value of receiver_datalink_layer_test\t" << val << endl;
            return val;
        }
        case(utopia): {
            LOG(Info) << "[RDL] Getting into RDL with protocol: utopia" << endl;
            val = receiver_datalink_layer_utopia(pipefd);
            LOG(Debug) << "[RDL] Return value of receiver_datalink_layer_utopia\t" << val << endl;
            return val;
        }
        default: {
            LOG(Error) << "[RDL] Datalink protocol selection error" << endl;
            return E_DATALINK_SELECT;
        }
    }
}

Status receiver_datalink_layer_test(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    signal(SIGFRARV, SIG_IGN);

    LOG(Info) << "[RDL] RDL start" << endl;

    int pipe_datalink_physical[2];
    if (pipe(pipe_datalink_physical) < 0) {
        LOG(Error) << "[RDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[RDL] pipe_datalink_physical init ok" << endl;
    }

    signal(SIGCHLD, SIG_IGN);

    pid_t physical_pid = fork();

    if (physical_pid < 0) {
        LOG(Error) << "[RDL] fork unsuccessful" << endl;
        return E_FORK;        
    }
    else if (physical_pid == 0) {
        Status val_spl = receiver_physical_layer(pipe_datalink_physical);
        if (val_spl < 0) {
            LOG(Error) << "[RPL] Error occured in RPL with code: " << val_spl << endl;
            return val_spl;
        }
        else {
            LOG(Info) << "[RPL] SPL end with success" << endl;
            return ALL_GOOD;
        }
    }
    else {
        close(pipefd[p_read]);    // write only
    }

    char pipe_buf_write[RAW_DATA_SIZE+1] = {0};
    char pipe_buf_read[LEN_PKG_DATA+1] = {0};
    char datalink_head[LEN_PKG_NODATA+1] = {0}; 

    close(pipe_datalink_physical[p_write]);

    while(1) {
        if (read(pipe_datalink_physical[p_read], pipe_buf_read, LEN_PKG_DATA) <= 0) {
            LOG(Error) << "[RDL] Pipe read from RPL to RDL error" << endl;
            return E_PIPE_READ;
        }
        //LOG(Debug) << "[RDL] Get info from RPL: " << pipe_buf_read << endl;

        memcpy(datalink_head, pipe_buf_read, LEN_PKG_NODATA);
        LOG(Debug) << "[RDL] Get head from RPL: " << datalink_head << endl;

        memcpy(pipe_buf_write, pipe_buf_read+LEN_PKG_NODATA, LEN_PKG_DATA);
        LOG(Debug) << "[RDL] Get info from RPL: " << pipe_buf_write << endl;

        if (write(pipefd[p_write], pipe_buf_write, RAW_DATA_SIZE) < 0) {
            LOG(Error) << "[RDL] Pipe write from RDL to RNL error" << endl;
            return E_PIPE_WRITE;            
        }
        if (0 == memcmp(pipe_buf_write, all_zero, RAW_DATA_SIZE)) {
            break;
        }
    }

    LOG(Info) << "[RDL] Transmission end detected" << endl;

    close(pipefd[p_read]);

    LOG(Info) << "[RDL] RDL test passed!" << endl;
    return ALL_GOOD;
}

Status sender_datalink_layer_utopia(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    LOG(Info) << "[SDL] SDL start" << endl;

    Status rtn = ALL_GOOD;
    int pipe_datalink_physical[2];
    pid_t phy_pid;

    if(pipe(pipe_datalink_physical) == -1){
        LOG(Error) << "[SDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[SDL] pipe_datalink_physical init ok" << endl;
    }

    phy_pid = fork();

    if(phy_pid < 0){
        LOG(Error) << "[SDL] fork unsuccessful" << endl;
        return E_FORK;
    }

    //physical layer proc
    else if(phy_pid == 0){
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        LOG(Info) << "sender: SPL start."<< endl;

        rtn = sender_physical_layer(pipe_datalink_physical);
        if(rtn == TRANSMISSION_END){
            LOG(Info) << "sender: Transmission end in SDL." << endl;
            return rtn;
        }
        else if(rtn < 0){
            LOG(Error) << "[SPL] Error occured in SPL with code: " << rtn << endl;
            return rtn;
        }
        else{    //return ALL_GOOD
            LOG(Info) << "sender: SPL send frame successfully." << endl;
            return ALL_GOOD;
        }   
    }//end of else if

    //datalink layer proc
    else{    
        //clsoe write port
        close(pipefd[p_write]);
        frame s;
        packet buffer;

        //enable_network_layer();
        while(true){
            rtn = from_network_layer(&buffer, pipefd);
            if(rtn == E_PIPE_READ)  
                return rtn;

            s.info = buffer;
            rtn = to_physical_layer(&s, pipe_datalink_physical);
           
            if(rtn == TRANSMISSION_END)
                break;
            if(rtn < 0)
                return rtn;
            else
                continue;
        }

        LOG(Info) << "[SDL] Transmission end detected" << endl;
        close(pipefd[p_read]);

        pid_t wait_pid = waitpid(phy_pid, NULL, 0); //wait for phy_pid exit
        LOG(Debug) << "[SDL] val_waitpid\t" << wait_pid << endl;
        LOG(Info) << "[SDL] SPL end detected" << endl;
    }//end of else

    LOG(Info) << "[SDL] SDL end with success!" << endl;
    return ALL_GOOD;
}

Status receiver_datalink_layer_utopia(int *pipefd) {
    //avoid zonbe proc
    signal(SIGCHLD, SIG_IGN);
    signal(SIGFRARV, Handler_SIGFRARV);

    Status rtn = ALL_GOOD;
    int pipe_physical_datalink[2];

    if(pipe(pipe_physical_datalink) == -1){
        LOG(Error) << "receiver: Pipe INIT error in datalink layer." << endl;
        return E_PIPE_INIT;
    }

    pid_t phy_pid = fork();

    if(phy_pid < 0){
        LOG(Error) << "receiver: SDL fork error." << endl;
        return E_FORK;
    }

    //physical layer proc 
    else if(phy_pid == 0){
        //physical proc exit after datalink proc exit 
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        LOG(Info) << "receiver: SPL start."<< endl;
        while(rtn >= 0){
            rtn = receiver_physical_layer(pipe_physical_datalink);
            
            // if(rtn == TRANSMISSION_END){
            //     LOG(Info) << "receiver: Transmission end in SDL." << endl;
            //     return rtn;
            // }
            if(rtn < 0)
                LOG(Error) << "receiver: SPL failed, returned error." << endl;
            else    //return ALL_GOOD
                LOG(Info) << "receiver: SPL end successfully." << endl; 
        }
        return rtn;
    }

    //datalink layer proc
    else{
        close(pipefd[p_read]);
        frame r;
        event_type event;
        Status P_rtn, N_rtn;

        while(true){
            while(true){
                wait_for_event(event);
                if(event == frame_arrival)
                    break;
            }

            P_rtn = from_physical_layer(&r, pipe_physical_datalink);
            if(P_rtn < 0)
                return P_rtn;

            N_rtn = to_network_layer(&r.info, pipefd);
            if(N_rtn < 0)
                return N_rtn;
            //this must be done afster to_network_layer!!
            if(P_rtn == TRANSMISSION_END)
                return TRANSMISSION_END;
            else
                continue;
        }
    }
    return ALL_GOOD;
}

Status from_network_layer(packet *p, int *pipefd) {
    char pipe_buf[RAW_DATA_SIZE + 1];
    //p_write closed in upper function
    if (read(pipefd[p_read], pipe_buf, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "sender: SNL read from SDL error." << endl;
        return E_PIPE_READ;
    }
    //data starts from data[12]
    memcpy(p->data, pipe_buf, RAW_DATA_SIZE);
    LOG(Info) << "sender: SDL successfully gets info from SNL."<< endl;
    return ALL_GOOD;
}     

Status to_physical_layer(frame *s, int pipefd){
        char pipe_buf[LEN_PKG_DATA+1];
        memcpy(pipe_buf, &(s->kind), sizeof(int));
        memcpy(pipe_buf+4, &(s->seq), sizeof(int));
        memcpy(pipe_buf+8, &(s->ack), sizeof(int));
        memcpy(pipe_buf+12, s->info.data, RAW_DATA_SIZE);

        close(pipefd[p_read]);  
        if((write(pipefd[p_write], pipe_buf, LEN_PKG_DATA )) <= 0){
            LOG(Error) << "sender: SDL write to SPL error." << endl;
            return E_PIPE_WRITE;
        }
        LOG(Info) << "sender: SDL sent frame to SPL successfully." << endl;

        if (0 == memcmp(pipe_buf, all_zero, RAW_DATA_SIZE)) {
            return TRANSMISSION_END;
        else
            return ALL_GOOD;
}

Status to_network_layer(packet *p, int *pipefd) {
    //p_read closed in upper function
    if (write(pipefd[p_write], p->data, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "receiver: SNL send to SDL error." << endl;
        return E_PIPE_WRITE;
    }
    LOG(Info) << "sender: SDL successfully gets info from SNL."<< endl;
    return ALL_GOOD;
}   

Status from_physical_layer(frame *s, int *pipefd) {
    char pipe_buf[LEN_PKG_DATA+1];

    close(pipefd[p_write]); 
    if((read(pipefd[p_read], pipe_buf, LEN_PKG_DATA)) <= 0){
        LOG(Error) << "receiver: SDL read from SPL error." << endl;
        return E_PIPE_READ;
    }
    memcpy(&(s->kind), pipe_buf, sizeof(int));
    memcpy(&(s->seq), pipe_buf+4, sizeof(int));
    memcpy(&(s->ack), pipe_buf+8, sizeof(int));
    memcpy(s->info.data, pipe_buf+12, RAW_DATA_SIZE);

    LOG(Info) << "receiver: SDL received frame from SPL successfully." << endl;
    return ALL_GOOD;
}   


//not used in Utopia
void enable_network_layer(void) {
}



/*****************************/
/*****  Physical Layer   *****/
/*****************************/
int tcp_server_block(const int port) {
    // AF_INET: IPv4 protocol
    // SOCK_STREAM: TCP protocol
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        graceful_return("socket", E_CREATE_SOCKET);
    } 
    LOG(Debug) << "server socket." << endl;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        graceful_return("setsockopt", E_SETSOCKOPT);
    }

    struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET; 
    // INADDR_ANY means 0.0.0.0(localhost), or all IP of local machine.
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); 
    int server_addrlen = sizeof(server_addr);
    if (bind(server_fd, (struct sockaddr *) &server_addr, server_addrlen) < 0) { 
        graceful_return("bind", E_BIND);
    }
    LOG(Debug) << "server bind." << endl;

    if (listen(server_fd, TCP_LISTEN_NUM) < 0) { 
        graceful_return("listen", E_LISTEN); 
    }
    LOG(Debug) << "server listen." << endl;

    int new_socket = accept(server_fd, (struct sockaddr *)&server_addr, (socklen_t*) &server_addrlen);
    if (new_socket < 0) { 
        graceful_return("accept", E_ACCEPT); 
    }
    else {
        LOG(Info) << "server accept client success" << endl;
    }

    return new_socket;
}

int tcp_client_block(const char *ip, const int port) {
    // AF_INET_IPv4 protocol
    // SOCK_STREAM: TCP protocol
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { 
        graceful_return("socket", E_CREATE_SOCKET);
    }
    LOG(Debug) << "server socket." << endl;

    struct sockaddr_in server_addr; 
    memset(&server_addr, '0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); 
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        LOG(Error) << "wrong peer IP" << endl;
        graceful_return("wrong peer IP", E_WRONG_IP);
    } 

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { 
        graceful_return("connect", E_CONNECT); 
    }
    else {
        LOG(Info) << "client connect server success" << endl;
    }
    return client_fd;
}

Status physical_layer_send(const int socket, const char *buf_send, const bool is_data, const bool is_end) {
    bool is_data_confirm = is_data | is_end;    // if is_end == true, is_data_confirm must be true.
    const unsigned int buf_length = is_data_confirm ? LEN_PKG_DATA : LEN_PKG_NODATA;
    char buffer[LEN_PKG_DATA] = {0};
    if (is_end) {
        memcpy(buffer, buf_send, LEN_PKG_NODATA);
    }
    else {
        memcpy(buffer, buf_send, buf_length);
    }
    unsigned int total_send = 0;
    while (total_send < buf_length) {
        int val_send = send(socket, buffer, buf_length, 0);
        if (val_send < 0) {
            graceful_return("send", E_SEND);
        }
        else if (val_send == 0) {
            graceful_return("peer disconnected", E_PEER_DISCONNECTED);
        }
        else {
            total_send += val_send;
        }
    }
    
    if (total_send > buf_length) {
        LOG(Error) << "wrong byte sent" << endl;
        return E_WRONG_BYTE;
    }
    else {
        return ALL_GOOD;
    }
}

Status physical_layer_recv(const int socket, char *buf_recv, const bool is_data) {
    const unsigned int buf_length = is_data ? LEN_PKG_DATA : LEN_PKG_NODATA;
    char buffer[LEN_PKG_DATA] = {0};
    unsigned int total_recv = 0;
    while (total_recv < buf_length) {
        int val_recv = recv(socket, buffer, buf_length, 0);
        if (val_recv < 0) {
            graceful_return("recv", E_RECV);
        }
        else if (val_recv == 0) {
            graceful_return("peer disconnected", E_PEER_DISCONNECTED);
        }
        else {
            total_recv += val_recv;
        }
    }
    
    if (total_recv > buf_length) {
        LOG(Error) << "wrong byte sent" << endl;
        return E_WRONG_BYTE;
    }
    // TODO: better way to check 1036 consecutive bytes from buffer is all '\0'.
    memcpy(buf_recv, buffer, buf_length);
    /*
    if (strlen(buffer) == 0) {
        return TRANSMISSION_END;
    }
    */
    if (0 == memcmp(all_zero, buffer+LEN_PKG_NODATA, RAW_DATA_SIZE) && is_data) {
        return TRANSMISSION_END;
    }
    else {
        return ALL_GOOD;
    }
}

Status sender_physical_layer(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    int client_fd = tcp_client_block();
    LOG(Debug) << "[SPL] client_fd\t" << client_fd << endl;
    if (client_fd < 0) {
        LOG(Error) << "[SPL] An error occured when initializing TCP client socoket or connect with error code: " << client_fd << endl;
        return client_fd;
    }

    close(pipefd[p_write]);

    while(1) {
        char buffer[LEN_PKG_DATA] = {0};
        if (read(pipefd[p_read], buffer, LEN_PKG_DATA) <= 0) {
            LOG(Error) << "[SPL] Pipe read from SNL to SDL error" << endl;
            return E_PIPE_READ;
        }
        LOG(Info) << "[SPL] Read info from SNL: " << buffer << endl;

        Status val_physical_layer_send;

        if (0 == memcmp(all_zero, buffer+LEN_PKG_NODATA, RAW_DATA_SIZE)) {
            LOG(Info) << "[SPL] Transmission end, detected by SPL" << endl;
            val_physical_layer_send = physical_layer_send(client_fd, buffer, true, true);
            LOG(Debug) << "[SPL] val_physical_layer_send\t" << val_physical_layer_send << endl;
            if (val_physical_layer_send < 0) {
                LOG(Error) << "[SPL] An error occured, val_physical_layer_send code: " << val_physical_layer_send << endl;
                return val_physical_layer_send;
            }
            close(pipefd[p_read]);
            // THINK: do we need to sleep here? How long do we need to sleep?
            //sleep(1);
            // THINK: can client_fd be closed here?
            close(client_fd);
            return TRANSMISSION_END;
        }
        else {
            val_physical_layer_send = physical_layer_send(client_fd, buffer);
            LOG(Debug) << "[SPL] val_physical_layer_send\t" << val_physical_layer_send << endl;
            if (val_physical_layer_send < 0) {
                LOG(Error) << "[SPL] An error occured, val_physical_layer_send code: " << val_physical_layer_send << endl;
                return val_physical_layer_send;
            }
        }
    }
}

Status receiver_physical_layer(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    int server_fd = tcp_server_block();
    LOG(Debug) << "[RPL] server_fd\t" << server_fd << endl;
    if (server_fd < 0) {
        LOG(Error) << "[RPL] An error occured when initializing TCP server socoket or connect with error code: " << server_fd << endl;
        return server_fd;
    }

    close(pipefd[p_read]);
    Status val_physical_layer_recv;

    while(1) {
        char buffer[LEN_PKG_DATA] = {0};

        val_physical_layer_recv = physical_layer_recv(server_fd, buffer);
        LOG(Debug) << "[RPL] val_physical_layer_recv\t" << val_physical_layer_recv << endl;

        if (val_physical_layer_recv < 0) {
            LOG(Error) << "[RPL] An error occured with val_physical_layer_recv code: " << val_physical_layer_recv << endl;
            return val_physical_layer_recv;
        }
        else {
            LOG(Debug) << "[RPL] Get info from SPL: " << buffer << endl;
            if (write(pipefd[p_write], buffer, LEN_PKG_DATA) < 0) {
                LOG(Error) << "Pipe write from RPL to RDL error" << endl;
                return E_PIPE_WRITE;
            }

            kill(getppid(), SIGFRARV);

            if (val_physical_layer_recv == TRANSMISSION_END) {
                LOG(Info) << "[RPL] Transmission end, detected by RPL" << endl;
                close(pipefd[p_write]);
                // THINK: can server_fd be closed here?
                close(server_fd);
                return TRANSMISSION_END;
            }            
        }
    }
}
