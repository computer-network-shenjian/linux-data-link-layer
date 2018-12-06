#include "shared_library.hpp"


int sig_cksum_err              =   false;
int sig_frame_arrival          =   false;
int sig_network_layer_ready    =   false;
int sig_enable_network_layer   =   false;
int sig_timeout                =   false;
int sig_ack_timeout            =   false;


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

unsigned int count_ending_zeros(const char * const data, unsigned int data_length) {
    // count the number of ending zeros of an array from position data_length
    int counter = data_length;
    while (counter >= 0 && data[--counter] == 0) {};
    return data_length - 1 - counter;
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

Status sender_network_layer(int *pipefd, const pid_t datalink_pid) {
    close(pipefd[p_read]);   // write only

    //ifstream in("test.myfile", ios::binary);
    ifstream in(SEND_FILE, ios::binary);
    if(!in.is_open()) {
        LOG(Error) << "Open input file error" << endl;
        return E_OPEN_FILE;
    }
    /*
    in.seekg(0, in.end);
    streampos in_length = in.tellg();
    in.seekg(0, in.beg);
    LOG(Info) << "Read file OK with file length: " << in_length << endl;
    */
    while (!in.eof()) {
        char pipe_buf[RAW_DATA_SIZE+1] = {0};
        in.read(pipe_buf, RAW_DATA_SIZE);
        if (write(pipefd[p_write], pipe_buf, RAW_DATA_SIZE) < 0) {
            LOG(Error) << "[SNL] Pipe write from SNL to SDL error" << endl;
            return E_PIPE_WRITE;
        }        
    }

    // eof detected, close file and send all zero.
    in.close();
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

Status receiver_network_layer(int *pipefd) {
    close(pipefd[p_write]);   // read only

    //ofstream out("out.myfile", ios::binary|ios::trunc);
    ofstream out(RECV_FILE, ios::binary|ios::trunc);
    if(!out.is_open()) {
        LOG(Error) << "Open output file error" << endl;
        return E_OPEN_FILE;
    }
    char pipe_buf_before[RAW_DATA_SIZE+1] = {0};
    char pipe_buf_now[RAW_DATA_SIZE+1] = {0};
    bool is_first = true;

    while(1) {
        if (read(pipefd[p_read], pipe_buf_now, RAW_DATA_SIZE) < 0) {
            LOG(Error) << "[RNL] Pipe read from RDL to RNL error" << endl;
            return E_PIPE_READ;
        }
        LOG(Debug) << "[RNL] RNL read from RDL: " << pipe_buf_now << endl;

        if (0 == memcmp(pipe_buf_now, all_zero, RAW_DATA_SIZE)) {
            unsigned int val_zeros = count_ending_zeros(pipe_buf_before);
            out.write(pipe_buf_before, RAW_DATA_SIZE-val_zeros);
            LOG(Info) << "[RNL] Transmission end detected" << endl;
            out.close();
            break;
        }
        else {
            if (!is_first) {
                out.write(pipe_buf_before, RAW_DATA_SIZE);
            }
            is_first = false;
            memcpy(pipe_buf_before, pipe_buf_now, RAW_DATA_SIZE);
        }
    }

    close(pipefd[p_read]);
    return ALL_GOOD;
}

/*****************************/
/*****  Datalink Layer   *****/
/*****************************/

void Handler_SIGFRARV(int sig) {
    if(sig == SIGFRARV)
        sig_frame_arrival ++;
}

void wait_for_event(event_type &event) {
    while(true){
        if(sig_frame_arrival){
            sig_frame_arrival --;
            event = frame_arrival;
            break;
        }   
        if(sig_cksum_err){
            sig_cksum_err --;
            event = cksum_err;
            break;
        }
        if(sig_timeout){
            sig_timeout --;
            event = timeout;
            break;
        }
        if(sig_network_layer_ready){
            sig_network_layer_ready --;
            event = network_layer_ready;
            break;
        }
        if(sig_ack_timeout){
            sig_ack_timeout --;
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

    LOG(Info) << "[RDL] Transmission end detected, wait for RNL's death" << endl;
    close(pipefd[p_read]);

    LOG(Info) << "[RDL] RDL test passed!" << endl;

    while(1){
        sleep(1);
    }
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
        rtn = sender_physical_layer(pipe_datalink_physical, NULL);
        if(rtn == TRANSMISSION_END){
            LOG(Info) << "[SPL] Transmission end." << endl;
            return rtn;
        }
        else if(rtn < 0){
            LOG(Error) << "[SPL] Error occured in SPL with code: " << rtn << endl;
            return rtn;
        }
        else{    //return ALL_GOOD
            LOG(Info) << "[SPL] SPL end with success" << endl;
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
            if(rtn < 0)
                return rtn;

            if (0 == memcmp(s.info.data, all_zero, RAW_DATA_SIZE)) {
                break;
            }
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
    //exit when father proc exit
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    //avoid zonbe proc
    signal(SIGCHLD, SIG_IGN);
    signal(SIGFRARV, Handler_SIGFRARV);

    LOG(Info) << "[RDL] RDL start" << endl;

    Status rtn = ALL_GOOD;
    int pipe_physical_datalink[2];

    if(pipe(pipe_physical_datalink) == -1){
        LOG(Error) << "[RDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    else {
        LOG(Info) << "[RDL] pipe_datalink_physical init ok" << endl;
    }

    pid_t phy_pid = fork();

    if(phy_pid < 0){
        LOG(Error) << "[RDL] fork unsuccessful" << endl;
        return E_FORK;
    }

    //physical layer proc 
    else if(phy_pid == 0){
        rtn = receiver_physical_layer(NULL, pipe_physical_datalink);
        // if(rtn == TRANSMISSION_END){
        //     LOG(Info) << "receiver: Transmission end in SDL." << endl;
        //     return rtn;
        // }
        if(rtn < 0){
            LOG(Error) << "[RPL] Error occured in RPL with code: " << rtn << endl;
            return rtn;
        }
        else {
            LOG(Info) << "[RPL] SPL end with success" << endl;
            return ALL_GOOD;
        } 
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
            // if(P_rtn == TRANSMISSION_END)
            //     return TRANSMISSION_END;

            if (0 == memcmp(r.info.data, all_zero, RAW_DATA_SIZE)) {
                break;
            }
        }
    }
    LOG(Info) << "[RDL] Transmission end detected, wait for RNL's death" << endl;
    close(pipefd[p_write]);

    LOG(Info) << "[RDL] RDL test passed!" << endl;

    while(1){
        sleep(1);
    }
    return ALL_GOOD;
}

Status sender_datalink_layer_StopAndWait(int *pipefd) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    signal(SIGFRARV, Handler_SIGFRARV);

    LOG(Info) << "[SDL] SDL start" << endl;

    Status rtn = ALL_GOOD;
    int pipe_datalink_physical[2];
    int pipe_physical_datalink[2];

    pid_t phy_pid;

    if(pipe(pipe_datalink_physical) == -1){
        LOG(Error) << "[SDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    if(pipe(pipe_physical_datalink) == -1){
        LOG(Error) << "[RDL] pipe_physical_datalink init error" << endl;
        return E_PIPE_INIT;
    }

    //set pipe nonblock
    int nPipeReadFlag = fcntl(pipe_datalink_physical[p_write], F_GETFL, 0);
    nPipeReadFlag |= O_NONBLOCK;
    if (fcntl(pipe_datalink_physical[p_write], F_SETFL, nPipeReadFlag) < 0) {
        LOG(Error) << "[RDL] pipe_datalink_physical set fcntl error" << endl;
        E_PIPE_INIT;
    }
    
    nPipeReadFlag = fcntl(pipe_physical_datalink[p_read], F_GETFL, 0);
    nPipeReadFlag |= O_NONBLOCK;
    if (fcntl(pipe_physical_datalink[p_write], F_SETFL, nPipeReadFlag) < 0) {
        LOG(Error) << "[RDL] pipe_physical_datalink set fcntl error" << endl;
        E_PIPE_INIT;
    }

    LOG(Info) << "[SDL] pipe init ok" << endl;

    phy_pid = fork();
    if(phy_pid < 0){
        LOG(Error) << "[SDL] fork unsuccessful" << endl;
        return E_FORK;
    }

    //physical layer proc
    else if(phy_pid == 0){
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        rtn = sender_physical_layer(pipe_datalink_physical, pipe_physical_datalink);
        if(rtn == TRANSMISSION_END){
            LOG(Info) << "sender: Transmission end in SDL." << endl;
            return rtn;
        }
        else if(rtn < 0){
            LOG(Error) << "[SPL] Error occured in SPL with code: " << rtn << endl;
            return rtn;
        }
        else{    //return ALL_GOOD
            LOG(Info) << "[SPL] SPL end with success" << endl;
            return ALL_GOOD;
        }   
    }//end of else if

    //datalink layer proc
    else{    
        //clsoe write port
        close(pipefd[p_write]);
        frame s;
        packet buffer;
        event_type event;

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

            while(true){
                wait_for_event(event);
                if(event == frame_arrival)
                    break;
            }
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

Status receiver_datalink_layer_StopAndWait(int *pipefd) {
    //exit when father proc exit
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    //avoid zonbe proc
    signal(SIGCHLD, SIG_IGN);
    signal(SIGFRARV, Handler_SIGFRARV);

    LOG(Info) << "[RDL] RDL start" << endl;

    Status rtn = ALL_GOOD;
    int pipe_physical_datalink[2];
    int pipe_datalink_physical[2];

    if(pipe(pipe_datalink_physical) == -1){
        LOG(Error) << "[SDL] pipe_datalink_physical init error" << endl;
        return E_PIPE_INIT;
    }
    if(pipe(pipe_physical_datalink) == -1){
        LOG(Error) << "[RDL] pipe_physical_datalink init error" << endl;
        return E_PIPE_INIT;
    }
    
    //set pipe nonblock
    int nPipeReadFlag = fcntl(pipe_datalink_physical[p_write], F_GETFL, 0);
    nPipeReadFlag |= O_NONBLOCK;
    if (fcntl(pipe_datalink_physical[p_write], F_SETFL, nPipeReadFlag) < 0) {
        LOG(Error) << "[RDL] pipe_datalink_physical set fcntl error" << endl;
        E_PIPE_INIT;
    }
    
    nPipeReadFlag = fcntl(pipe_physical_datalink[p_read], F_GETFL, 0);
    nPipeReadFlag |= O_NONBLOCK;
    if (fcntl(pipe_physical_datalink[p_write], F_SETFL, nPipeReadFlag) < 0) {
        LOG(Error) << "[RDL] pipe_physical_datalink set fcntl error" << endl;
        E_PIPE_INIT;
    }

    LOG(Info) << "[RDL] pipe init ok" << endl;

    pid_t phy_pid = fork();
    if(phy_pid < 0){
        LOG(Error) << "[RDL] fork unsuccessful" << endl;
        return E_FORK;
    }

    //physical layer proc 
    else if(phy_pid == 0){
        rtn = receiver_physical_layer(pipe_datalink_physical, pipe_physical_datalink);
        // if(rtn == TRANSMISSION_END){
        //     LOG(Info) << "receiver: Transmission end in SDL." << endl;
        //     return rtn;
        // }
        if(rtn < 0){
            LOG(Error) << "[RPL] Error occured in RPL with code: " << rtn << endl;
            return rtn;
        }
        else {
            LOG(Info) << "[RPL] SPL end with success" << endl;
            return ALL_GOOD;
        } 
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
            if (0 == memcmp(r.info.data, all_zero, RAW_DATA_SIZE)) {
                break;
            }
        }
    }
    LOG(Info) << "[RDL] Transmission end detected, wait for RNL's death" << endl;
    close(pipefd[p_write]);

    LOG(Info) << "[RDL] RDL test passed!" << endl;

    while(1){
        sleep(1);
    }
    return ALL_GOOD;
}

Status from_network_layer(packet *p, int *pipefd){
    char pipe_buf[RAW_DATA_SIZE + 1];
    //p_write closed in upper function
    if (read(pipefd[p_read], pipe_buf, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "[SDL] read from SNL error." << endl;
        return E_PIPE_READ;
    }
    //data starts from data[12]
    memcpy(p->data, pipe_buf, RAW_DATA_SIZE);
    LOG(Info) << "[SDL] successfully gets info from SNL."<< endl;
    return ALL_GOOD;
}        

Status to_physical_layer(frame *s, int *pipefd) {
        char pipe_buf[LEN_PKG_DATA+1];
        memcpy(pipe_buf, &(s->kind), sizeof(int));
        memcpy(pipe_buf+4, &(s->seq), sizeof(int));
        memcpy(pipe_buf+8, &(s->ack), sizeof(int));
        memcpy(pipe_buf+12, s->info.data, RAW_DATA_SIZE);

        close(pipefd[p_read]);  
        if((write(pipefd[p_write], pipe_buf, LEN_PKG_DATA )) <= 0){
            LOG(Error) << "[SDL] write to SPL error." << endl;
            return E_PIPE_WRITE;
        }
        LOG(Info) << "[SDL] sent frame to SPL successfully." << endl;

        if (0 == memcmp(pipe_buf, all_zero, RAW_DATA_SIZE)) {
            return TRANSMISSION_END;
        }
        else
            return ALL_GOOD;
}

Status to_network_layer(packet *p, int *pipefd) {
    //p_read closed in upper function
    if (write(pipefd[p_write], p->data, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "[RDL] Pipe write from RDL to RNL error" << endl;
        return E_PIPE_WRITE;
    }
    LOG(Info) << "[RDL] send data to RNL successfully."<< endl;
    return ALL_GOOD;
}     

Status from_physical_layer(frame *s, int *pipefd) {
    char pipe_buf[LEN_PKG_DATA+1] = {0};

    close(pipefd[p_write]); 
    if((read(pipefd[p_read], pipe_buf, LEN_PKG_DATA)) <= 0){
        LOG(Error) << "[RDL] Pipe read from RPL to RDL error" << endl;
        return E_PIPE_READ;
    }
    memcpy(&(s->kind), pipe_buf, sizeof(int));
    memcpy(&(s->seq), pipe_buf+4, sizeof(int));
    memcpy(&(s->ack), pipe_buf+8, sizeof(int));
    memcpy(s->info.data, pipe_buf+12, RAW_DATA_SIZE);

    LOG(Info) << "[RDL] read frame from SPL successfully." << endl;
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

int tcp_server_nonblock(const int port) {
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

    int flags;
    flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

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

int tcp_client_nonblock(const char *ip, const int port) {
    // AF_INET_IPv4 protocol
    // SOCK_STREAM: TCP protocol
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) { 
        graceful_return("socket", E_CREATE_SOCKET);
    }
    LOG(Debug) << "server socket." << endl;

    int flags;
    flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);


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
        int val_recv = recv(socket, buffer, buf_length - total_recv, 0);
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

Status sender_physical_layer(int *pipefd_down, int *pipefd_up) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    int client_fd = tcp_client_block();
    LOG(Debug) << "[SPL] client_fd\t" << client_fd << endl;
    if (client_fd < 0) {
        LOG(Error) << "[SPL] An error occured when initializing TCP client socoket or connect with error code: " << client_fd << endl;
        return client_fd;
    }

    close(pipefd_down[p_write]);
    if(pipefd_up)
        close(pipefd_up[p_read]);

    int r_rtn, w_rtn;
    int *r_seq;
    frame r;
    char buffer[LEN_PKG_DATA] = {0};
    Status val_physical_layer_send;
    Status val_physical_layer_recv;
    int flag_trans_end = false;
    int flag_sleep = true;

    while(1) {
        flag_sleep = true;
        //nonblock
        r_rtn = read(pipefd_down[p_read], buffer, LEN_PKG_DATA);
        if(r_rtn <= 0 && errno != EAGAIN){
            LOG(Error) << "[SPL] Pipe read from SNL to SDL error" << endl;
            return E_PIPE_READ;
        }
        //if r_rtn == -1 and errno == EAGAIN, mean temporarily no data to read, just don't read this time
        if (r_rtn > 0) {
            flag_sleep = false;
            LOG(Debug) << "[SPL] Read info from SNL: " << buffer << endl;

            if (0 != memcmp(all_zero, buffer+LEN_PKG_NODATA, RAW_DATA_SIZE)) {
                val_physical_layer_send = physical_layer_send(client_fd, buffer);

            }
            else {  //transmission end
                LOG(Info) << "[SPL] Transmission end, detected by SPL" << endl;
                val_physical_layer_send = physical_layer_send(client_fd, buffer, true, true);
                flag_trans_end = true;
            }

            LOG(Debug) << "[SPL] val_physical_layer_send\t" << val_physical_layer_send << endl;
            if (val_physical_layer_send < 0) {
                LOG(Error) << "[SPL] An error occured, val_physical_layer_send code: " << val_physical_layer_send << endl;
                return val_physical_layer_send;
            }
        }//end of if read > 0

        if(pipefd_up){
            //expecting to recv ACK/NAK
            val_physical_layer_recv = physical_layer_recv(client_fd, buffer, false);
            
            //nothing received
            if(val_physical_layer_recv == E_RECV)   
                continue;
            //other error: ignore this packet
            else if(val_physical_layer_recv < 0)
                continue;
            else{    //recv frame from receiver: ACK/NAK
                flag_sleep = false;
                LOG(Debug) << "[RPL] Get info from SPL: " << buffer << endl;
                
                //to test if recved ACK/NAK frame
                r_seq = buffer+4;
                if((*r_seq) != 0xFFFFFFFF){
                    LOG(Error) << "received unkown frame type!" <<endl;
                    continue;
                }
                //recognized ACK/NAK
                //set buffer+12 ~ buffer+1035 all zero
                memcpy(buffer+12, all_zero, RAW_DATA_SIZE);
                if (write(pipefd_up[p_write], buffer, LEN_PKG_DATA) < 0) {
                    LOG(Error) << "Pipe write from RPL to RDL error" << endl;
                    return E_PIPE_WRITE;
                }
                kill(getppid(), SIGFRARV);

                //after recv ACK
                if(flag_trans_end == true)
                    break;
            }//end of else
        }
        else
            if(flag_trans_end == true)
                break;
    
        //if nothing happened in this loop
        if(flag_sleep == true)
            usleep(1);
    }//end of while

    //detected transmission_end
    close(pipefd_down[p_read]);
    if(pipefd_up)
        close(pipefd_up[p_write]);
    // THINK: do we need to sleep here? How long do we need to sleep?
    //sleep(1);
    // THINK: can client_fd be closed here?
    while(1) {
        if (0 == send(client_fd, NULL, 0, 0)) 
            break;
        else 
            sleep(1);
    }
    LOG(Info) << "[SPL] peer(RPL) disconnected detected, SPL will end too." << endl;
    close(client_fd);
    return TRANSMISSION_END;
}

Status receiver_physical_layer(int *pipefd_down, int *pipefd_up) {
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    int server_fd = tcp_server_block();
    LOG(Debug) << "[RPL] server_fd\t" << server_fd << endl;
    if (server_fd < 0) {
        LOG(Error) << "[RPL] An error occured when initializing TCP server socoket or connect with error code: " << server_fd << endl;
        return server_fd;
    }

    close(pipefd_up[p_read]);
    if(pipefd_down)
        close(pipefd_down[p_write]);

    int r_rtn, w_rtn;
    int *r_seq;
    frame s;
    char buffer[LEN_PKG_DATA] = {0};   
    Status val_physical_layer_recv;
    Status val_physical_layer_send;
    int flag_trans_end = false;
    int flag_sleep = true;

    while(1) {
        flag_sleep = true;
        //nonblock
        val_physical_layer_recv = physical_layer_recv(server_fd, buffer);   //is_data = true
        LOG(Debug) << "[RPL] val_physical_layer_recv\t" << val_physical_layer_recv << endl;

        //switch(recv < 0):
        //case(E_RECV): nothing recved, do nothing
        //case(other error): ignore this packet
        if(val_physical_layer_recv >= 0){
            flag_sleep = false;
            LOG(Debug) << "[RPL] Get info from SPL: " << buffer << endl;

            if (write(pipefd_up[p_write], buffer, LEN_PKG_DATA) < 0) {
                LOG(Error) << "Pipe write from RPL to RDL error" << endl;
                return E_PIPE_WRITE;
            }
            kill(getppid(), SIGFRARV);

            if (val_physical_layer_recv == TRANSMISSION_END)
                flag_trans_end = true;           
        }//end of if recv >= 0

        if(pipefd_down){
            r_rtn = read(pipefd_down[p_read], buffer, LEN_PKG_DATA);
            if(r_rtn <= 0 && errno != EAGAIN){
                LOG(Error) << "[SPL] Pipe read from SNL to SDL error" << endl;
                return E_PIPE_READ;
            }
            //send ACK
            if(r_rtn > 0){
                flag_sleep = false;
                memcpy(buffer, &(s->kind), sizeof(int));
                memcpy(buffer+4, &(s->seq), sizeof(int));
                memcpy(buffer+8, &(s->ack), sizeof(int));
                val_physical_layer_send = physical_layer_send(server_fd, buffer, false);

                LOG(Debug) << "[SPL] val_physical_layer_send\t" << val_physical_layer_send << endl;
                if (val_physical_layer_send < 0) {
                    LOG(Error) << "[SPL] An error occured, val_physical_layer_send code: " << val_physical_layer_send << endl;
                    return val_physical_layer_send;
                }
                if(flag_trans_end)
                    break;
            }//end of r_rtn > 0
        }//end of if pipefd_down
        else             
            if(flag_trans_end)
                break;
        //if nothing happened in this loop
        if(flag_sleep == true)
            usleep(1);
    }//end of while

    LOG(Info) << "[RPL] Transmission end, detected by RPL" << endl;
    close(pipefd_up[p_write]);
    close(pipefd_down[p_read]);
    // THINK: can server_fd be closed here?
    close(server_fd);
    LOG(Info) << "[RPL] Wait for RDL's death." << endl;
    while(1) {
        sleep(1);
    }
    return TRANSMISSION_END;
}
