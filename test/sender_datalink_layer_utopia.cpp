#include "../common/shared_library.hpp"


using namespace std;


Status sender_datalink_layer_utopia(int *pipe){
    //clsoe write port
    close(pipe[p_write]);
    frame s;
    packet buffer;
    Status rtn;

    //enable_network_layer();
    while(true){
        rtn = from_network_layer(&buffer, pipe);
        if(rtn == E_PIPE_READ)  
            return rtn;

        s.info = buffer;
        return (to_physical_layer(&s));
    }
}

Status from_network_layer(packet *p, int *pipe){
    char pipe_buf[RAW_DATA_SIZE + 1];
    //p_write closed in upper function
    if (read(pipe[p_read], pipe_buf, RAW_DATA_SIZE) <= 0) {
        LOG(Error) << "sender: SNL read from SDL error." << endl;
        return E_PIPE_READ;
    }
    //data starts from data[12]
    memcpy(p->data, pipe_buf, RAW_DATA_SIZE);
    LOG(Info) << "sender: SDL successfully gets info from SNL."<< endl;
    return ALL_GOOD;
}   

Status to_physical_layer(frame *s){
    Status rtn;
    int pipe_datalink_physical[2];
    pid_t phy_pid;

    if(pipe(pipe_datalink_physical) == -1){
        LOG(Error) << "sender: Pipe INIT error in datalink layer." << endl;
        return E_PIPE_INIT;
    }
    //avoid zonbe proc
    signal(SIGCHLD, SIG_IGN);
    phy_pid = fork();

    if(phy_pid < 0){
        LOG(Error) << "sender: SDL fork error." << endl;
        return E_FORK;
    }
    else if(phy_pid == 0){
        LOG(Info) << "sender: SPL start."<< endl;
        rtn = sender_physical_layer(pipe_datalink_physical);
        
        if(rtn == TRANSMISSION_END)
            LOG(Info) << "sender: Transmission end in SDL." << endl;
        else if(rtn < 0)
            LOG(Error) << "sender: SPL failed, returned error in SDL." << endl;
        else    //return ALL_GOOD
            LOG(Info) << "sender: SPL end successfully." << endl;
        return rtn;
    }
    else{
        char pipe_buf[LEN_PKG_DATA+1];
        memcpy(pipe_buf, &(s->kind), sizeof(int));
        memcpy(pipe_buf+4, &(s->seq), sizeof(int));
        memcpy(pipe_buf+8, &(s->ack), sizeof(int));
        memcpy(pipe_buf+12, s->info.data, RAW_DATA_SIZE);

        close(pipe_datalink_physical[p_read]);  
        if((write(pipe_datalink_physical[p_write], pipe_buf, LEN_PKG_DATA )) <= 0){
            LOG(Error) << "sender: SDL write to SPL error." << endl;
            return E_PIPE_WRITE;
        }
        LOG(Info) << "sender: SDL sent frame to SPL successfully." << endl;
        return ALL_GOOD;
    }
}

void enable_network_layer(void){
}
