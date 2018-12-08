#include "../common/shared_library.hpp"


using namespace std;


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

void enable_network_layer(void){
}
