Status RDL_SlidingWindow(int *pipefd) {
    LOG(Info) << "[RDL] RDL start" << endl;

    /*********** signal init begin ***********/

    //exit when father proc exit
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    //avoid zombie proc
    signal(SIGCHLD, SIG_IGN);
    signal(SIGFRARV, handler_SIGFRARV);
    signal(SIGCKERR, handler_SIGCKERR);
    
    /*********** signal init end ***********/

    /*********** Pipe init begin ***********/

    int pipe_datalink_physical[2];
    int pipe_physical_datalink[2];

    if(nonblock_pipe_init(pipe_datalink_physical) != ALL_GOOD)
    	return E_PIPE_INIT;
    if(nonblock_pipe_init(pipe_physical_datalink) != ALL_GOOD)
    	return E_PIPE_INIT;

    LOG(Info) << "[RDL] pipe init ok" << endl;

    /*********** Pipe init end ***********/
    
    Status rtn = ALL_GOOD;
    pid_t phy_pid = fork();
    if(phy_pid < 0){
        LOG(Error) << "[RDL] fork unsuccessful" << endl;
        return E_FORK;
    }

    //physical layer proc
    else if(phy_pid == 0){
        rtn = RPL(pipe_datalink_physical, pipe_physical_datalink);
        else if(rtn < 0){
            LOG(Error) << "[RPL] Error occured in RPL with code: " << rtn << endl;
            return rtn;
        }
        else{    //return ALL_GOOD
            LOG(Info) << "[RPL] RPL end with success" << endl;
            return ALL_GOOD;
        }   
    }//end of else if

    //datalink layer proc
    else{    
        //close write port
        close(pipefd[p_read]);

        seq_nr next_frame_to_send = 0;
        seq_nr frame_expected = 0;
        frame s, r;
        packet buffer;
        event_type event = no_event;
        int N_rtn, P_rtn;
  //       rtn = from_net  work_layer(&buffer, pipefd);
  //       if(rtn == E_PIPE_READ)  
  //           return rtn;

  //       s.kind = hton(data);
  //       s.seq = hton(next_frame_to_send);
  //       s.ack = hton(frame_expected);
		// s.info = buffer;

  //       to_physical_layer(&s, pipe_datalink_physical);
  //       start_timer(s.seq);
        s.kind = hton(ack);
        s.seq = hton(0xFFFFFFFF);
        s.ack = hton(1-frame_expected);

        wait_for_event(event);
        if(event == frame_arrival) {
            //after frame_arrived
            P_rtn = from_physical_layer(&r, pipe_physical_datalink, true); //recv data
            if(P_rtn < 0)
                return P_rtn;
            LOG(Debug) << "[RDL] get frame from tcp." << endl;
            
            if(r.seq == frame_expected){
                N_rtn = to_network_layer(&r.info, pipefd);
                if(N_rtn < 0)
                    return N_rtn;
                inc_1(frame_expected);
            }
        }//end of event frame_arrival

        rtn = to_physical_layer(&s, pipe_datalink_physical, false);
        if(rtn < 0)
            return rtn;
        // TODO: check if upper code do or downer code do.
        if (0 == memcmp(s.info.data, all_zero, RAW_DATA_SIZE)) {
            break;
        }
        start_ack_timer();   

        while(true){
        	LOG(Info) << "[RDL] beginning waiting for event\n";        
            wait_for_event(event);
            //frame_arrival
            if(event == frame_arrival) {
                //after frame_arrived
                P_rtn = from_physical_layer(&r, pipe_physical_datalink, true); //recv data
                if(P_rtn < 0)
                    return P_rtn;
                LOG(Debug) << "[RDL] get frame from tcp." << endl;
			    
			    if(r.seq == frame_expected){
			    	N_rtn = to_network_layer(&r.info, pipefd);
                    if(N_rtn < 0)
                        return N_rtn;
			    	inc_1(frame_expected);
			    }
			   	if(r.ack == next_frame_to_semd){
			   		stop_ack_timer();
			   		from_network_layer(&buffer, pipefd);
			   		inc_1(next_frame_to_send);
			   	}
            }//end of event frame_arrival
            //encountered frame_arrival or
            //other event(timeout)	

            //this buffer could be the old one or new one
            s.info = buffer;
            s.ack = hton(1-frame_expected);

            rtn = to_physical_layer(&s, pipe_datalink_physical, false);
            if(rtn < 0)
                return rtn;
            // TODO: check if upper code do or downer code do.
            if (0 == memcmp(s.info.data, all_zero, RAW_DATA_SIZE)) {
                break;
            }
            start_ack_timer();     
        }

    LOG(Info) << "[RDL] Transmission end detected, wait for RNL's death." << endl;
    close(pipefd[p_write]);

    //LOG(Info) << "[RDL] RDL test passed!" << endl;
    while(1){
        sleep(1);
    }
    return ALL_GOOD;
}
