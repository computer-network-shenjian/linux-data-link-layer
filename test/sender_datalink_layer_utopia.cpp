#include "../common/shared_library.hpp"


using namespace std;


Status sender_datalink_layer_utopia(int *pipefd){
    Status rtn = ALL_GOOD;
    int pipe_datalink_physical[2];
    pid_t phy_pid;

    if(pipe(pipe_datalink_physical) == -1){
        LOG(Error) << "sender: Pipe INIT error in datalink layer." << endl;
        return E_PIPE_INIT;
    }

    phy_pid = fork();

    if(phy_pid < 0){
        LOG(Error) << "sender: SDL fork error." << endl;
        return E_FORK;
    }

    //physical layer proc
    else if(phy_pid == 0){
        LOG(Info) << "sender: SPL start."<< endl;
        while(rtn >= 0){
            rtn = sender_physical_layer(pipe_datalink_physical);
            
            if(rtn == TRANSMISSION_END){
                LOG(Info) << "sender: Transmission end in SDL." << endl;
                return rtn;
            }
            else if(rtn < 0)
                LOG(Error) << "sender: SPL failed, returned error in SDL." << endl;
            else    //return ALL_GOOD
                LOG(Info) << "sender: SPL send frame successfully." << endl;
        }
        LOG(Info) << "sender: SPL end." << endl;
    }

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
            //error, return
            if(rtn < 0)
                return rtn;
            else
                continue;
        }
        pid_t wait_pid = waitpid(phy_pid, NULL, 0); //wait for phy_pid exit
        LOG(Debug) << "waitpid: " << wait_pid <<endl;
    }
}


Status from_network_layer(packet *p, int *pipefd){
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
        return ALL_GOOD;
}

void enable_network_layer(void){
}
