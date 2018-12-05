#include "../common/shared_library.hpp"


using namespace std;



void Handler_SIGFRARV(int sig)
{
	sig_frame_arrival = 1;
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

Status to_network_layer(packet *p, int *pipe)
{
	//p_read closed in upper function
    if (write(pipe[p_write], p->data, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "receiver: SNL send to SDL error." << endl;
        return E_PIPE_WRITE;
    }
    LOG(Info) << "sender: SDL successfully gets info from SNL."<< endl;
    return ALL_GOOD;
}   


Status from_physical_layer(frame *s, int *pipe)
{
    char pipe_buf[LEN_PKG_DATA+1];

    close(pipe[p_write]); 
    if((read(pipe[p_read], pipe_buf, LEN_PKG_DATA)) <= 0){
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
