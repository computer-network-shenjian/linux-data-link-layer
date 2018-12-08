#include "../common/shared_library.hpp"


using namespace std;



void Handler_SIGFRARV(int sig)
{
	sig_frame_arrival = 1;
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


void wait_for_event(event_type event)
{
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
